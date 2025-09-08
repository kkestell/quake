#include "raylib.h"
#include "quakedef.h"
#include "d_local.h"

viddef_t vid;
uint16_t d_8to16table[256];

#define BASEWIDTH (640)
#define BASEHEIGHT (400)

static Image image8bpp = {0};
static Image image32bpp = {0};
static Texture2D screenTexture = {0};
static Color palette[256];

static bool mouse_avail;
static float mouse_x, mouse_y;
static int32_t mouse_oldbuttonstate = 0;

void (*vid_menudrawfn)(void) = NULL;
void (*vid_menukeyfn)(int32_t key) = NULL;

void VID_SetPalette(unsigned char *palette_data)
{
    for (int i = 0; i < 256; ++i)
    {
        palette[i].r = *palette_data++;
        palette[i].g = *palette_data++;
        palette[i].b = *palette_data++;
        palette[i].a = 255;
    }
}

void VID_ShiftPalette(unsigned char *palette)
{
    VID_SetPalette(palette);
}

void VID_Init(unsigned char *palette)
{
    int pnum, chunk;
    uint8_t *cache;
    int cachesize;

    vid.width = BASEWIDTH;
    vid.height = BASEHEIGHT;
    vid.maxwarpwidth = WARP_WIDTH;
    vid.maxwarpheight = WARP_HEIGHT;
    if ((pnum = COM_CheckParm("-winsize")))
    {
        if (pnum >= com_argc - 2)
            Sys_Error("VID: -winsize <width> <height>\n");
        vid.width = (int32_t)strtol(com_argv[pnum + 1], NULL, 0);
        vid.height = (int32_t)strtol(com_argv[pnum + 2], NULL, 0);
        if (!vid.width || !vid.height)
            Sys_Error("VID: Bad window width/height\n");
    }

    int window_width = BASEWIDTH * 2;
    int window_height = BASEHEIGHT * 2;

    if (COM_CheckParm("-fullscreen"))
    {
        SetConfigFlags(FLAG_FULLSCREEN_MODE);
        window_width = GetMonitorWidth(0);
        window_height = GetMonitorHeight(0);
    }

    InitWindow(window_width, window_height, "Quake");
    // SetTargetFPS(60);

    image8bpp.data = MemAlloc(vid.width * vid.height);
    image8bpp.width = vid.width;
    image8bpp.height = vid.height;
    image8bpp.format = PIXELFORMAT_UNCOMPRESSED_GRAYSCALE;
    image8bpp.mipmaps = 1;

    image32bpp = GenImageColor(vid.width, vid.height, BLACK);
    screenTexture = LoadTextureFromImage(image32bpp);

    VID_SetPalette(palette);

    vid.conwidth = vid.width;
    vid.conheight = vid.height;
    vid.aspect = ((float)vid.height / (float)vid.width) * (320.0 / 240.0);
    vid.numpages = 1;
    vid.colormap = host_colormap;
    vid.fullbright = 256 -  (*((int *)vid.colormap + 2048));
    vid.buffer = image8bpp.data;
    vid.rowbytes = vid.width;
    vid.conbuffer = vid.buffer;
    vid.conrowbytes = vid.rowbytes;
    vid.direct = 0;

    chunk = vid.width * vid.height * sizeof(*d_pzbuffer);
    cachesize = D_SurfaceCacheForRes(vid.width, vid.height);
    chunk += cachesize;
    d_pzbuffer = Hunk_HighAllocName(chunk, "video");
    if (d_pzbuffer == NULL)
        Sys_Error("Not enough memory for video mode\n");

    cache = (uint8_t *)d_pzbuffer + vid.width * vid.height * sizeof(*d_pzbuffer);
    D_InitCaches(cache, cachesize);

    HideCursor();
    DisableCursor();
}

void VID_Shutdown(void)
{
    UnloadTexture(screenTexture);
    UnloadImage(image32bpp);
    MemFree(image8bpp.data);
    CloseWindow();
}

void VID_Update(vrect_t *rects)
{
    unsigned char *src = (unsigned char *)vid.buffer;
    Color *dest = (Color *)image32bpp.data;

    for (int i = 0; i < vid.width * vid.height; i++)
    {
        dest[i] = palette[src[i]];
    }

    UpdateTexture(screenTexture, dest);

    BeginDrawing();
    ClearBackground(BLACK);
    DrawTexturePro(screenTexture, (Rectangle){0, 0, vid.width, vid.height},
                   (Rectangle){0, 0, GetScreenWidth(), GetScreenHeight()}, (Vector2){0, 0}, 0, WHITE);
    EndDrawing();
}

static int TranslateKey(int key)
{
    switch (key)
    {
    case KEY_DELETE: return K_DEL;
    case KEY_BACKSPACE: return K_BACKSPACE;
    case KEY_F1: return K_F1;
    case KEY_F2: return K_F2;
    case KEY_F3: return K_F3;
    case KEY_F4: return K_F4;
    case KEY_F5: return K_F5;
    case KEY_F6: return K_F6;
    case KEY_F7: return K_F7;
    case KEY_F8: return K_F8;
    case KEY_F9: return K_F9;
    case KEY_F10: return K_F10;
    case KEY_F11: return K_F11;
    case KEY_F12: return K_F12;
    case KEY_PAUSE: return K_PAUSE;
    case KEY_UP: return K_UPARROW;
    case KEY_DOWN: return K_DOWNARROW;
    case KEY_RIGHT: return K_RIGHTARROW;
    case KEY_LEFT: return K_LEFTARROW;
    case KEY_INSERT: return K_INS;
    case KEY_HOME: return K_HOME;
    case KEY_END: return K_END;
    case KEY_PAGE_UP: return K_PGUP;
    case KEY_PAGE_DOWN: return K_PGDN;
    case KEY_LEFT_SHIFT: case KEY_RIGHT_SHIFT: return K_SHIFT;
    case KEY_LEFT_CONTROL: case KEY_RIGHT_CONTROL: return K_CTRL;
    case KEY_LEFT_ALT: case KEY_RIGHT_ALT: return K_ALT;
    case KEY_KP_DIVIDE: return '/';
    case KEY_KP_MULTIPLY: return '*';
    case KEY_KP_SUBTRACT: return '-';
    case KEY_KP_ADD: return '+';
    case KEY_KP_ENTER: return K_ENTER;
    case KEY_KP_EQUAL: return '=';
    case KEY_ENTER: return K_ENTER;
    default: return tolower(key);
    }
}

void Sys_SendKeyEvents(void)
{
    if (WindowShouldClose())
    {
        CL_Disconnect();
        Host_ShutdownServer(false);
        Sys_Quit();
    }

    for (int i = 32; i < 349; i++)
    {
        if (IsKeyPressed(i)) Key_Event(TranslateKey(i), true);
        if (IsKeyReleased(i)) Key_Event(TranslateKey(i), false);
    }

    Vector2 delta = GetMouseDelta();
    mouse_x = delta.x * 10;
    mouse_y = delta.y * 10;
}

void IN_Init(void)
{
    if (COM_CheckParm("-nomouse"))
        return;
    mouse_x = mouse_y = 0.0;
    mouse_avail = true;
}

void IN_Shutdown(void)
{
    mouse_avail = false;
}

void IN_Commands(void)
{
    int mouse_buttonstate = 0;

    if (!mouse_avail) return;

    if (IsMouseButtonDown(MOUSE_BUTTON_LEFT)) mouse_buttonstate |= 1;
    if (IsMouseButtonDown(MOUSE_BUTTON_RIGHT)) mouse_buttonstate |= 2;
    if (IsMouseButtonDown(MOUSE_BUTTON_MIDDLE)) mouse_buttonstate |= 4;

    int quake_buttonstate = (mouse_buttonstate & ~0x06) | ((mouse_buttonstate & 0x02) << 1) | ((mouse_buttonstate & 0x04) >> 1);

    for (int i = 0; i < 3; i++)
    {
        if ((quake_buttonstate & (1 << i)) && !(mouse_oldbuttonstate & (1 << i)))
            Key_Event(K_MOUSE1 + i, true);
        if (!(quake_buttonstate & (1 << i)) && (mouse_oldbuttonstate & (1 << i)))
            Key_Event(K_MOUSE1 + i, false);
    }
    mouse_oldbuttonstate = quake_buttonstate;
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
    mouse_x = 0.0;
    mouse_y = 0.0;
}

char *Sys_ConsoleInput(void)
{
    return NULL;
}

void VID_LockBuffer() {}
void VID_UnlockBuffer() {}
void VID_HandlePause(bool pause) {}