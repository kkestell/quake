// d_iface.h: interface header file for rasterization driver modules

#define WARP_WIDTH 320
#define WARP_HEIGHT 200

#define MAX_LBM_HEIGHT 480

typedef struct
{
    float u, v;
    float s, t;
    float zi;
} emitpoint_t;

typedef enum
{
    pt_static,
    pt_grav,
    pt_slowgrav,
    pt_fire,
    pt_explode,
    pt_explode2,
    pt_blob,
    pt_blob2
} ptype_t;

// !!! if this is changed, it must be changed in d_ifacea.h too !!!
typedef struct particle_s
{
    // driver-usable fields
    vec3_t org;
    float color;
    // drivers never touch the following fields
    struct particle_s *next;
    vec3_t vel;
    float ramp;
    float die;
    ptype_t type;
} particle_t;

#define PARTICLE_Z_CLIP 8.0

typedef struct polyvert_s
{
    float u, v, zi, s, t;
} polyvert_t;

typedef struct polydesc_s
{
    int32_t numverts;
    float nearzi;
    msurface_t *pcurrentface;
    polyvert_t *pverts;
} polydesc_t;

// !!! if this is changed, it must be changed in d_ifacea.h too !!!
typedef struct finalvert_s
{
    int32_t v[6]; // u, v, s, t, l, 1/z
    int32_t flags;
    float reserved;
} finalvert_t;

// !!! if this is changed, it must be changed in d_ifacea.h too !!!
typedef struct
{
    void *pskin;
    maliasskindesc_t *pskindesc;
    int32_t skinwidth;
    int32_t skinheight;
    mtriangle_t *ptriangles;
    finalvert_t *pfinalverts;
    int32_t numtriangles;
    int32_t drawtype;
    int32_t seamfixupX16;
} affinetridesc_t;

// !!! if this is changed, it must be changed in d_ifacea.h too !!!
typedef struct
{
    float u, v, zi, color;
} screenpart_t;

typedef struct
{
    int32_t nump;
    emitpoint_t *pverts; // there's room for an extra element at [nump],
                         //  if the driver wants to duplicate element [0] at
                         //  element [nump] to avoid dealing with wrapping
    mspriteframe_t *pspriteframe;
    vec3_t vup, vright, vpn; // in worldspace
    float nearzi;
} spritedesc_t;

typedef struct
{
    int32_t u, v;
    float zi;
    int32_t color;
} zpointdesc_t;

extern cvar_t r_drawflat;
extern int32_t d_spanpixcount;
extern int32_t r_framecount;            // sequence # of current frame since Quake
                                        //  started
extern bool r_drawpolys;                // 1 if driver wants clipped polygons
                                        //  rather than a span list
extern bool r_drawculledpolys;          // 1 if driver wants clipped polygons that
                                        //  have been culled by the edge list
extern bool r_worldpolysbacktofront;    // 1 if driver wants polygons
                                        //  delivered back to front rather
                                        //  than front to back
extern bool r_recursiveaffinetriangles; // true if a driver wants to use
                                        //  recursive triangular subdivison
                                        //  and vertex drawing via
                                        //  D_PolysetDrawFinalVerts() past
                                        //  a certain distance (normally
                                        //  only used by the software
                                        //  driver)
extern float r_aliasuvscale;            // scale-up factor for screen u and v
                                        //  on Alias vertices passed to driver
extern int32_t r_pixbytes;
extern bool r_dowarp;

extern affinetridesc_t r_affinetridesc;
extern spritedesc_t r_spritedesc;
extern zpointdesc_t r_zpointdesc;
extern polydesc_t r_polydesc;

extern int32_t d_con_indirect; // if 0, Quake will draw console directly
                               //  to vid.buffer; if 1, Quake will
                               //  draw console via D_DrawRect. Must be
                               //  defined by driver

extern vec3_t r_pright, r_pup, r_ppn;

void D_Aff8Patch(void *pcolormap);
void D_BeginDirectRect(int32_t x, int32_t y, uint8_t *pbitmap, int32_t width, int32_t height);
void D_DisableBackBufferAccess(void);
void D_EndDirectRect(int32_t x, int32_t y, int32_t width, int32_t height);
void D_PolysetDraw(void);
void D_PolysetDrawFinalVerts(finalvert_t *fv, int32_t numverts);
void D_DrawParticle(particle_t *pparticle);
void D_DrawPoly(void);
void D_DrawSprite(void);
void D_DrawSurfaces(void);
void D_DrawZPoint(void);
void D_EnableBackBufferAccess(void);
void D_EndParticles(void);
void D_Init(void);
void D_ViewChanged(void);
void D_SetupFrame(void);
void D_StartParticles(void);
void D_TurnZOn(void);
void D_WarpScreen(void);

void D_FillRect(vrect_t *vrect, int32_t color);
void D_DrawRect(void);
void D_UpdateRects(vrect_t *prect);

// currently for internal use only, and should be a do-nothing function in
// hardware drivers
// FIXME: this should go away
void D_PolysetUpdateTables(void);

// these are currently for internal use only, and should not be used by drivers
extern int32_t r_skydirect;
extern uint8_t *r_skysource;

// transparency types for D_DrawRect ()
#define DR_SOLID 0
#define DR_TRANSPARENT 1

// !!! must be kept the same as in quakeasm.h !!!
#define TRANSPARENT_COLOR 0xFF

extern void *acolormap; // FIXME: should go away

//=======================================================================//

// callbacks to Quake

typedef struct
{
    pixel_t *surfdat; // destination for generated surface
    int32_t rowbytes; // destination logical width in bytes
    msurface_t *surf; // description for surface to generate
    fixed8_t lightadj[MAXLIGHTMAPS];
    // adjust for lightmap levels for dynamic lighting
    texture_t *texture; // corrected for animating textures
    int32_t surfmip;    // mipmapped ratio of surface texels / world pixels
    int32_t surfwidth;  // in mipmapped texels
    int32_t surfheight; // in mipmapped texels
} drawsurf_t;

extern drawsurf_t r_drawsurf;

void R_DrawSurface(void);
void R_GenTile(msurface_t *psurf, void *pdest);

// !!! if this is changed, it must be changed in d_ifacea.h too !!!
#define TURB_TEX_SIZE 64 // base turbulent texture size

// !!! if this is changed, it must be changed in d_ifacea.h too !!!
#define CYCLE 128 // turbulent cycle size

#define TILE_SIZE 128 // size of textures generated by R_GenTiledSurf

#define SKYSHIFT 7
#define SKYSIZE (1 << SKYSHIFT)
#define SKYMASK (SKYSIZE - 1)

extern float skyspeed, skyspeed2;
extern float skytime;

extern int32_t c_surf;
extern vrect_t scr_vrect;

extern uint8_t *r_warpbuffer;
