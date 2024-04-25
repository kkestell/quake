// d_sky.c

#include "quakedef.h"
#include "r_local.h"
#include "d_local.h"

#define SKY_SPAN_SHIFT	5
#define SKY_SPAN_MAX	(1 << SKY_SPAN_SHIFT)


/*
=================
D_Sky_uv_To_st
=================
*/
void D_Sky_uv_To_st (int32_t u, int32_t v, fixed16_t *s, fixed16_t *t)
{
	float	wu, wv, temp;
	vec3_t	end;

	if (r_refdef.vrect.width >= r_refdef.vrect.height)
		temp = (float)r_refdef.vrect.width;
	else
		temp = (float)r_refdef.vrect.height;

	wu = 8192.0 * (float)(u-((int32_t)vid.width>>1)) / temp;
	wv = 8192.0 * (float)(((int32_t)vid.height>>1)-v) / temp;

	end[0] = 4096*vpn[0] + wu*vright[0] + wv*vup[0];
	end[1] = 4096*vpn[1] + wu*vright[1] + wv*vup[1];
	end[2] = 4096*vpn[2] + wu*vright[2] + wv*vup[2];
	end[2] *= 3;
	VectorNormalize (end);

	temp = skytime*skyspeed;	// TODO: add D_SetupFrame & set this there
	*s = (int32_t)((temp + 6*(SKYSIZE/2-1)*end[0]) * 0x10000);
	*t = (int32_t)((temp + 6*(SKYSIZE/2-1)*end[1]) * 0x10000);
}


/*
=================
D_DrawSkyScans8
=================
*/
void D_DrawSkyScans8 (espan_t *pspan)
{
	int32_t				count, spancount, u, v;
	unsigned char	*pdest;
	fixed16_t		s, t, snext, tnext, sstep, tstep;
	int32_t				spancountminus1;

	sstep = 0;	// keep compiler happy
	tstep = 0;	// ditto

	do
	{
		pdest = (unsigned char *)((uint8_t *)d_viewbuffer +
				(screenwidth * pspan->v) + pspan->u);

		count = pspan->count;

	// calculate the initial s & t
		u = pspan->u;
		v = pspan->v;
		D_Sky_uv_To_st (u, v, &s, &t);

		do
		{
			if (count >= SKY_SPAN_MAX)
				spancount = SKY_SPAN_MAX;
			else
				spancount = count;

			count -= spancount;

			if (count)
			{
				u += spancount;

			// calculate s and t at far end of span,
			// calculate s and t steps across span by shifting
				D_Sky_uv_To_st (u, v, &snext, &tnext);

				sstep = (snext - s) >> SKY_SPAN_SHIFT;
				tstep = (tnext - t) >> SKY_SPAN_SHIFT;
			}
			else
			{
			// calculate s and t at last pixel in span,
			// calculate s and t steps across span by division
				spancountminus1 = (float)(spancount - 1);

				if (spancountminus1 > 0)
				{
					u += spancountminus1;
					D_Sky_uv_To_st (u, v, &snext, &tnext);

					sstep = (snext - s) / spancountminus1;
					tstep = (tnext - t) / spancountminus1;
				}
			}

			do
			{
				*pdest++ = r_skysource[((t & R_SKY_TMASK) >> 8) +
						((s & R_SKY_SMASK) >> 16)];
				s += sstep;
				t += tstep;
			} while (--spancount > 0);

			s = snext;
			t = tnext;

		} while (count > 0);

	} while ((pspan = pspan->pnext) != NULL);
}

