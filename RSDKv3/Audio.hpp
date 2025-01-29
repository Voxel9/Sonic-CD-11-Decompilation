#ifndef AUDIO_H
#define AUDIO_H

#include <stdlib.h>

#if RETRO_PLATFORM == RETRO_3DS
#include <tremor/ivorbisfile.h>
#else
#include <vorbis/vorbisfile.h>
#endif

#if RETRO_PLATFORM != RETRO_3DS && RETRO_PLATFORM != RETRO_3DSSIM && RETRO_PLATFORM != RETRO_VITA && RETRO_PLATFORM != RETRO_OSX
#include "SDL.h"
#endif

#if RETRO_USING_SDL2_AUDIO

#define LockAudioDevice()   SDL_LockAudio()
#define UnlockAudioDevice() SDL_UnlockAudio()

#else
#define LockAudioDevice()   ;
#define UnlockAudioDevice() ;
#endif

#define TRACK_COUNT   (0x10)
#define SFX_COUNT     (0x100)
#define CHANNEL_COUNT (0x4)
#define SFXDATA_COUNT (0x400000)

#define MAX_VOLUME (100)

#define STREAMFILE_COUNT (2)

#define MIX_BUFFER_SAMPLES (256)

struct TrackInfo {
    char fileName[0x40];
    bool trackLoop;
    uint loopPoint;

    byte *buffer;
    int fileSize;
    int filePos;
};

struct StreamInfo {
    OggVorbis_File vorbisFile;
    int vorbBitstream;
#if RETRO_USING_SDL1_AUDIO
    SDL_AudioSpec spec;
#endif
#if RETRO_USING_SDL2_AUDIO
    SDL_AudioStream *stream;
#endif
    short buffer[MIX_BUFFER_SAMPLES];
    bool trackLoop;
    uint loopPoint;
    bool loaded;
};

struct SFXInfo {
    char name[0x40];
    short *buffer;
    size_t length;
    bool loaded;
};

struct ChannelInfo {
    size_t sampleLength;
    short *samplePtr;
    int sfxID;
    byte loopSFX;
    sbyte pan;
};

enum MusicStatuses {
    MUSIC_STOPPED = 0,
    MUSIC_PLAYING = 1,
    MUSIC_PAUSED  = 2,
    MUSIC_LOADING = 3,
    MUSIC_READY   = 4,
};

extern int globalSFXCount;
extern int stageSFXCount;

extern int masterVolume;
extern int trackID;
extern int sfxVolume;
extern int bgmVolume;
extern bool audioEnabled;

extern int nextChannelPos;
extern bool musicEnabled;
extern int musicStatus;
extern TrackInfo musicTracks[TRACK_COUNT];
extern SFXInfo sfxList[SFX_COUNT];

extern ChannelInfo sfxChannels[CHANNEL_COUNT];

extern int currentStreamIndex;
extern StreamInfo streamInfo[STREAMFILE_COUNT];
extern TrackInfo* streamFilePtr;
extern StreamInfo *streamInfoPtr;

extern int currentMusicTrack;

#if RETRO_USING_SDL1_AUDIO || RETRO_USING_SDL2_AUDIO
extern SDL_AudioSpec audioDeviceFormat;
#endif

int InitAudioPlayback();
void LoadGlobalSfx();

void ProcessMusicStream(int *stream, size_t bytes_wanted);
void ProcessAudioPlayback(void *userdata, unsigned char *stream, int len);
void ProcessAudioMixing(int *dst, const short *src, int len, int volume, char pan);

inline void FreeMusInfo()
{
    LockAudioDevice();

#if RETRO_USING_SDL2_AUDIO
    if (streamInfo[currentStreamIndex].stream)
        SDL_FreeAudioStream(streamInfo[currentStreamIndex].stream);
#endif
    ov_clear(&streamInfo[currentStreamIndex].vorbisFile);
#if RETRO_USING_SDL2_AUDIO
    streamInfo[currentStreamIndex].stream = nullptr;
#endif

    UnlockAudioDevice();
}

#if RETRO_USE_MOD_LOADER
extern char globalSfxNames[SFX_COUNT][0x40];
extern char stageSfxNames[SFX_COUNT][0x40];
void SetSfxName(const char *sfxName, int sfxID, bool global);
#endif

void LoadMusic();
void SetMusicTrack(char *filePath, byte trackID, bool loop, uint loopPoint);
bool PlayMusic(int track);
inline void StopMusic()
{
    musicStatus = MUSIC_STOPPED;
    FreeMusInfo();
}

void LoadSfx(char *filePath, byte sfxID);
void PlaySfx(int sfx, bool loop);
inline void StopSfx(int sfx)
{
    for (int i = 0; i < CHANNEL_COUNT; ++i) {
        if (sfxChannels[i].sfxID == sfx) {
            MEM_ZERO(sfxChannels[i]);
            sfxChannels[i].sfxID = -1;
        }
    }
}
void SetSfxAttributes(int sfx, int loopCount, sbyte pan);

inline void SetMusicVolume(int volume)
{
    if (volume < 0)
        volume = 0;
    if (volume > MAX_VOLUME)
        volume = MAX_VOLUME;
    masterVolume = volume;
}

inline bool PauseSound()
{
    if (musicStatus == MUSIC_PLAYING) {
        musicStatus = MUSIC_PAUSED;
        return true;
    }
    return false;
}

inline void ResumeSound()
{
    if (musicStatus == MUSIC_PAUSED)
        musicStatus = MUSIC_PLAYING;
}

inline void StopAllSfx()
{
    for (int i = 0; i < CHANNEL_COUNT; ++i) sfxChannels[i].sfxID = -1;
}
inline void ReleaseGlobalSfx()
{
    StopAllSfx();
    for (int i = globalSFXCount - 1; i >= 0; --i) {
        if (sfxList[i].loaded) {
            StrCopy(sfxList[i].name, "");
            free(sfxList[i].buffer);
            sfxList[i].length = 0;
            sfxList[i].loaded = false;
        }
    }
    globalSFXCount = 0;
}
inline void ReleaseStageSfx()
{
    for (int i = stageSFXCount + globalSFXCount; i >= globalSFXCount; --i) {
        if (sfxList[i].loaded) {
            StrCopy(sfxList[i].name, "");
            free(sfxList[i].buffer);
            sfxList[i].length = 0;
            sfxList[i].loaded = false;
        }
    }
    stageSFXCount = 0;
}

void ReleaseAudioDevice();

#endif // !AUDIO_H
