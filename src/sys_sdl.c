/* -*- Mode: C; tab-width: 4 -*- */

#include <errno.h>
#include <signal.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/time.h>

#include "quakedef.h"

bool isDedicated;

char *basedir = ".";

cvar_t sys_nostdout = {"sys_nostdout", "0"};

// =======================================================================
// General routines
// =======================================================================

void Sys_DebugNumber(int32_t y, int32_t val)
{
}

void Sys_Printf(char *fmt, ...)
{
    va_list argptr;
    char text[1024];

    va_start(argptr, fmt);
    vsprintf(text, fmt, argptr);
    va_end(argptr);
    fprintf(stderr, "%s", text);

    // Con_Print (text);
}

void Sys_Quit(void)
{
    Host_Shutdown();
    exit(0);
}

void Sys_Init(void)
{
}

/*
================
Sys_LowFPPrecision
================
*/
void Sys_LowFPPrecision(void)
{
    // causes weird problems on Nextstep
}

/*
================
Sys_HighFPPrecision
================
*/
void Sys_HighFPPrecision(void)
{
    // causes weird problems on Nextstep
}

void Sys_Error(char *error, ...)
{
    va_list argptr;
    char string[1024];

    va_start(argptr, error);
    vsprintf(string, error, argptr);
    va_end(argptr);
    fprintf(stderr, "Error: %s\n", string);

    Host_Shutdown();
    exit(1);
}

void Sys_Warn(char *warning, ...)
{
    va_list argptr;
    char string[1024];

    va_start(argptr, warning);
    vsprintf(string, warning, argptr);
    va_end(argptr);
    fprintf(stderr, "Warning: %s", string);
}

/*
===============================================================================

FILE IO

===============================================================================
*/

#define MAX_HANDLES 10
FILE *sys_handles[MAX_HANDLES];

int32_t findhandle(void)
{
    int32_t i;

    for (i = 1; i < MAX_HANDLES; i++)
        if (!sys_handles[i])
            return i;
    Sys_Error("out of handles");
    return -1;
}

/*
================
Qfilelength
================
*/
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
    int32_t i;

    i = findhandle();

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
    int32_t i;

    i = findhandle();

    f = fopen(path, "wb");
    if (!f)
        Sys_Error("Error opening %s: %s", path, strerror(errno));
    sys_handles[i] = f;

    return i;
}

void Sys_FileClose(int32_t handle)
{
    if (handle >= 0)
    {
        fclose(sys_handles[handle]);
        sys_handles[handle] = NULL;
    }
}

void Sys_FileSeek(int32_t handle, int32_t position)
{
    if (handle >= 0)
    {
        fseek(sys_handles[handle], position, SEEK_SET);
    }
}

int32_t Sys_FileRead(int32_t handle, void *dst, int32_t count)
{
    char *data;
    int32_t size, done;

    size = 0;
    if (handle >= 0)
    {
        data = dst;
        while (count > 0)
        {
            done = fread(data, 1, count, sys_handles[handle]);
            if (done == 0)
            {
                break;
            }
            data += done;
            count -= done;
            size += done;
        }
    }
    return size;
}

int32_t Sys_FileWrite(int32_t handle, void *src, int32_t count)
{
    char *data;
    int32_t size, done;

    size = 0;
    if (handle >= 0)
    {
        data = src;
        while (count > 0)
        {
            done = fread(data, 1, count, sys_handles[handle]);
            if (done == 0)
            {
                break;
            }
            data += done;
            count -= done;
            size += done;
        }
    }
    return size;
}

int32_t Sys_FileTime(char *path)
{
    FILE *f;

    f = fopen(path, "rb");
    if (f)
    {
        fclose(f);
        return 1;
    }

    return -1;
}

void Sys_mkdir(char *path)
{
    mkdir(path, 0777);
}

void Sys_DebugLog(char *file, char *fmt, ...)
{
    va_list argptr;
    static char data[1024];
    FILE *fp;

    va_start(argptr, fmt);
    vsprintf(data, fmt, argptr);
    va_end(argptr);
    fp = fopen(file, "a");
    fwrite(data, strlen(data), 1, fp);
    fclose(fp);
}

double Sys_FloatTime(void)
{
    struct timeval tp;
    struct timezone tzp;
    static int32_t secbase;

    gettimeofday(&tp, &tzp);

    if (!secbase)
    {
        secbase = tp.tv_sec;
        return tp.tv_usec / 1000000.0;
    }

    return (tp.tv_sec - secbase) + tp.tv_usec / 1000000.0;
}

// =======================================================================
// Sleeps for microseconds
// =======================================================================

void floating_point_exception_handler(int32_t whatever)
{
    Sys_Warn("floating point exception\n");
    signal(SIGFPE, floating_point_exception_handler);
}

int32_t main(int32_t argc, char *argv[])
{
    double time, oldtime, newtime;
    quakeparms_t parms;
    static int32_t frame;

    signal(SIGFPE, SIG_IGN);

    parms.memsize = 32 * 1024 * 1024;
    parms.membase = malloc(parms.memsize);
    parms.basedir = basedir;
    // Disable cache, else it looks in the cache for config.cfg.
    parms.cachedir = NULL;

    COM_InitArgv(argc, argv);
    parms.argc = com_argc;
    parms.argv = com_argv;

    Sys_Init();

    Host_Init(&parms);

    Cvar_RegisterVariable(&sys_nostdout);

    oldtime = Sys_FloatTime() - 0.1;
    while (1)
    {
        newtime = Sys_FloatTime();
        time = newtime - oldtime;

        if (time > sys_ticrate.value * 2)
            oldtime = newtime;
        else
            oldtime += time;

        Host_Frame((float)time);
    }
}
