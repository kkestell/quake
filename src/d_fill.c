#include "quakedef.h"

void D_FillRect(const vrect_t *vrect, const int32_t color)
{
    int32_t rx = vrect->x;
    int32_t ry = vrect->y;
    int32_t rwidth = vrect->width;
    int32_t rheight = vrect->height;

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

    const int32_t vwidth  = vid.width > (uint32_t)INT32_MAX ? INT32_MAX : (int32_t)vid.width;
    const int32_t vheight = vid.height > (uint32_t)INT32_MAX ? INT32_MAX : (int32_t)vid.height;

    if (rx > vwidth)
        rwidth = 0;
    else if (rwidth > vwidth - rx)
        rwidth = vwidth - rx;

    if (ry > vheight)
        rheight = 0;
    else if (rheight > vheight - ry)
        rheight = vheight - ry;

    if (rwidth < 1 || rheight < 1)
        return;

    uint8_t *dest = vid.buffer + (size_t)ry * vid.rowbytes + rx;

    if ((rwidth & 3) == 0 && ((uintptr_t)dest & 3u) == 0 && (vid.rowbytes & 3) == 0)
    {
        uint32_t *ldest = (uint32_t *)dest;
        const uint32_t fill32 = 0x01010101u * (uint8_t)color;
        const int32_t words = rwidth >> 2;

        for (int32_t y = 0; y < rheight; y++)
        {
            for (int32_t x = 0; x < words; x++)
                ldest[x] = fill32;

            ldest = (uint32_t *)((uint8_t *)ldest + vid.rowbytes);
        }
    }
    else
    {
        const uint8_t fill8 = (uint8_t)color;
        for (int32_t y = 0; y < rheight; y++)
        {
            for (int32_t x = 0; x < rwidth; x++)
                dest[x] = fill8;

            dest += vid.rowbytes;
        }
    }
}

