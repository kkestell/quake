// d_clear: clears a specified rectangle to the specified color

#include "quakedef.h"

/*
================
D_FillRect
================
*/
void D_FillRect(vrect_t *rect, int32_t color)
{
    int32_t rx, ry, rwidth, rheight;
    unsigned char *dest;
    uint32_t *ldest;

    rx = rect->x;
    ry = rect->y;
    rwidth = rect->width;
    rheight = rect->height;

    if (rx < 0)
    {
        rwidth += rx;
        rx = 0;
    }
    if (ry < 0)
    {
        rheight += ry;
        ry = 0;
    }
    if (rx + rwidth > vid.width)
        rwidth = vid.width - rx;
    if (ry + rheight > vid.height)
        rheight = vid.height - rx;

    if (rwidth < 1 || rheight < 1)
        return;

    dest = ((uint8_t *)vid.buffer + ry * vid.rowbytes + rx);

    if (((rwidth & 0x03) == 0) && (((int32_t)dest & 0x03) == 0))
    {
        // faster aligned dword clear
        ldest = (uint32_t *)dest;
        color += color << 16;

        rwidth >>= 2;
        color += color << 8;

        for (ry = 0; ry < rheight; ry++)
        {
            for (rx = 0; rx < rwidth; rx++)
                ldest[rx] = color;
            ldest = (uint32_t *)((uint8_t *)ldest + vid.rowbytes);
        }
    }
    else
    {
        // slower byte-by-byte clear for unaligned cases
        for (ry = 0; ry < rheight; ry++)
        {
            for (rx = 0; rx < rwidth; rx++)
                dest[rx] = color;
            dest += vid.rowbytes;
        }
    }
}
