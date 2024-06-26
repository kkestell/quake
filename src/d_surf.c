// d_surf.c: rasterization driver surface heap manager

#include "quakedef.h"
#include "d_local.h"
#include "r_local.h"

float surfscale;
bool r_cache_thrash; // set if surface cache is thrashing

int32_t sc_size;
surfcache_t *sc_rover, *sc_base;

#define GUARDSIZE 4

int32_t D_SurfaceCacheForRes(int32_t width, int32_t height)
{
    int32_t size, pix;

    if (COM_CheckParm("-surfcachesize"))
    {
        size = Q_atoi(com_argv[COM_CheckParm("-surfcachesize") + 1]) * 1024;
        return size;
    }

    size = SURFCACHE_SIZE_AT_320X200;

    pix = width * height;
    if (pix > 64000)
        size += (pix - 64000) * 3;

    return size;
}

void D_CheckCacheGuard(void)
{
    uint8_t *s;
    int32_t i;

    s = (uint8_t *)sc_base + sc_size;
    for (i = 0; i < GUARDSIZE; i++)
        if (s[i] != (uint8_t)i)
            Sys_Error("D_CheckCacheGuard: failed");
}

void D_ClearCacheGuard(void)
{
    uint8_t *s;
    int32_t i;

    s = (uint8_t *)sc_base + sc_size;
    for (i = 0; i < GUARDSIZE; i++)
        s[i] = (uint8_t)i;
}

/*
================
D_InitCaches

================
*/
void D_InitCaches(void *buffer, int32_t size)
{

    if (!msg_suppress_1)
        Con_Printf("%ik surface cache\n", size / 1024);

    sc_size = size - GUARDSIZE;
    sc_base = (surfcache_t *)buffer;
    sc_rover = sc_base;

    sc_base->next = NULL;
    sc_base->owner = NULL;
    sc_base->size = sc_size;

    D_ClearCacheGuard();
}

/*
==================
D_FlushCaches
==================
*/
void D_FlushCaches(void)
{
    surfcache_t *c;

    if (!sc_base)
        return;

    for (c = sc_base; c; c = c->next)
    {
        if (c->owner)
            *c->owner = NULL;
    }

    sc_rover = sc_base;
    sc_base->next = NULL;
    sc_base->owner = NULL;
    sc_base->size = sc_size;
}

/*
=================
D_SCAlloc
=================
*/
surfcache_t *D_SCAlloc(int32_t width, int32_t size)
{
    surfcache_t *new;
    bool wrapped_this_time;

    if ((width < 0) || (width > 256))
        Sys_Error("D_SCAlloc: bad cache width %d\n", width);

    if ((size <= 0) || (size > 0x10000))
        Sys_Error("D_SCAlloc: bad cache size %d\n", size);

    size = (int32_t) & ((surfcache_t *)0)->data[size];
    size = (size + 3) & ~3;
    if (size > sc_size)
        Sys_Error("D_SCAlloc: %i > cache size", size);

    // if there is not size bytes after the rover, reset to the start
    wrapped_this_time = false;

    if (!sc_rover || (uint8_t *)sc_rover - (uint8_t *)sc_base > sc_size - size)
    {
        if (sc_rover)
        {
            wrapped_this_time = true;
        }
        sc_rover = sc_base;
    }

    // colect and free surfcache_t blocks until the rover block is large enough
    new = sc_rover;
    if (sc_rover->owner)
        *sc_rover->owner = NULL;

    while (new->size < size)
    {
        // free another
        sc_rover = sc_rover->next;
        if (!sc_rover)
            Sys_Error("D_SCAlloc: hit the end of memory");
        if (sc_rover->owner)
            *sc_rover->owner = NULL;

        new->size += sc_rover->size;
        new->next = sc_rover->next;
    }

    // create a fragment out of any leftovers
    if (new->size - size > 256)
    {
        sc_rover = (surfcache_t *)((uint8_t *)new + size);
        sc_rover->size = new->size - size;
        sc_rover->next = new->next;
        sc_rover->width = 0;
        sc_rover->owner = NULL;
        new->next = sc_rover;
        new->size = size;
    }
    else
        sc_rover = new->next;

    new->width = width;
    // DEBUG
    if (width > 0)
        new->height = (size - sizeof(*new) + sizeof(new->data)) / width;

    new->owner = NULL; // should be set properly after return

    if (d_roverwrapped)
    {
        if (wrapped_this_time || (sc_rover >= d_initial_rover))
            r_cache_thrash = true;
    }
    else if (wrapped_this_time)
    {
        d_roverwrapped = true;
    }

    D_CheckCacheGuard(); // DEBUG
    return new;
}

/*
=================
D_SCDump
=================
*/
void D_SCDump(void)
{
    surfcache_t *test;

    for (test = sc_base; test; test = test->next)
    {
        if (test == sc_rover)
            Sys_Printf("ROVER:\n");
        printf("%p : %i bytes     %i width\n", test, test->size, test->width);
    }
}

//=============================================================================

// if the num is not a power of 2, assume it will not repeat

int32_t MaskForNum(int32_t num)
{
    if (num == 128)
        return 127;
    if (num == 64)
        return 63;
    if (num == 32)
        return 31;
    if (num == 16)
        return 15;
    return 255;
}

int32_t D_log2(int32_t num)
{
    int32_t c;

    c = 0;

    while (num >>= 1)
        c++;
    return c;
}

//=============================================================================

/*
================
D_CacheSurface
================
*/
surfcache_t *D_CacheSurface(msurface_t *surface, int32_t miplevel)
{
    surfcache_t *cache;

    //
    // if the surface is animating or flashing, flush the cache
    //
    r_drawsurf.texture = R_TextureAnimation(surface->texinfo->texture);
    r_drawsurf.lightadj[0] = d_lightstylevalue[surface->styles[0]];
    r_drawsurf.lightadj[1] = d_lightstylevalue[surface->styles[1]];
    r_drawsurf.lightadj[2] = d_lightstylevalue[surface->styles[2]];
    r_drawsurf.lightadj[3] = d_lightstylevalue[surface->styles[3]];

    //
    // see if the cache holds apropriate data
    //
    cache = surface->cachespots[miplevel];

    if (cache && !cache->dlight && surface->dlightframe != r_framecount && cache->texture == r_drawsurf.texture &&
        cache->lightadj[0] == r_drawsurf.lightadj[0] && cache->lightadj[1] == r_drawsurf.lightadj[1] &&
        cache->lightadj[2] == r_drawsurf.lightadj[2] && cache->lightadj[3] == r_drawsurf.lightadj[3])
        return cache;

    //
    // determine shape of surface
    //
    surfscale = 1.0 / (1 << miplevel);
    r_drawsurf.surfmip = miplevel;
    r_drawsurf.surfwidth = surface->extents[0] >> miplevel;
    r_drawsurf.rowbytes = r_drawsurf.surfwidth;
    r_drawsurf.surfheight = surface->extents[1] >> miplevel;

    //
    // allocate memory if needed
    //
    if (!cache) // if a texture just animated, don't reallocate it
    {
        cache = D_SCAlloc(r_drawsurf.surfwidth, r_drawsurf.surfwidth * r_drawsurf.surfheight);
        surface->cachespots[miplevel] = cache;
        cache->owner = &surface->cachespots[miplevel];
        cache->mipscale = surfscale;
    }

    if (surface->dlightframe == r_framecount)
        cache->dlight = 1;
    else
        cache->dlight = 0;

    r_drawsurf.surfdat = (pixel_t *)cache->data;

    cache->texture = r_drawsurf.texture;
    cache->lightadj[0] = r_drawsurf.lightadj[0];
    cache->lightadj[1] = r_drawsurf.lightadj[1];
    cache->lightadj[2] = r_drawsurf.lightadj[2];
    cache->lightadj[3] = r_drawsurf.lightadj[3];

    //
    // draw and light the surface texture
    //
    r_drawsurf.surf = surface;

    c_surf++;
    R_DrawSurface();

    return surface->cachespots[miplevel];
}
