#ifndef DEBUG_H
#define DEBUG_H

void PrintLog(const char *msg, ...);

enum DevMenuMenus {
    DEVMENU_MAIN,
    DEVMENU_PLAYERSEL,
    DEVMENU_STAGELISTSEL,
    DEVMENU_STAGESEL,
    DEVMENU_SCRIPTERROR,
#if RETRO_USE_MOD_LOADER
    DEVMENU_MODMENU,
#endif
};

void InitDevMenu();
void InitErrorMessage();
void ProcessStageSelect();

#endif //! DEBUG_H
