// r_sky.c

#include "quakedef.h"
#include "r_local.h"
#include "d_local.h"


int32_t		iskyspeed = 8;
int32_t		iskyspeed2 = 2;
float	skyspeed, skyspeed2;

float		skytime;

uint8_t		*r_skysource;

int32_t r_skymade;
int32_t r_skydirect;		// not used?


// TODO: clean up these routines

uint8_t	bottomsky[128*131];
uint8_t	bottommask[128*131];
uint8_t	newsky[128*256];	// newsky and topsky both pack in here, 128 bytes
							//  of newsky on the left of each scan, 128 bytes
							//  of topsky on the right, because the low-level
							//  drawers need 256-byte scan widths


/*
=============
R_InitSky

A sky texture is 256*128, with the right side being a masked overlay
==============
*/
void R_InitSky (texture_t *mt)
{
	int32_t			i, j;
	uint8_t		*src;

	src = (byte *)mt + mt->offsets[0];

	for (i=0 ; i<128 ; i++)
	{
		for (j=0 ; j<128 ; j++)
		{
			newsky[(i*256) + j + 128] = src[i*256 + j + 128];
		}
	}

	for (i=0 ; i<128 ; i++)
	{
		for (j=0 ; j<131 ; j++)
		{
			if (src[i*256 + (j & 0x7F)])
			{
				bottomsky[(i*131) + j] = src[i*256 + (j & 0x7F)];
				bottommask[(i*131) + j] = 0;
			}
			else
			{
				bottomsky[(i*131) + j] = 0;
				bottommask[(i*131) + j] = 0xff;
			}
		}
	}
	
	r_skysource = newsky;
}


/*
=================
R_MakeSky
=================
*/
void R_MakeSky (void)
{
	int32_t			x, y;
	int32_t			ofs, baseofs;
	int32_t			xshift, yshift;
	uint32_t 	*pnewsky;
	static int32_t	xlast = -1, ylast = -1;

	xshift = skytime*skyspeed;
	yshift = skytime*skyspeed;

	if ((xshift == xlast) && (yshift == ylast))
		return;

	xlast = xshift;
	ylast = yshift;
	
	pnewsky = (uint32_t *)&newsky[0];

	for (y=0 ; y<SKYSIZE ; y++)
	{
		baseofs = ((y+yshift) & SKYMASK) * 131;

// FIXME: clean this up
#if UNALIGNED_OK

		for (x=0 ; x<SKYSIZE ; x += 4)
		{
			ofs = baseofs + ((x+xshift) & SKYMASK);

		// PORT: unaligned dword access to bottommask and bottomsky

			*pnewsky = (*(pnewsky + (128 / sizeof (uint32_t))) &
						*(uint32_t *)&bottommask[ofs]) |
						*(uint32_t *)&bottomsky[ofs];
			pnewsky++;
		}

#else

		for (x=0 ; x<SKYSIZE ; x++)
		{
			ofs = baseofs + ((x+xshift) & SKYMASK);

			*(byte *)pnewsky = (*((byte *)pnewsky + 128) &
						*(byte *)&bottommask[ofs]) |
						*(byte *)&bottomsky[ofs];
			pnewsky = (uint32_t *)((byte *)pnewsky + 1);
		}

#endif

		pnewsky += 128 / sizeof (uint32_t);
	}

	r_skymade = 1;
}


/*
=================
R_GenSkyTile
=================
*/
void R_GenSkyTile (void *pdest)
{
	int32_t			x, y;
	int32_t			ofs, baseofs;
	int32_t			xshift, yshift;
	uint32_t 	*pnewsky;
	uint32_t 	*pd;

	xshift = skytime*skyspeed;
	yshift = skytime*skyspeed;

	pnewsky = (uint32_t *)&newsky[0];
	pd = (uint32_t *)pdest;

	for (y=0 ; y<SKYSIZE ; y++)
	{
		baseofs = ((y+yshift) & SKYMASK) * 131;

// FIXME: clean this up
#if UNALIGNED_OK

		for (x=0 ; x<SKYSIZE ; x += 4)
		{
			ofs = baseofs + ((x+xshift) & SKYMASK);

		// PORT: unaligned dword access to bottommask and bottomsky

			*pd = (*(pnewsky + (128 / sizeof (uint32_t))) &
				   *(uint32_t *)&bottommask[ofs]) |
				   *(uint32_t *)&bottomsky[ofs];
			pnewsky++;
			pd++;
		}

#else

		for (x=0 ; x<SKYSIZE ; x++)
		{
			ofs = baseofs + ((x+xshift) & SKYMASK);

			*(byte *)pd = (*((byte *)pnewsky + 128) &
						*(byte *)&bottommask[ofs]) |
						*(byte *)&bottomsky[ofs];
			pnewsky = (uint32_t *)((byte *)pnewsky + 1);
			pd = (uint32_t *)((byte *)pd + 1);
		}

#endif

		pnewsky += 128 / sizeof (uint32_t);
	}
}


/*
=================
R_GenSkyTile16
=================
*/
void R_GenSkyTile16 (void *pdest)
{
	int32_t				x, y;
	int32_t				ofs, baseofs;
	int32_t				xshift, yshift;
	uint8_t			*pnewsky;
	uint16_t	*pd;

	xshift = skytime * skyspeed;
	yshift = skytime * skyspeed;

	pnewsky = (byte *)&newsky[0];
	pd = (uint16_t *)pdest;

	for (y=0 ; y<SKYSIZE ; y++)
	{
		baseofs = ((y+yshift) & SKYMASK) * 131;

// FIXME: clean this up
// FIXME: do faster unaligned version?
		for (x=0 ; x<SKYSIZE ; x++)
		{
			ofs = baseofs + ((x+xshift) & SKYMASK);

			*pd = d_8to16table[(*(pnewsky + 128) &
					*(byte *)&bottommask[ofs]) |
					*(byte *)&bottomsky[ofs]];
			pnewsky++;
			pd++;
		}

		pnewsky += TILE_SIZE;
	}
}


/*
=============
R_SetSkyFrame
==============
*/
void R_SetSkyFrame (void)
{
	int32_t		g, s1, s2;
	float	temp;

	skyspeed = iskyspeed;
	skyspeed2 = iskyspeed2;

	g = GreatestCommonDivisor (iskyspeed, iskyspeed2);
	s1 = iskyspeed / g;
	s2 = iskyspeed2 / g;
	temp = SKYSIZE * s1 * s2;

	skytime = cl.time - ((int32_t)(cl.time / temp) * temp);
	

	r_skymade = 0;
}


