
// refresh.h -- public interface to refresh functions

#define MAXCLIPPLANES 11

#define TOP_RANGE 16 // soldier uniform colors
#define BOTTOM_RANGE 96

//=============================================================================

typedef struct efrag_s
{
    struct mleaf_s *leaf;
    struct efrag_s *leafnext;
    struct entity_s *entity;
    struct efrag_s *entnext;
} efrag_t;

typedef struct entity_s
{
    bool forcelink; // model changed

    int32_t update_type;

    entity_state_t baseline; // to fill in defaults in updates

    double msgtime;        // time of last update
    vec3_t msg_origins[2]; // last two updates (0 is newest)
    vec3_t origin;
    vec3_t msg_angles[2]; // last two updates (0 is newest)
    vec3_t angles;
    struct model_s *model; // NULL = no model
    struct efrag_s *efrag; // linked list of efrags
    int32_t frame;
    float syncbase; // for client-side animations
    uint8_t *colormap;
    int32_t effects;  // light, particals, etc
    int32_t skinnum;  // for Alias models
    int32_t visframe; // last frame this entity was
                      //  found in an active leaf

    int32_t dlightframe; // dynamic lighting
    int32_t dlightbits;

    // FIXME: could turn these into a union
    int32_t trivial_accept;
    struct mnode_s *topnode; // for bmodels, first world node
                             //  that splits bmodel, or NULL if
                             //  not split
} entity_t;

// !!! if this is changed, it must be changed in asm_draw.h too !!!
typedef struct
{
    vrect_t vrect;                             // subwindow in video for refresh
                                               // FIXME: not need vrect next field here?
    vrect_t aliasvrect;                        // scaled Alias version
    int32_t vrectright, vrectbottom;           // right & bottom screen coords
    int32_t aliasvrectright, aliasvrectbottom; // scaled Alias versions
    float vrectrightedge;                      // rightmost right edge we care about,
                                               //  for use in edge list
    float fvrectx, fvrecty;                    // for floating-point compares
    float fvrectx_adj, fvrecty_adj;            // left and top edges, for clamping
    int32_t vrect_x_adj_shift20;               // (vrect.x + 0.5 - epsilon) << 20
    int32_t vrectright_adj_shift20;            // (vrectright + 0.5 - epsilon) << 20
    float fvrectright_adj, fvrectbottom_adj;
    // right and bottom edges, for clamping
    float fvrectright;           // rightmost edge, for Alias clamping
    float fvrectbottom;          // bottommost edge, for Alias clamping
    float horizontalFieldOfView; // at Z = 1.0, this many X is visible
                                 // 2.0 = 90 degrees
    float xOrigin;               // should probably allways be 0.5
    float yOrigin;               // between be around 0.3 to 0.5

    vec3_t vieworg;
    vec3_t viewangles;

    float fov_x, fov_y;

    int32_t ambientlight;
} refdef_t;

//
// refresh
//
extern int32_t reinit_surfcache;

extern refdef_t r_refdef;
extern vec3_t r_origin, vpn, vright, vup;

extern struct texture_s *r_notexture_mip;

void R_Init(void);
void R_InitTextures(void);
void R_InitEfrags(void);
void R_RenderView(void); // must set r_refdef first
void R_ViewChanged(vrect_t *pvrect, int32_t lineadj, float aspect);
// called whenever r_refdef or vid change
void R_InitSky(struct texture_s *mt); // called at level load

void R_AddEfrags(entity_t *ent);
void R_RemoveEfrags(entity_t *ent);

void R_NewMap(void);

void R_ParseParticleEffect(void);
void R_RunParticleEffect(vec3_t org, vec3_t dir, int32_t color, int32_t count);
void R_RocketTrail(vec3_t start, vec3_t end, int32_t type);

void R_EntityParticles(entity_t *ent);
void R_BlobExplosion(vec3_t org);
void R_ParticleExplosion(vec3_t org);
void R_ParticleExplosion2(vec3_t org, int32_t colorStart, int32_t colorLength);
void R_LavaSplash(vec3_t org);
void R_TeleportSplash(vec3_t org);

void R_PushDlights(void);

//
// surface cache related
//
extern int32_t reinit_surfcache; // if 1, surface cache is currently empty and
extern bool r_cache_thrash;      // set if thrashing the surface cache

int32_t D_SurfaceCacheForRes(int32_t width, int32_t height);
void D_FlushCaches(void);
void D_DeleteSurfaceCache(void);
void D_InitCaches(void *buffer, int32_t size);
void R_SetVrect(vrect_t *pvrect, vrect_t *pvrectin, int32_t lineadj);
