
#include "quakedef.h"
#include <SDL2/SDL_audio.h>
#include <SDL2/SDL_mixer.h>
#include <SDL2/SDL_endian.h>
#include <stdio.h>

static dma_t the_shm;
static int32_t snd_inited;

extern int32_t desired_speed;
extern int32_t desired_bits;

static void paint_audio(void *unused, Uint8 *stream, int32_t len)
{
    if (shm)
    {
        shm->buffer = stream;
        shm->samplepos += len / (shm->samplebits / 8) / 2;
        S_PaintChannels(shm->samplepos);
    }
}

bool SNDDMA_Init(void)
{
    SDL_AudioSpec desired, obtained;

    snd_inited = 0;

    // Set up the desired format
    desired.freq = desired_speed;
    switch (desired_bits)
    {
    case 8:
        desired.format = AUDIO_U8;
        break;
    case 16:
        desired.format = AUDIO_S16LSB;
        break;
    default:
        Con_Printf("Unknown number of audio bits: %d\n", desired_bits);
        return 0;
    }
    desired.channels = 2;
    desired.samples = 512;
    desired.callback = paint_audio;

    // Open the audio device
    if (SDL_OpenAudio(&desired, &obtained) < 0)
    {
        Con_Printf("Couldn't open SDL audio: %s\n", SDL_GetError());
        return 0;
    }

    // Make sure we can support the audio format
    switch (obtained.format)
    {
    case AUDIO_U8:
        break;
    case AUDIO_S16LSB:
    case AUDIO_S16MSB:
        if ((obtained.format == AUDIO_S16LSB) != 0)
            break;
    default:
        SDL_CloseAudio();
        if (SDL_OpenAudio(&desired, NULL) < 0)
        {
            Con_Printf("Couldn't open SDL audio: %s\n", SDL_GetError());
            return 0;
        }
        memcpy(&obtained, &desired, sizeof(desired));
        break;
    }
    SDL_PauseAudio(0);

    // Fill the audio DMA information block
    shm = &the_shm;
    shm->splitbuffer = 0;
    shm->samplebits = (obtained.format & 0xFF);
    shm->speed = obtained.freq;
    shm->channels = obtained.channels;
    shm->samples = obtained.samples * shm->channels;
    shm->samplepos = 0;
    shm->submission_chunk = 1;
    shm->buffer = NULL;

    if (Mix_OpenAudio(desired.freq, AUDIO_S16SYS, 2, 1024) == -1) {
        Con_Printf("Could not initialize mixer: %s\n", Mix_GetError());
        return 0;
    }

    Mix_AllocateChannels(16);

    snd_inited = 1;
    return 1;
}

int32_t SNDDMA_GetDMAPos(void)
{
    return shm->samplepos;
}

void SNDDMA_Shutdown(void)
{
    if (snd_inited)
    {
        SDL_CloseAudio();
        snd_inited = 0;
    }
}

void SNDDMA_Submit(void)
{
}
