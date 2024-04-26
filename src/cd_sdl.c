#include "quakedef.h"
#include <SDL_mixer.h>

static Mix_Music *music = NULL;

int32_t CDAudio_Init(void)
{
    return 0;
}

void CDAudio_Play(uint8_t track, bool looping)
{
    char filename[256];
    snprintf(filename, sizeof(filename), "id1/music/%d.mp3", track);
    music = Mix_LoadMUS(filename);
    if (!music)
    {
        Con_Printf("Could not load music file: %s\n", Mix_GetError());
        return;
    }
    Mix_PlayMusic(music, looping ? -1 : 1);
}

void CDAudio_Stop(void)
{
    if (music)
    {
        Mix_HaltMusic();
        Mix_FreeMusic(music);
        music = NULL;
    }
}

void CDAudio_Pause(void)
{
    Mix_PauseMusic();
}

void CDAudio_Resume(void)
{
    Mix_ResumeMusic();
}

void CDAudio_Shutdown(void)
{
    CDAudio_Stop();
    Mix_CloseAudio();
}

void CDAudio_Update(void)
{
    Mix_VolumeMusic((int)(bgmvolume.value * MIX_MAX_VOLUME));
}
