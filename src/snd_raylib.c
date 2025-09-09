// snd_raylib.c
#include <raylib.h>
#include "quakedef.h"

static dma_t the_shm;
static int32_t snd_inited;
static AudioStream stream;
static void *mixbuffer = NULL;

extern int32_t desired_speed;
extern int32_t desired_bits;

bool SNDDMA_Init(void)
{
    int buffer_samples = 1024;

    snd_inited = 0;

    if (desired_bits != 8 && desired_bits != 16)
    {
        Con_Printf("Unknown number of audio bits: %d\n", desired_bits);
        return false;
    }

    InitAudioDevice();
    SetAudioStreamBufferSizeDefault(buffer_samples);

    stream = LoadAudioStream(desired_speed, desired_bits, 2);
    if (!IsAudioStreamValid(stream))
    {
        Con_Printf("Could not create audio stream\n");
        return false;
    }

    PlayAudioStream(stream);

    mixbuffer = MemAlloc(buffer_samples * 2 * (desired_bits / 8));

    shm = &the_shm;
    shm->splitbuffer = 0;
    shm->samplebits = desired_bits;
    shm->speed = desired_speed;
    shm->channels = 2;
    shm->samples = buffer_samples * shm->channels;
    shm->samplepos = 0;
    shm->submission_chunk = 1;
    shm->buffer = mixbuffer;

    snd_inited = true;
    return true;
}

int32_t SNDDMA_GetDMAPos(void)
{
    return shm->samplepos;
}

void SNDDMA_Shutdown(void)
{
    if (snd_inited)
    {
        MemFree(mixbuffer);
        UnloadAudioStream(stream);
        CloseAudioDevice();
        snd_inited = 0;
    }
}

void SNDDMA_Submit(void)
{
    if (!snd_inited || !IsAudioStreamProcessed(stream))
    {
        return;
    }

    int sample_frames = shm->samples / shm->channels;
    shm->samplepos += sample_frames;

    S_PaintChannels(shm->samplepos);
    UpdateAudioStream(stream, shm->buffer, sample_frames);
}
