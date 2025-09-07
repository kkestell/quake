#include <errno.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>

#include "quakedef.h"
#include "raylib.h"

bool isDedicated;
char *basedir = ".";
cvar_t sys_nostdout = {"sys_nostdout", "0"};

void Sys_DebugNumber(int32_t y, int32_t val) {}

void Sys_Printf(char *fmt, ...)
{
    va_list argptr;
    char text[1024];

    va_start(argptr, fmt);
    vsnprintf(text, sizeof(text), fmt, argptr);
    va_end(argptr);

    TraceLog(LOG_INFO, text);
}

void Sys_Quit(void)
{
    Host_Shutdown();
    exit(0);
}

void Sys_Init(void) {}

void Sys_LowFPPrecision(void) {}

void Sys_HighFPPrecision(void) {}

void Sys_Error(char *error, ...)
{
    va_list argptr;
    char string[1024];

    va_start(argptr, error);
    vsnprintf(string, sizeof(string), error, argptr);
    va_end(argptr);
    TraceLog(LOG_ERROR, string);

    Host_Shutdown();
    exit(1);
}

void Sys_Warn(char *warning, ...)
{
    va_list argptr;
    char string[1024];

    va_start(argptr, warning);
    vsnprintf(string, sizeof(string), warning, argptr);
    va_end(argptr);
    TraceLog(LOG_WARNING, string);
}

#define MAX_HANDLES 10
FILE *sys_handles[MAX_HANDLES];

int32_t findhandle(void)
{
    for (int i = 1; i < MAX_HANDLES; i++)
    {
        if (!sys_handles[i])
            return i;
    }
    Sys_Error("out of handles");
    return -1;
}

static int32_t Qfilelength(FILE *f)
{
    int32_t pos;
    int32_t end;

    pos = ftell(f);
    fseek(f, 0, SEEK_END);
    end = ftell(f);
    fseek(f, pos, SEEK_SET);

    return end;
}

int32_t Sys_FileOpenRead(char *path, int32_t *hndl)
{
    FILE *f;
    int32_t i = findhandle();

    f = fopen(path, "rb");
    if (!f)
    {
        *hndl = -1;
        return -1;
    }
    sys_handles[i] = f;
    *hndl = i;

    return Qfilelength(f);
}

int32_t Sys_FileOpenWrite(char *path)
{
    FILE *f;
    int32_t i = findhandle();

    f = fopen(path, "wb");
    if (!f)
        Sys_Error("Error opening %s: %s", path, strerror(errno));
    sys_handles[i] = f;

    return i;
}

void Sys_FileClose(int32_t handle)
{
    if (handle >= 0 && handle < MAX_HANDLES && sys_handles[handle])
    {
        fclose(sys_handles[handle]);
        sys_handles[handle] = NULL;
    }
}

void Sys_FileSeek(int32_t handle, int32_t position)
{
    if (handle >= 0 && handle < MAX_HANDLES && sys_handles[handle])
    {
        fseek(sys_handles[handle], position, SEEK_SET);
    }
}

int32_t Sys_FileRead(int32_t handle, void *dst, int32_t count)
{
    if (handle < 0 || handle >= MAX_HANDLES || !sys_handles[handle])
    {
        return 0;
    }
    return fread(dst, 1, count, sys_handles[handle]);
}

int32_t Sys_FileWrite(int32_t handle, void *src, int32_t count)
{
    if (handle < 0 || handle >= MAX_HANDLES || !sys_handles[handle])
    {
        return 0;
    }
    return fwrite(src, 1, count, sys_handles[handle]);
}

int32_t Sys_FileTime(char *path)
{
    return FileExists(path) ? 1 : -1;
}

void Sys_mkdir(char *path)
{
    mkdir(path, 0777);
}

void Sys_DebugLog(char *file, char *fmt, ...)
{
    va_list argptr;
    char data[1024];
    FILE *fp;

    va_start(argptr, fmt);
    vsnprintf(data, sizeof(data), fmt, argptr);
    va_end(argptr);

    fp = fopen(file, "a");
    if (fp)
    {
        fwrite(data, strlen(data), 1, fp);
        fclose(fp);
    }
}

double Sys_FloatTime(void)
{
    return GetTime();
}

int main(int argc, char *argv[])
{
    quakeparms_t parms;

    parms.memsize = 32 * 1024 * 1024;
    parms.membase = malloc(parms.memsize);
    parms.basedir = basedir;
    parms.cachedir = NULL;

    COM_InitArgv(argc, argv);
    parms.argc = com_argc;
    parms.argv = com_argv;

    Sys_Init();
    Host_Init(&parms);
    Cvar_RegisterVariable(&sys_nostdout);

    while (!WindowShouldClose())
    {
        Host_Frame(GetFrameTime());
    }

    Host_Shutdown();
    return 0;
}