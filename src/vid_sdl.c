// vid_sdl.h -- sdl video driver

#include <SDL2/SDL.h>
#include "quakedef.h"
#include "d_local.h"

viddef_t vid; // global video state
uint16_t d_8to16table[256];

#define BASEWIDTH (640)
#define BASEHEIGHT (400)

static SDL_Window *window = NULL;
static SDL_Surface *screen = NULL;
static SDL_Surface *surface8bpp = NULL;
static SDL_Surface *surface32bpp = NULL;

static bool mouse_avail;
static float mouse_x, mouse_y;
static int32_t mouse_oldbuttonstate = 0;

// No support for option menus
void (*vid_menudrawfn)(void) = NULL;
void (*vid_menukeyfn)(int32_t key) = NULL;

void VID_SetPalette(unsigned char *palette)
{
    int32_t i;
    SDL_Color colors[256];

    for (i = 0; i < 256; ++i)
    {
        colors[i].r = *palette++;
        colors[i].g = *palette++;
        colors[i].b = *palette++;
    }

    SDL_SetPaletteColors(surface8bpp->format->palette, colors, 0, 256);
}

void VID_ShiftPalette(unsigned char *palette)
{
    VID_SetPalette(palette);
}

void VID_Init(unsigned char *palette)
{
    int32_t pnum, chunk;
    uint8_t *cache;
    int32_t cachesize;
    Uint8 video_bpp;
    Uint16 video_w, video_h;
    Uint32 flags;

    // Load the SDL library
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO) < 0)
        Sys_Error("VID: Couldn't load SDL: %s", SDL_GetError());

    // Set up display mode (width and height)
    vid.width = BASEWIDTH;
    vid.height = BASEHEIGHT;
    vid.maxwarpwidth = WARP_WIDTH;
    vid.maxwarpheight = WARP_HEIGHT;
    if ((pnum = COM_CheckParm("-winsize")))
    {
        if (pnum >= com_argc - 2)
            Sys_Error("VID: -winsize <width> <height>\n");
        vid.width = Q_atoi(com_argv[pnum + 1]);
        vid.height = Q_atoi(com_argv[pnum + 2]);
        if (!vid.width || !vid.height)
            Sys_Error("VID: Bad window width/height\n");
    }

    int32_t window_width = BASEWIDTH * 2;
    int32_t window_height = BASEHEIGHT * 2;
    Uint32 window_flags = SDL_WINDOW_SHOWN;

    if (COM_CheckParm("-fullscreen"))
    {
        window_width = 0;
        window_height = 0;
        window_flags |= SDL_WINDOW_FULLSCREEN_DESKTOP;
    }

    // Initialize display
    if (!(window = SDL_CreateWindow("Quake", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, window_width,
                                    window_height, window_flags)))
    {
        Sys_Error("VID: Couldn't set video mode: %s\n", SDL_GetError());
    }

    surface8bpp = SDL_CreateRGBSurface(SDL_SWSURFACE, vid.width, vid.height, 8, 0, 0, 0, 0);
    // SDL_SetSurfaceBlendMode(surface, SDL_BLENDMODE_NONE);
    screen = SDL_GetWindowSurface(window);
    // SDL_SetSurfaceBlendMode(screen, SDL_BLENDMODE_NONE);

    SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "linear");

    surface32bpp =
        SDL_CreateRGBSurface(surface8bpp->flags, vid.width, vid.height, 32, surface8bpp->format->Rmask,
                             surface8bpp->format->Gmask, surface8bpp->format->Bmask, surface8bpp->format->Amask);

    VID_SetPalette(palette);

    // now we know everything we need to know about the buffer
    vid.conwidth = vid.width;
    vid.conheight = vid.height;
    vid.aspect = ((float)vid.height / (float)vid.width) * (320.0 / 240.0);
    vid.numpages = 1;
    vid.colormap = host_colormap;
    vid.fullbright = 256 - LittleLong(*((int32_t *)vid.colormap + 2048));
    vid.buffer = surface8bpp->pixels;
    vid.rowbytes = surface8bpp->pitch;
    vid.conbuffer = vid.buffer;
    vid.conrowbytes = vid.rowbytes;
    vid.direct = 0;

    // allocate z buffer and surface cache
    chunk = vid.width * vid.height * sizeof(*d_pzbuffer);
    cachesize = D_SurfaceCacheForRes(vid.width, vid.height);
    chunk += cachesize;
    d_pzbuffer = Hunk_HighAllocName(chunk, "video");
    if (d_pzbuffer == NULL)
        Sys_Error("Not enough memory for video mode\n");

    // initialize the cache memory
    cache = (uint8_t *)d_pzbuffer + vid.width * vid.height * sizeof(*d_pzbuffer);
    D_InitCaches(cache, cachesize);

    // initialize the mouse
    SDL_ShowCursor(0);
}

void VID_Shutdown(void)
{
    SDL_Quit();
}

void VID_Update(vrect_t *rects)
{
    SDL_BlitSurface(surface8bpp, NULL, surface32bpp, NULL);
    SDL_BlitScaled(surface32bpp, NULL, screen, NULL);
    SDL_UpdateWindowSurface(window);
}

/*
================
Sys_SendKeyEvents
================
*/

void Sys_SendKeyEvents(void)
{
    SDL_Event event;
    int32_t sym, state;
    int32_t modstate;

    while (SDL_PollEvent(&event))
    {
        switch (event.type)
        {

        case SDL_KEYDOWN:
        case SDL_KEYUP:
            sym = event.key.keysym.sym;
            state = event.key.state;
            modstate = SDL_GetModState();
            switch (sym)
            {
            case SDLK_DELETE:
                sym = K_DEL;
                break;
            case SDLK_BACKSPACE:
                sym = K_BACKSPACE;
                break;
            case SDLK_F1:
                sym = K_F1;
                break;
            case SDLK_F2:
                sym = K_F2;
                break;
            case SDLK_F3:
                sym = K_F3;
                break;
            case SDLK_F4:
                sym = K_F4;
                break;
            case SDLK_F5:
                sym = K_F5;
                break;
            case SDLK_F6:
                sym = K_F6;
                break;
            case SDLK_F7:
                sym = K_F7;
                break;
            case SDLK_F8:
                sym = K_F8;
                break;
            case SDLK_F9:
                sym = K_F9;
                break;
            case SDLK_F10:
                sym = K_F10;
                break;
            case SDLK_F11:
                sym = K_F11;
                break;
            case SDLK_F12:
                sym = K_F12;
                break;
            case SDLK_PAUSE:
                sym = K_PAUSE;
                break;
            case SDLK_UP:
                sym = K_UPARROW;
                break;
            case SDLK_DOWN:
                sym = K_DOWNARROW;
                break;
            case SDLK_RIGHT:
                sym = K_RIGHTARROW;
                break;
            case SDLK_LEFT:
                sym = K_LEFTARROW;
                break;
            case SDLK_INSERT:
                sym = K_INS;
                break;
            case SDLK_HOME:
                sym = K_HOME;
                break;
            case SDLK_END:
                sym = K_END;
                break;
            case SDLK_PAGEUP:
                sym = K_PGUP;
                break;
            case SDLK_PAGEDOWN:
                sym = K_PGDN;
                break;
            case SDLK_RSHIFT:
            case SDLK_LSHIFT:
                sym = K_SHIFT;
                break;
            case SDLK_RCTRL:
            case SDLK_LCTRL:
                sym = K_CTRL;
                break;
            case SDLK_RALT:
            case SDLK_LALT:
                sym = K_ALT;
                break;
            case SDLK_KP_DIVIDE:
                sym = SDLK_SLASH;
                break;
            case SDLK_KP_MULTIPLY:
                sym = SDLK_ASTERISK;
                break;
            case SDLK_KP_MINUS:
                sym = SDLK_MINUS;
                break;
            case SDLK_KP_PLUS:
                sym = SDLK_PLUS;
                break;
            case SDLK_KP_ENTER:
                sym = SDLK_RETURN;
                break;
            case SDLK_KP_EQUALS:
                sym = SDLK_EQUALS;
                break;
            }
            // If we're not directly handled and still above 255
            // just force it to 0
            if (sym > 255)
                sym = 0;
            Key_Event(sym, state);
            break;

        case SDL_MOUSEMOTION:
            if ((event.motion.x != (vid.width / 2)) || (event.motion.y != (vid.height / 2)))
            {
                mouse_x = event.motion.xrel * 10;
                mouse_y = event.motion.yrel * 10;
                if ((event.motion.x < ((vid.width / 2) - (vid.width / 4))) ||
                    (event.motion.x > ((vid.width / 2) + (vid.width / 4))) ||
                    (event.motion.y < ((vid.height / 2) - (vid.height / 4))) ||
                    (event.motion.y > ((vid.height / 2) + (vid.height / 4))))
                {
                    SDL_WarpMouseInWindow(window, vid.width / 2, vid.height / 2);
                }
            }
            break;

        case SDL_QUIT:
            CL_Disconnect();
            Host_ShutdownServer(false);
            Sys_Quit();
            break;
        default:
            break;
        }
    }
}

void IN_Init(void)
{
    if (COM_CheckParm("-nomouse"))
        return;
    mouse_x = mouse_y = 0.0;
    mouse_avail = 1;
}

void IN_Shutdown(void)
{
    mouse_avail = 0;
}

void IN_Commands(void)
{
    int32_t i;
    int32_t mouse_buttonstate;

    if (!mouse_avail)
        return;

    i = SDL_GetMouseState(NULL, NULL);
    /* Quake swaps the second and third buttons */
    mouse_buttonstate = (i & ~0x06) | ((i & 0x02) << 1) | ((i & 0x04) >> 1);
    for (i = 0; i < 3; i++)
    {
        if ((mouse_buttonstate & (1 << i)) && !(mouse_oldbuttonstate & (1 << i)))
            Key_Event(K_MOUSE1 + i, true);

        if (!(mouse_buttonstate & (1 << i)) && (mouse_oldbuttonstate & (1 << i)))
            Key_Event(K_MOUSE1 + i, false);
    }
    mouse_oldbuttonstate = mouse_buttonstate;
}

void IN_Move(usercmd_t *cmd)
{
    if (!mouse_avail)
        return;

    mouse_x *= sensitivity.value;
    mouse_y *= sensitivity.value;

    if ((in_strafe.state & 1) || (lookstrafe.value && (in_mlook.state & 1)))
        cmd->sidemove += m_side.value * mouse_x;
    else
        cl.viewangles[YAW] -= m_yaw.value * mouse_x;
    if (in_mlook.state & 1)
        V_StopPitchDrift();

    if ((in_mlook.state & 1) && !(in_strafe.state & 1))
    {
        cl.viewangles[PITCH] += m_pitch.value * mouse_y;
        if (cl.viewangles[PITCH] > 80)
            cl.viewangles[PITCH] = 80;
        if (cl.viewangles[PITCH] < -70)
            cl.viewangles[PITCH] = -70;
    }
    else
    {
        if ((in_strafe.state & 1) && noclip_anglehack)
            cmd->upmove -= m_forward.value * mouse_y;
        else
            cmd->forwardmove -= m_forward.value * mouse_y;
    }
    mouse_x = mouse_y = 0.0;
}

/*
================
Sys_ConsoleInput
================
*/
char *Sys_ConsoleInput(void)
{
    return 0;
}

void VID_LockBuffer()
{
}

void VID_UnlockBuffer()
{
}

void VID_HandlePause(bool pause)
{
}
