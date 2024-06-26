// r_light.c

#include "quakedef.h"
#include "r_local.h"

int32_t r_dlightframecount;

/*
==================
R_AnimateLight
==================
*/
void R_AnimateLight(void)
{
    int32_t i, j, k;

    //
    // light animations
    // 'm' is normal light, 'a' is no light, 'z' is double bright
    i = (int32_t)(cl.time * 10);
    for (j = 0; j < MAX_LIGHTSTYLES; j++)
    {
        if (!cl_lightstyle[j].length)
        {
            d_lightstylevalue[j] = 256;
            continue;
        }
        k = i % cl_lightstyle[j].length;
        k = cl_lightstyle[j].map[k] - 'a';
        k = k * 22;
        d_lightstylevalue[j] = k;
    }
}

/*
=============================================================================

DYNAMIC LIGHTS

=============================================================================
*/

/*
=============
R_MarkLights
=============
*/
void R_MarkLights(dlight_t *light, int32_t bit, mnode_t *node)
{
    mplane_t *splitplane;
    float dist;
    msurface_t *surf;
    int32_t i;

    if (node->contents < 0)
        return;

    splitplane = node->plane;
    dist = DotProduct(light->origin, splitplane->normal) - splitplane->dist;

    if (dist > light->radius)
    {
        R_MarkLights(light, bit, node->children[0]);
        return;
    }
    if (dist < -light->radius)
    {
        R_MarkLights(light, bit, node->children[1]);
        return;
    }

    // mark the polygons
    surf = cl.worldmodel->surfaces + node->firstsurface;
    for (i = 0; i < node->numsurfaces; i++, surf++)
    {
        if (surf->dlightframe != r_dlightframecount)
        {
            surf->dlightbits = 0;
            surf->dlightframe = r_dlightframecount;
        }
        surf->dlightbits |= bit;
    }

    R_MarkLights(light, bit, node->children[0]);
    R_MarkLights(light, bit, node->children[1]);
}

/*
=============
R_PushDlights
=============
*/
void R_PushDlights(void)
{
    int32_t i;
    dlight_t *l;

    r_dlightframecount = r_framecount + 1; // because the count hasn't
                                           //  advanced yet for this frame
    l = cl_dlights;

    for (i = 0; i < MAX_DLIGHTS; i++, l++)
    {
        if (l->die < cl.time || !l->radius)
            continue;
        R_MarkLights(l, 1 << i, cl.worldmodel->nodes);
    }
}

/*
=============================================================================

LIGHT SAMPLING

=============================================================================
*/

int32_t RecursiveLightPoint(mnode_t *node, vec3_t start, vec3_t end)
{
    int32_t r;
    float front, back, frac;
    int32_t side;
    mplane_t *plane;
    vec3_t mid;
    msurface_t *surf;
    int32_t s, t, ds, dt;
    int32_t i;
    mtexinfo_t *tex;
    uint8_t *lightmap;
    uint32_t scale;
    int32_t maps;

    if (node->contents < 0)
        return -1; // didn't hit anything

    // calculate mid point

    // FIXME: optimize for axial
    plane = node->plane;
    front = DotProduct(start, plane->normal) - plane->dist;
    back = DotProduct(end, plane->normal) - plane->dist;
    side = front < 0;

    if ((back < 0) == side)
        return RecursiveLightPoint(node->children[side], start, end);

    frac = front / (front - back);
    mid[0] = start[0] + (end[0] - start[0]) * frac;
    mid[1] = start[1] + (end[1] - start[1]) * frac;
    mid[2] = start[2] + (end[2] - start[2]) * frac;

    // go down front side
    r = RecursiveLightPoint(node->children[side], start, mid);
    if (r >= 0)
        return r; // hit something

    if ((back < 0) == side)
        return -1; // didn't hit anuthing

    // check for impact on this node

    surf = cl.worldmodel->surfaces + node->firstsurface;
    for (i = 0; i < node->numsurfaces; i++, surf++)
    {
        if (surf->flags & SURF_DRAWTILED)
            continue; // no lightmaps

        tex = surf->texinfo;

        s = DotProduct(mid, tex->vecs[0]) + tex->vecs[0][3];
        t = DotProduct(mid, tex->vecs[1]) + tex->vecs[1][3];
        ;

        if (s < surf->texturemins[0] || t < surf->texturemins[1])
            continue;

        ds = s - surf->texturemins[0];
        dt = t - surf->texturemins[1];

        if (ds > surf->extents[0] || dt > surf->extents[1])
            continue;

        if (!surf->samples)
            return 0;

        ds >>= 4;
        dt >>= 4;

        lightmap = surf->samples;
        r = 0;
        if (lightmap)
        {

            lightmap += dt * ((surf->extents[0] >> 4) + 1) + ds;

            for (maps = 0; maps < MAXLIGHTMAPS && surf->styles[maps] != 255; maps++)
            {
                scale = d_lightstylevalue[surf->styles[maps]];
                r += *lightmap * scale;
                lightmap += ((surf->extents[0] >> 4) + 1) * ((surf->extents[1] >> 4) + 1);
            }

            r >>= 8;
        }

        return r;
    }

    // go down back side
    return RecursiveLightPoint(node->children[!side], mid, end);
}

int32_t R_LightPoint(vec3_t p)
{
    vec3_t end;
    int32_t r;

    if (!cl.worldmodel->lightdata)
        return 255;

    end[0] = p[0];
    end[1] = p[1];
    end[2] = p[2] - 2048;

    r = RecursiveLightPoint(cl.worldmodel->nodes, p, end);

    if (r == -1)
        r = 0;

    if (r < r_refdef.ambientlight)
        r = r_refdef.ambientlight;

    return r;
}
