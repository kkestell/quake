// r_surf.c: surface-related refresh code

#include "quakedef.h"
#include "r_local.h"

drawsurf_t r_drawsurf;

int32_t lightleft, sourcesstep, blocksize, sourcetstep;
int32_t lightdelta, lightdeltastep;
int32_t lightright, lightleftstep, lightrightstep, blockdivshift;
uint32_t blockdivmask;
void *prowdestbase;
unsigned char *pbasesource;
int32_t surfrowbytes; // used by ASM files
uint32_t *r_lightptr;
int32_t r_stepback;
int32_t r_lightwidth;
int32_t r_numhblocks, r_numvblocks;
unsigned char *r_source, *r_sourcemax;

void R_DrawSurfaceBlock8_mip0(void);
void R_DrawSurfaceBlock8_mip1(void);
void R_DrawSurfaceBlock8_mip2(void);
void R_DrawSurfaceBlock8_mip3(void);

static void (*surfmiptable[4])(void) = {R_DrawSurfaceBlock8_mip0, R_DrawSurfaceBlock8_mip1, R_DrawSurfaceBlock8_mip2,
                                        R_DrawSurfaceBlock8_mip3};

uint32_t blocklights[18 * 18];

/*
===============
R_AddDynamicLights
===============
*/
void R_AddDynamicLights(void)
{
    msurface_t *surf;
    int32_t lnum;
    int32_t sd, td;
    float dist, rad, minlight;
    vec3_t impact, local;
    int32_t s, t;
    int32_t i;
    int32_t smax, tmax;
    mtexinfo_t *tex;

    surf = r_drawsurf.surf;
    smax = (surf->extents[0] >> 4) + 1;
    tmax = (surf->extents[1] >> 4) + 1;
    tex = surf->texinfo;

    for (lnum = 0; lnum < MAX_DLIGHTS; lnum++)
    {
        if (!(surf->dlightbits & (1 << lnum)))
            continue; // not lit by this light

        rad = cl_dlights[lnum].radius;
        dist = DotProduct(cl_dlights[lnum].origin, surf->plane->normal) - surf->plane->dist;
        rad -= fabs(dist);
        minlight = cl_dlights[lnum].minlight;
        if (rad < minlight)
            continue;
        minlight = rad - minlight;

        for (i = 0; i < 3; i++)
        {
            impact[i] = cl_dlights[lnum].origin[i] - surf->plane->normal[i] * dist;
        }

        local[0] = DotProduct(impact, tex->vecs[0]) + tex->vecs[0][3];
        local[1] = DotProduct(impact, tex->vecs[1]) + tex->vecs[1][3];

        local[0] -= surf->texturemins[0];
        local[1] -= surf->texturemins[1];

        for (t = 0; t < tmax; t++)
        {
            td = local[1] - t * 16;
            if (td < 0)
                td = -td;
            for (s = 0; s < smax; s++)
            {
                sd = local[0] - s * 16;
                if (sd < 0)
                    sd = -sd;
                if (sd > td)
                    dist = sd + (td >> 1);
                else
                    dist = td + (sd >> 1);
                if (dist < minlight)
                    blocklights[t * smax + s] += (rad - dist) * 256;
            }
        }
    }
}

/*
===============
R_BuildLightMap

Combine and scale multiple lightmaps into the 8.8 format in blocklights
===============
*/
void R_BuildLightMap(void)
{
    int32_t smax, tmax;
    int32_t t;
    int32_t i, size;
    uint8_t *lightmap;
    uint32_t scale;
    int32_t maps;
    msurface_t *surf;

    surf = r_drawsurf.surf;

    smax = (surf->extents[0] >> 4) + 1;
    tmax = (surf->extents[1] >> 4) + 1;
    size = smax * tmax;
    lightmap = surf->samples;

    if (r_fullbright.value || !cl.worldmodel->lightdata)
    {
        for (i = 0; i < size; i++)
            blocklights[i] = 0;
        return;
    }

    // clear to ambient
    for (i = 0; i < size; i++)
        blocklights[i] = r_refdef.ambientlight << 8;

    // add all the lightmaps
    if (lightmap)
        for (maps = 0; maps < MAXLIGHTMAPS && surf->styles[maps] != 255; maps++)
        {
            scale = r_drawsurf.lightadj[maps]; // 8.8 fraction
            for (i = 0; i < size; i++)
                blocklights[i] += lightmap[i] * scale;
            lightmap += size; // skip to next lightmap
        }

    // add all the dynamic lights
    if (surf->dlightframe == r_framecount)
        R_AddDynamicLights();

    // bound, invert, and shift
    for (i = 0; i < size; i++)
    {
        t = (255 * 256 - (int32_t)blocklights[i]) >> (8 - VID_CBITS);

        if (t < (1 << 6))
            t = (1 << 6);

        blocklights[i] = t;
    }
}

/*
===============
R_TextureAnimation

Returns the proper texture for a given time and base texture
===============
*/
texture_t *R_TextureAnimation(texture_t *base)
{
    int32_t reletive;
    int32_t count;

    if (currententity->frame)
    {
        if (base->alternate_anims)
            base = base->alternate_anims;
    }

    if (!base->anim_total)
        return base;

    reletive = (int32_t)(cl.time * 10) % base->anim_total;

    count = 0;
    while (base->anim_min > reletive || base->anim_max <= reletive)
    {
        base = base->anim_next;
        if (!base)
            Sys_Error("R_TextureAnimation: broken cycle");
        if (++count > 100)
            Sys_Error("R_TextureAnimation: infinite cycle");
    }

    return base;
}

/*
===============
R_DrawSurface
===============
*/
void R_DrawSurface(void)
{
    unsigned char *basetptr;
    int32_t smax, tmax, twidth;
    int32_t u;
    int32_t soffset, basetoffset, texwidth;
    int32_t horzblockstep;
    unsigned char *pcolumndest;
    void (*pblockdrawer)(void);
    texture_t *mt;

    // calculate the lightings
    R_BuildLightMap();

    surfrowbytes = r_drawsurf.rowbytes;

    mt = r_drawsurf.texture;

    r_source = (uint8_t *)mt + mt->offsets[r_drawsurf.surfmip];

    // the fractional light values should range from 0 to (VID_GRADES - 1) << 16
    // from a source range of 0 - 255

    texwidth = mt->width >> r_drawsurf.surfmip;

    blocksize = 16 >> r_drawsurf.surfmip;
    blockdivshift = 4 - r_drawsurf.surfmip;
    blockdivmask = (1 << blockdivshift) - 1;

    r_lightwidth = (r_drawsurf.surf->extents[0] >> 4) + 1;

    r_numhblocks = r_drawsurf.surfwidth >> blockdivshift;
    r_numvblocks = r_drawsurf.surfheight >> blockdivshift;

    //==============================

    if (r_pixbytes == 1)
    {
        pblockdrawer = surfmiptable[r_drawsurf.surfmip];
        // TODO: only needs to be set when there is a display settings change
        horzblockstep = blocksize;
    }
    else
    {
        pblockdrawer = R_DrawSurfaceBlock16;
        // TODO: only needs to be set when there is a display settings change
        horzblockstep = blocksize << 1;
    }

    smax = mt->width >> r_drawsurf.surfmip;
    twidth = texwidth;
    tmax = mt->height >> r_drawsurf.surfmip;
    sourcetstep = texwidth;
    r_stepback = tmax * twidth;

    r_sourcemax = r_source + (tmax * smax);

    soffset = r_drawsurf.surf->texturemins[0];
    basetoffset = r_drawsurf.surf->texturemins[1];

    // << 16 components are to guarantee positive values for %
    soffset = ((soffset >> r_drawsurf.surfmip) + (smax << 16)) % smax;
    basetptr = &r_source[((((basetoffset >> r_drawsurf.surfmip) + (tmax << 16)) % tmax) * twidth)];

    pcolumndest = r_drawsurf.surfdat;

    for (u = 0; u < r_numhblocks; u++)
    {
        r_lightptr = blocklights + u;

        prowdestbase = pcolumndest;

        pbasesource = basetptr + soffset;

        (*pblockdrawer)();

        soffset = soffset + blocksize;
        if (soffset >= smax)
            soffset = 0;

        pcolumndest += horzblockstep;
    }
}

//=============================================================================

/*
================
R_DrawSurfaceBlock8_mip0
================
*/
void R_DrawSurfaceBlock8_mip0(void)
{
    int32_t v, i, b, lightstep, lighttemp, light;
    unsigned char pix, *psource, *prowdest;

    psource = pbasesource;
    prowdest = prowdestbase;

    for (v = 0; v < r_numvblocks; v++)
    {
        // FIXME: make these locals?
        // FIXME: use delta rather than both right and left, like ASM?
        lightleft = r_lightptr[0];
        lightright = r_lightptr[1];
        r_lightptr += r_lightwidth;
        lightleftstep = (r_lightptr[0] - lightleft) >> 4;
        lightrightstep = (r_lightptr[1] - lightright) >> 4;

        for (i = 0; i < 16; i++)
        {
            lighttemp = lightleft - lightright;
            lightstep = lighttemp >> 4;

            light = lightright;

            for (b = 15; b >= 0; b--)
            {
                pix = psource[b];
                prowdest[b] = ((unsigned char *)vid.colormap)[(light & 0xFF00) + pix];
                light += lightstep;
            }

            psource += sourcetstep;
            lightright += lightrightstep;
            lightleft += lightleftstep;
            prowdest += surfrowbytes;
        }

        if (psource >= r_sourcemax)
            psource -= r_stepback;
    }
}

/*
================
R_DrawSurfaceBlock8_mip1
================
*/
void R_DrawSurfaceBlock8_mip1(void)
{
    int32_t v, i, b, lightstep, lighttemp, light;
    unsigned char pix, *psource, *prowdest;

    psource = pbasesource;
    prowdest = prowdestbase;

    for (v = 0; v < r_numvblocks; v++)
    {
        // FIXME: make these locals?
        // FIXME: use delta rather than both right and left, like ASM?
        lightleft = r_lightptr[0];
        lightright = r_lightptr[1];
        r_lightptr += r_lightwidth;
        lightleftstep = (r_lightptr[0] - lightleft) >> 3;
        lightrightstep = (r_lightptr[1] - lightright) >> 3;

        for (i = 0; i < 8; i++)
        {
            lighttemp = lightleft - lightright;
            lightstep = lighttemp >> 3;

            light = lightright;

            for (b = 7; b >= 0; b--)
            {
                pix = psource[b];
                prowdest[b] = ((unsigned char *)vid.colormap)[(light & 0xFF00) + pix];
                light += lightstep;
            }

            psource += sourcetstep;
            lightright += lightrightstep;
            lightleft += lightleftstep;
            prowdest += surfrowbytes;
        }

        if (psource >= r_sourcemax)
            psource -= r_stepback;
    }
}

/*
================
R_DrawSurfaceBlock8_mip2
================
*/
void R_DrawSurfaceBlock8_mip2(void)
{
    int32_t v, i, b, lightstep, lighttemp, light;
    unsigned char pix, *psource, *prowdest;

    psource = pbasesource;
    prowdest = prowdestbase;

    for (v = 0; v < r_numvblocks; v++)
    {
        // FIXME: make these locals?
        // FIXME: use delta rather than both right and left, like ASM?
        lightleft = r_lightptr[0];
        lightright = r_lightptr[1];
        r_lightptr += r_lightwidth;
        lightleftstep = (r_lightptr[0] - lightleft) >> 2;
        lightrightstep = (r_lightptr[1] - lightright) >> 2;

        for (i = 0; i < 4; i++)
        {
            lighttemp = lightleft - lightright;
            lightstep = lighttemp >> 2;

            light = lightright;

            for (b = 3; b >= 0; b--)
            {
                pix = psource[b];
                prowdest[b] = ((unsigned char *)vid.colormap)[(light & 0xFF00) + pix];
                light += lightstep;
            }

            psource += sourcetstep;
            lightright += lightrightstep;
            lightleft += lightleftstep;
            prowdest += surfrowbytes;
        }

        if (psource >= r_sourcemax)
            psource -= r_stepback;
    }
}

/*
================
R_DrawSurfaceBlock8_mip3
================
*/
void R_DrawSurfaceBlock8_mip3(void)
{
    int32_t v, i, b, lightstep, lighttemp, light;
    unsigned char pix, *psource, *prowdest;

    psource = pbasesource;
    prowdest = prowdestbase;

    for (v = 0; v < r_numvblocks; v++)
    {
        // FIXME: make these locals?
        // FIXME: use delta rather than both right and left, like ASM?
        lightleft = r_lightptr[0];
        lightright = r_lightptr[1];
        r_lightptr += r_lightwidth;
        lightleftstep = (r_lightptr[0] - lightleft) >> 1;
        lightrightstep = (r_lightptr[1] - lightright) >> 1;

        for (i = 0; i < 2; i++)
        {
            lighttemp = lightleft - lightright;
            lightstep = lighttemp >> 1;

            light = lightright;

            for (b = 1; b >= 0; b--)
            {
                pix = psource[b];
                prowdest[b] = ((unsigned char *)vid.colormap)[(light & 0xFF00) + pix];
                light += lightstep;
            }

            psource += sourcetstep;
            lightright += lightrightstep;
            lightleft += lightleftstep;
            prowdest += surfrowbytes;
        }

        if (psource >= r_sourcemax)
            psource -= r_stepback;
    }
}

/*
================
R_DrawSurfaceBlock16

FIXME: make this work
================
*/
void R_DrawSurfaceBlock16(void)
{
    int32_t k;
    unsigned char *psource;
    int32_t lighttemp, lightstep, light;
    uint16_t *prowdest;

    prowdest = (uint16_t *)prowdestbase;

    for (k = 0; k < blocksize; k++)
    {
        uint16_t *pdest;
        unsigned char pix;
        int32_t b;

        psource = pbasesource;
        lighttemp = lightright - lightleft;
        lightstep = lighttemp >> blockdivshift;

        light = lightleft;
        pdest = prowdest;

        for (b = 0; b < blocksize; b++)
        {
            pix = *psource;
            *pdest = vid.colormap16[(light & 0xFF00) + pix];
            psource += sourcesstep;
            pdest++;
            light += lightstep;
        }

        pbasesource += sourcetstep;
        lightright += lightrightstep;
        lightleft += lightleftstep;
        prowdest = (uint16_t *)((int32_t)prowdest + surfrowbytes);
    }

    prowdestbase = prowdest;
}

//============================================================================

/*
================
R_GenTurbTile
================
*/
void R_GenTurbTile(pixel_t *pbasetex, void *pdest)
{
    int32_t *turb;
    int32_t i, j, s, t;
    uint8_t *pd;

    turb = sintable + ((int32_t)(cl.time * SPEED) & (CYCLE - 1));
    pd = (uint8_t *)pdest;

    for (i = 0; i < TILE_SIZE; i++)
    {
        for (j = 0; j < TILE_SIZE; j++)
        {
            s = (((j << 16) + turb[i & (CYCLE - 1)]) >> 16) & 63;
            t = (((i << 16) + turb[j & (CYCLE - 1)]) >> 16) & 63;
            *pd++ = *(pbasetex + (t << 6) + s);
        }
    }
}

/*
================
R_GenTurbTile16
================
*/
void R_GenTurbTile16(pixel_t *pbasetex, void *pdest)
{
    int32_t *turb;
    int32_t i, j, s, t;
    uint16_t *pd;

    turb = sintable + ((int32_t)(cl.time * SPEED) & (CYCLE - 1));
    pd = (uint16_t *)pdest;

    for (i = 0; i < TILE_SIZE; i++)
    {
        for (j = 0; j < TILE_SIZE; j++)
        {
            s = (((j << 16) + turb[i & (CYCLE - 1)]) >> 16) & 63;
            t = (((i << 16) + turb[j & (CYCLE - 1)]) >> 16) & 63;
            *pd++ = d_8to16table[*(pbasetex + (t << 6) + s)];
        }
    }
}

/*
================
R_GenTile
================
*/
void R_GenTile(msurface_t *psurf, void *pdest)
{
    if (psurf->flags & SURF_DRAWTURB)
    {
        if (r_pixbytes == 1)
        {
            R_GenTurbTile((pixel_t *)((uint8_t *)psurf->texinfo->texture + psurf->texinfo->texture->offsets[0]), pdest);
        }
        else
        {
            R_GenTurbTile16((pixel_t *)((uint8_t *)psurf->texinfo->texture + psurf->texinfo->texture->offsets[0]),
                            pdest);
        }
    }
    else if (psurf->flags & SURF_DRAWSKY)
    {
        if (r_pixbytes == 1)
        {
            R_GenSkyTile(pdest);
        }
        else
        {
            R_GenSkyTile16(pdest);
        }
    }
    else
    {
        Sys_Error("Unknown tile type");
    }
}
