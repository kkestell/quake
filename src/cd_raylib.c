// cd_raylib.c
#include "quakedef.h"
#include "raylib.h"

static Music music = (Music){ 0 };

int32_t CDAudio_Init(void)
{
    return 0;
}

void CDAudio_Play(uint8_t track, bool looping)
{
    if (IsMusicValid(music))
    {
        UnloadMusicStream(music);
    }

    char filename[256];
    snprintf(filename, sizeof(filename), "id1/music/%d.mp3", track);
    
    music = LoadMusicStream(filename);

    if (!IsMusicValid(music))
    {
        Con_Printf("Could not load music file: %s\n", filename);
        return;
    }

    music.looping = looping;
    PlayMusicStream(music);
}

void CDAudio_Stop(void)
{
    if (IsMusicValid(music))
    {
        StopMusicStream(music);
        UnloadMusicStream(music);
        music = (Music){ 0 };
    }
}

void CDAudio_Pause(void)
{
    if (IsMusicValid(music))
    {
        PauseMusicStream(music);
    }
}

void CDAudio_Resume(void)
{
    if (IsMusicValid(music))
    {
        ResumeMusicStream(music);
    }
}

void CDAudio_Shutdown(void)
{
    CDAudio_Stop();
}

void CDAudio_Update(void)
{
    if (IsMusicValid(music))
    {
        UpdateMusicStream(music);
        SetMusicVolume(music, bgmvolume.value);
    }
}
