#include "RetroEngine.hpp"

#if RETRO_USE_MOD_LOADER || !RETRO_USE_ORIGINAL_CODE
char savePath[0x100];
#endif

#if RETRO_USE_MOD_LOADER
std::vector<ModInfo> modList;
int activeMod = -1;

char modsPath[0x100];

bool redirectSave           = false;
bool disableSaveIniOverride = false;

char modTypeNames[OBJECT_COUNT][0x40];
char modScriptPaths[OBJECT_COUNT][0x40];
byte modScriptFlags[OBJECT_COUNT];
byte modObjCount = 0;

char playerNames[PLAYERNAME_COUNT][0x20];
byte playerCount = 0;

#include <dirent.h>
#include <sys/stat.h>

int OpenModMenu()
{
    // Engine.gameMode      = ENGINE_INITMODMENU;
    // Engine.modMenuCalled = true;
    return 1;
}

std::string toLowerCase(const std::string &str) {
    std::string result = str;
    std::transform(result.begin(), result.end(), result.begin(), ::tolower);
    return result;
}

std::string ResolvePath(const std::string &givenPath) {
    std::string resolvedPath = givenPath;

    // Make path absolute if it is relative
    if (givenPath.find(BASE_PATH) != 0) {
        resolvedPath = BASE_PATH + givenPath;
    }

    // Extract parent directory and target filename
    size_t lastSlashPos = resolvedPath.find_last_of('/');
    std::string parentDir = resolvedPath.substr(0, lastSlashPos);
    std::string targetFile = resolvedPath.substr(lastSlashPos + 1);

    // Open the parent directory
    DIR *dir = opendir(parentDir.c_str());
    if (dir == NULL) {
        // If the directory cannot be opened, return the original path
        return resolvedPath;
    }

    struct dirent *entry;
    while ((entry = readdir(dir)) != NULL) {
        // Skip "." and ".." entries
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) {
            continue;
        }

        // Case-insensitive comparison of filenames
        std::string entryName = entry->d_name;
        if (toLowerCase(entryName) == toLowerCase(targetFile)) {
            // Construct the full path to the matched file
            closedir(dir);
            return parentDir + "/" + entryName;
        }
    }

    closedir(dir);

    // Return the original path if no match was found
    return resolvedPath;
}

bool pathExists(const std::string &path) {
    struct stat s;
    return (stat(path.c_str(), &s) == 0);
}

bool isRegularFile(const std::string &path) {
    struct stat s;
    if (stat(path.c_str(), &s) == 0 && S_ISREG(s.st_mode)) {
        return true;
    }
    return false;
}

bool isDirectory(const std::string &path) {
    struct stat s;
    if (stat(path.c_str(), &s) == 0 && S_ISDIR(s.st_mode)) {
        return true;
    }
    return false;
}

void InitMods()
{
    modList.clear();
    forceUseScripts        = forceUseScripts_Config;
    disableFocusPause      = disableFocusPause_Config;
    redirectSave           = false;
    disableSaveIniOverride = false;
    sprintf(savePath, "");

    char modBuf[0x100];
    sprintf(modBuf, "%smods", modsPath);
    std::string modPath = ResolvePath(modBuf);

    if (pathExists(modPath) && isDirectory(modPath)) {
        std::string mod_config = modPath + "/modconfig.ini";
        FileIO *configFile     = fOpen(mod_config.c_str(), "r");
        if (configFile) {
            fClose(configFile);
            IniParser modConfig(mod_config.c_str(), false);

            for (int m = 0; m < modConfig.items.size(); ++m) {
                bool active = false;
                ModInfo info;
                modConfig.GetBool("mods", modConfig.items[m].key, &active);
                if (LoadMod(&info, modPath, modConfig.items[m].key, active))
                    modList.push_back(info);
            }
        }

        DIR *dir = opendir(modPath.c_str());
        if (!dir) {
            // Uh oh
        }

        struct dirent *entry;
        while ((entry = readdir(dir)) != NULL) {
            // Skip "." and ".."
            if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) {
                continue;
            }

            std::string fullPath = modPath + "/" + entry->d_name;

            if (isDirectory(fullPath)) {
                ModInfo info;

                std::string modDir            = fullPath.c_str();
                const std::string mod_inifile = modDir + "/mod.ini";
                std::string folder            = entry->d_name;

                bool flag = true;
                for (int m = 0; m < modList.size(); ++m) {
                    if (modList[m].folder == folder) {
                        flag = false;
                        break;
                    }
                }

                if (flag) {
                    if (LoadMod(&info, modPath, entry->d_name, false))
                        modList.push_back(info);
                }
            }
        }

        closedir(dir);
    }

    disableFocusPause = disableFocusPause_Config;
    forceUseScripts   = forceUseScripts_Config;
    sprintf(savePath, "");
    redirectSave           = false;
    disableSaveIniOverride = false;
    for (int m = 0; m < modList.size(); ++m) {
        if (!modList[m].active)
            continue;
        if (modList[m].useScripts)
            forceUseScripts = true;
        if (modList[m].disableFocusPause)
            disableFocusPause |= modList[m].disableFocusPause;
        if (modList[m].redirectSave) {
            sprintf(savePath, "%s", modList[m].savePath.c_str());
            redirectSave = true;
        }
        if (modList[m].disableSaveIniOverride)
            disableSaveIniOverride = true;
    }

    ReadSaveRAMData();
    ReadUserdata();
}

bool LoadMod(ModInfo *info, std::string modsPath, std::string folder, bool active)
{
    if (!info)
        return false;

    info->fileMap.clear();
    info->name    = "";
    info->desc    = "";
    info->author  = "";
    info->version = "";
    info->folder  = "";
    info->active  = false;

    const std::string modDir = modsPath + "/" + folder;

    FileIO *f = fOpen((modDir + "/mod.ini").c_str(), "r");
    if (f) {
        fClose(f);
        IniParser modSettings((modDir + "/mod.ini").c_str(), false);

        info->name    = "Unnamed Mod";
        info->desc    = "";
        info->author  = "Unknown Author";
        info->version = "1.0.0";
        info->folder  = folder;

        char infoBuf[0x100];
        // Name
        StrCopy(infoBuf, "");
        modSettings.GetString("", "Name", infoBuf);
        if (!StrComp(infoBuf, ""))
            info->name = infoBuf;
        // Desc
        StrCopy(infoBuf, "");
        modSettings.GetString("", "Description", infoBuf);
        if (!StrComp(infoBuf, ""))
            info->desc = infoBuf;
        // Author
        StrCopy(infoBuf, "");
        modSettings.GetString("", "Author", infoBuf);
        if (!StrComp(infoBuf, ""))
            info->author = infoBuf;
        // Version
        StrCopy(infoBuf, "");
        modSettings.GetString("", "Version", infoBuf);
        if (!StrComp(infoBuf, ""))
            info->version = infoBuf;

        info->active = active;

        ScanModFolder(info);

        info->useScripts = false;
        modSettings.GetBool("", "TxtScripts", &info->useScripts);
        if (info->useScripts && info->active)
            forceUseScripts = true;

        info->disableFocusPause = false;
        modSettings.GetInteger("", "DisableFocusPause", &info->disableFocusPause);
        if (info->disableFocusPause && info->active)
            disableFocusPause |= info->disableFocusPause;

        info->redirectSave = false;
        modSettings.GetBool("", "RedirectSaveRAM", &info->redirectSave);
        if (info->redirectSave) {
            char path[0x100];
            sprintf(path, "mods/%s/", folder.c_str());
            info->savePath = path;
        }

        info->disableSaveIniOverride = false;
        modSettings.GetBool("", "DisableSaveIniOverride", &info->disableSaveIniOverride);
        if (info->disableSaveIniOverride && info->active)
            disableSaveIniOverride = true;

        return true;
    }
    return false;
}

void ScanModFolder(ModInfo *info)
{
    if (!info)
        return;

    char modBuf[0x100];
    sprintf(modBuf, "%smods", modsPath);

    std::string modPath = ResolvePath(modBuf);

    const std::string modDir = modPath + "/" + info->folder;

    // Check for Data/ replacements
    std::string dataPath = ResolvePath(modDir + "/Data");

    if (pathExists(dataPath) && isDirectory(dataPath)) {
        std::stack<std::string> dirs;

        // Push the initial directory to the stack
        dirs.push(dataPath);

        while (!dirs.empty()) {
            std::string currentDir = dirs.top();

            // Pop the top directory from the stack
            dirs.pop();

            DIR *dir = opendir(currentDir.c_str());
            if (!dir) {
                continue;
            }

            struct dirent *entry;
            while ((entry = readdir(dir)) != NULL) {
                // Skip "." and ".."
                if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) {
                    continue;
                }

                std::string fullPath = currentDir + "/" + entry->d_name;

                if (isRegularFile(fullPath)) {
                    char modBuf[0x100];
                    StrCopy(modBuf, fullPath.c_str());
                    char folderTest[4][0x10] = {
                        "Data/",
                        "Data\\",
                        "data/",
                        "data\\",
                    };
                    int tokenPos = -1;
                    for (int i = 0; i < 4; ++i) {
                        tokenPos = FindStringToken(modBuf, folderTest[i], 1);
                        if (tokenPos >= 0)
                            break;
                    }

                    if (tokenPos >= 0) {
                        char buffer[0x100];
                        for (int i = StrLength(modBuf); i >= tokenPos; --i) {
                            buffer[i - tokenPos] = modBuf[i] == '\\' ? '/' : modBuf[i];
                        }

                        // PrintLog(modBuf);
                        std::string path(buffer);
                        std::string modPath(modBuf);
                        char pathLower[0x100];
                        memset(pathLower, 0, sizeof(char) * 0x100);
                        for (int c = 0; c < path.size(); ++c) {
                            pathLower[c] = tolower(path.c_str()[c]);
                        }

                        info->fileMap.insert(std::pair<std::string, std::string>(pathLower, modBuf));
                    }
                }
                else if (isDirectory(fullPath)) {
                    dirs.push(fullPath);
                }
            }

            closedir(dir);
        }
    }

    // Check for Scripts/ replacements
    std::string scriptPath = ResolvePath(modDir + "/Scripts");

    if (pathExists(scriptPath) && isDirectory(scriptPath)) {
        std::stack<std::string> dirs;

        // Push the initial directory to the stack
        dirs.push(dataPath);

        while (!dirs.empty()) {
            std::string currentDir = dirs.top();

            // Pop the top directory from the stack
            dirs.pop();

            DIR *dir = opendir(currentDir.c_str());
            if (!dir) {
                continue;
            }

            struct dirent *entry;
            while ((entry = readdir(dir)) != NULL) {
                // Skip "." and ".."
                if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) {
                    continue;
                }

                std::string fullPath = currentDir + "/" + entry->d_name;

                if (isRegularFile(fullPath)) {
                    char modBuf[0x100];
                    StrCopy(modBuf, fullPath.c_str());
                    char folderTest[4][0x10] = {
                        "Scripts/",
                        "Scripts\\",
                        "scripts/",
                        "scripts\\",
                    };
                    int tokenPos = -1;
                    for (int i = 0; i < 4; ++i) {
                        tokenPos = FindStringToken(modBuf, folderTest[i], 1);
                        if (tokenPos >= 0)
                            break;
                    }

                    if (tokenPos >= 0) {
                        char buffer[0x100];
                        for (int i = StrLength(modBuf); i >= tokenPos; --i) {
                            buffer[i - tokenPos] = modBuf[i] == '\\' ? '/' : modBuf[i];
                        }

                        // PrintLog(modBuf);
                        std::string path(buffer);
                        std::string modPath(modBuf);
                        char pathLower[0x100];
                        memset(pathLower, 0, sizeof(char) * 0x100);
                        for (int c = 0; c < path.size(); ++c) {
                            pathLower[c] = tolower(path.c_str()[c]);
                        }

                        info->fileMap.insert(std::pair<std::string, std::string>(pathLower, modBuf));
                    }
                }
                else if (isDirectory(fullPath)) {
                    dirs.push(fullPath);
                }
            }

            closedir(dir);
        }
    }

    // Check for Videos/ replacements
    std::string videosPath = ResolvePath(modDir + "/Videos");

    if (pathExists(videosPath) && isDirectory(videosPath)) {
        std::stack<std::string> dirs;

            // Push the initial directory to the stack
            dirs.push(dataPath);

            while (!dirs.empty()) {
                std::string currentDir = dirs.top();

                // Pop the top directory from the stack
                dirs.pop();

                DIR *dir = opendir(currentDir.c_str());
                if (!dir) {
                    continue;
                }

                struct dirent *entry;
                while ((entry = readdir(dir)) != NULL) {
                    // Skip "." and ".."
                    if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) {
                        continue;
                    }

                    std::string fullPath = currentDir + "/" + entry->d_name;

                    if (isRegularFile(fullPath)) {
                        char modBuf[0x100];
                        StrCopy(modBuf, fullPath.c_str());
                        char folderTest[4][0x10] = {
                            "Videos/",
                            "Videos\\",
                            "videos/",
                            "videos\\",
                        };
                        int tokenPos = -1;
                        for (int i = 0; i < 4; ++i) {
                            tokenPos = FindStringToken(modBuf, folderTest[i], 1);
                            if (tokenPos >= 0)
                                break;
                        }

                        if (tokenPos >= 0) {
                            char buffer[0x100];
                            for (int i = StrLength(modBuf); i >= tokenPos; --i) {
                                buffer[i - tokenPos] = modBuf[i] == '\\' ? '/' : modBuf[i];
                            }

                            // PrintLog(modBuf);
                            std::string path(buffer);
                            std::string modPath(modBuf);
                            char pathLower[0x100];
                            memset(pathLower, 0, sizeof(char) * 0x100);
                            for (int c = 0; c < path.size(); ++c) {
                                pathLower[c] = tolower(path.c_str()[c]);
                            }

                            info->fileMap.insert(std::pair<std::string, std::string>(pathLower, modBuf));
                        }
                    }
                    else if (isDirectory(fullPath)) {
                        dirs.push(fullPath);
                    }
                }

                closedir(dir);
            }
    }
}

void SaveMods()
{
    char modBuf[0x100];
    sprintf(modBuf, "%smods", modsPath);
    std::string modPath = ResolvePath(modBuf);

    if (pathExists(modPath) && isDirectory(modPath)) {
        std::string mod_config = modPath + "/modconfig.ini";
        IniParser modConfig;

        for (int m = 0; m < modList.size(); ++m) {
            ModInfo *info = &modList[m];

            modConfig.SetBool("mods", info->folder.c_str(), info->active);
        }

        modConfig.Write(mod_config.c_str(), false);
    }
}

void RefreshEngine()
{
    // Reload entire engine
    Engine.LoadGameConfig("Data/Game/GameConfig.bin");
#if RETRO_USING_SDL2
    if (Engine.window) {
        char gameTitle[0x40];
        sprintf(gameTitle, "%s%s", Engine.gameWindowText, Engine.usingDataFile_Config ? "" : " (Using Data Folder)");
        SDL_SetWindowTitle(Engine.window, gameTitle);
    }
#elif RETRO_USING_SDL1
    char gameTitle[0x40];
    sprintf(gameTitle, "%s%s", Engine.gameWindowText, Engine.usingDataFile_Config ? "" : " (Using Data Folder)");
    SDL_WM_SetCaption(gameTitle, NULL);
#endif

    ReleaseGlobalSfx();
    LoadGlobalSfx();

    disableFocusPause = disableFocusPause_Config;
    forceUseScripts   = forceUseScripts_Config;
    sprintf(savePath, "");
    redirectSave           = false;
    disableSaveIniOverride = false;
    for (int m = 0; m < modList.size(); ++m) {
        if (!modList[m].active)
            continue;
        if (modList[m].useScripts)
            forceUseScripts = true;
        if (modList[m].disableFocusPause)
            disableFocusPause |= modList[m].disableFocusPause;
        if (modList[m].redirectSave) {
            sprintf(savePath, "%s", modList[m].savePath.c_str());
            redirectSave = true;
        }
        if (modList[m].disableSaveIniOverride)
            disableSaveIniOverride = true;
    }

    SaveMods();

    ReadSaveRAMData();
    ReadUserdata();
}

#endif

#if RETRO_USE_MOD_LOADER || !RETRO_USE_ORIGINAL_CODE
int GetSceneID(byte listID, const char *sceneName)
{
    if (listID >= 3)
        return -1;

    char scnName[0x40];
    int scnPos = 0;
    int pos    = 0;
    while (sceneName[scnPos]) {
        if (sceneName[scnPos] != ' ')
            scnName[pos++] = sceneName[scnPos];
        ++scnPos;
    }
    scnName[pos] = 0;

    for (int s = 0; s < stageListCount[listID]; ++s) {
        char nameBuffer[0x40];

        scnPos = 0;
        pos    = 0;
        while (stageList[listID][s].name[scnPos]) {
            if (stageList[listID][s].name[scnPos] != ' ')
                nameBuffer[pos++] = stageList[listID][s].name[scnPos];
            ++scnPos;
        }
        nameBuffer[pos] = 0;

        if (StrComp(scnName, nameBuffer)) {
            return s;
        }
    }
    return -1;
}
#endif
