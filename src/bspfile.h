#include <stdint.h>

// upper design bounds

#define MAX_MAP_HULLS 4

#define MAX_MAP_MODELS 256
#define MAX_MAP_BRUSHES 4096
#define MAX_MAP_ENTITIES 1024
#define MAX_MAP_ENTSTRING 65536

#define MAX_MAP_PLANES 32767
#define MAX_MAP_NODES 32767     // because negative shorts are contents
#define MAX_MAP_CLIPNODES 32767 //
#define MAX_MAP_LEAFS 8192
#define MAX_MAP_VERTS 65535
#define MAX_MAP_FACES 65535
#define MAX_MAP_MARKSURFACES 65535
#define MAX_MAP_TEXINFO 4096
#define MAX_MAP_EDGES 256000
#define MAX_MAP_SURFEDGES 512000
#define MAX_MAP_TEXTURES 512
#define MAX_MAP_MIPTEX 0x200000
#define MAX_MAP_LIGHTING 0x100000
#define MAX_MAP_VISIBILITY 0x100000

#define MAX_MAP_PORTALS 65536

// key / value pair sizes

#define MAX_KEY 32
#define MAX_VALUE 1024

//=============================================================================

#define BSPVERSION 29
#define TOOLVERSION 2

typedef struct
{
    int32_t fileofs, filelen;
} lump_t;

#define LUMP_ENTITIES 0
#define LUMP_PLANES 1
#define LUMP_TEXTURES 2
#define LUMP_VERTEXES 3
#define LUMP_VISIBILITY 4
#define LUMP_NODES 5
#define LUMP_TEXINFO 6
#define LUMP_FACES 7
#define LUMP_LIGHTING 8
#define LUMP_CLIPNODES 9
#define LUMP_LEAFS 10
#define LUMP_MARKSURFACES 11
#define LUMP_EDGES 12
#define LUMP_SURFEDGES 13
#define LUMP_MODELS 14

#define HEADER_LUMPS 15

typedef struct
{
    float mins[3], maxs[3];
    float origin[3];
    int32_t headnode[MAX_MAP_HULLS];
    int32_t visleafs; // not including the solid leaf 0
    int32_t firstface, numfaces;
} dmodel_t;

typedef struct
{
    int32_t version;
    lump_t lumps[HEADER_LUMPS];
} dheader_t;

typedef struct
{
    int32_t nummiptex;
    int32_t dataofs[4]; // [nummiptex]
} dmiptexlump_t;

#define MIPLEVELS 4
typedef struct miptex_s
{
    int8_t name[16];
    uint32_t width, height;
    uint32_t offsets[MIPLEVELS]; // four mip maps stored
} miptex_t;

typedef struct
{
    float point[3];
} dvertex_t;

// 0-2 are axial planes
#define PLANE_X 0
#define PLANE_Y 1
#define PLANE_Z 2

// 3-5 are non-axial planes snapped to the nearest
#define PLANE_ANYX 3
#define PLANE_ANYY 4
#define PLANE_ANYZ 5

typedef struct
{
    float normal[3];
    float dist;
    int32_t type; // PLANE_X - PLANE_ANYZ ?remove? trivial to regenerate
} dplane_t;

#define CONTENTS_EMPTY -1
#define CONTENTS_SOLID -2
#define CONTENTS_WATER -3
#define CONTENTS_SLIME -4
#define CONTENTS_LAVA -5
#define CONTENTS_SKY -6
#define CONTENTS_ORIGIN -7 // removed at csg time
#define CONTENTS_CLIP -8   // changed to contents_solid

#define CONTENTS_CURRENT_0 -9
#define CONTENTS_CURRENT_90 -10
#define CONTENTS_CURRENT_180 -11
#define CONTENTS_CURRENT_270 -12
#define CONTENTS_CURRENT_UP -13
#define CONTENTS_CURRENT_DOWN -14

// !!! if this is changed, it must be changed in asm_i386.h too !!!
typedef struct
{
    int32_t planenum;
    int16_t children[2]; // negative numbers are -(leafs+1), not nodes
    int16_t mins[3];     // for sphere culling
    int16_t maxs[3];
    uint16_t firstface;
    uint16_t numfaces; // counting both sides
} dnode_t;

typedef struct
{
    int32_t planenum;
    int16_t children[2]; // negative numbers are contents
} dclipnode_t;

typedef struct texinfo_s
{
    float vecs[2][4]; // [s/t][xyz offset]
    int32_t miptex;
    int32_t flags;
} texinfo_t;
#define TEX_SPECIAL 1 // sky or slime, no lightmap or 256 subdivision

// note that edge 0 is never used, because negative edge nums are used for
// counterclockwise use of the edge in a face
typedef struct
{
    uint16_t v[2]; // vertex numbers
} dedge_t;

#define MAXLIGHTMAPS 4
typedef struct
{
    int16_t planenum;
    int16_t side;

    int32_t firstedge; // we must support > 64k edges
    int16_t numedges;
    int16_t texinfo;

    // lighting info
    uint8_t styles[MAXLIGHTMAPS];
    int32_t lightofs; // start of [numstyles*surfsize] samples
} dface_t;

#define AMBIENT_WATER 0
#define AMBIENT_SKY 1
#define AMBIENT_SLIME 2
#define AMBIENT_LAVA 3

#define NUM_AMBIENTS 4 // automatic ambient sounds

// leaf 0 is the generic CONTENTS_SOLID leaf, used for all solid areas
// all other leafs need visibility info
typedef struct
{
    int32_t contents;
    int32_t visofs; // -1 = no visibility info

    int16_t mins[3]; // for frustum culling
    int16_t maxs[3];

    uint16_t firstmarksurface;
    uint16_t nummarksurfaces;

    uint8_t ambient_level[NUM_AMBIENTS];
} dleaf_t;

//============================================================================

#ifndef QUAKE_GAME

#define ANGLE_UP -1
#define ANGLE_DOWN -2

// the utilities get to be lazy and just use large static arrays

extern int32_t nummodels;
extern dmodel_t dmodels[MAX_MAP_MODELS];

extern int32_t visdatasize;
extern uint8_t dvisdata[MAX_MAP_VISIBILITY];

extern int32_t lightdatasize;
extern uint8_t dlightdata[MAX_MAP_LIGHTING];

extern int32_t texdatasize;
extern uint8_t dtexdata[MAX_MAP_MIPTEX]; // (dmiptexlump_t)

extern int32_t entdatasize;
extern int8_t dentdata[MAX_MAP_ENTSTRING];

extern int32_t numleafs;
extern dleaf_t dleafs[MAX_MAP_LEAFS];

extern int32_t numplanes;
extern dplane_t dplanes[MAX_MAP_PLANES];

extern int32_t numvertexes;
extern dvertex_t dvertexes[MAX_MAP_VERTS];

extern int32_t numnodes;
extern dnode_t dnodes[MAX_MAP_NODES];

extern int32_t numtexinfo;
extern texinfo_t texinfo[MAX_MAP_TEXINFO];

extern int32_t numfaces;
extern dface_t dfaces[MAX_MAP_FACES];

extern int32_t numclipnodes;
extern dclipnode_t dclipnodes[MAX_MAP_CLIPNODES];

extern int32_t numedges;
extern dedge_t dedges[MAX_MAP_EDGES];

extern int32_t nummarksurfaces;
extern uint16_t dmarksurfaces[MAX_MAP_MARKSURFACES];

extern int32_t numsurfedges;
extern int32_t dsurfedges[MAX_MAP_SURFEDGES];

void DecompressVis(uint8_t *in, uint8_t *decompressed);
int32_t CompressVis(uint8_t *vis, uint8_t *dest);

void LoadBSPFile(int8_t *filename);
void WriteBSPFile(int8_t *filename);
void PrintBSPFileSizes(void);

//===============

typedef struct epair_s
{
    struct epair_s *next;
    int8_t *key;
    int8_t *value;
} epair_t;

typedef struct
{
    vec3_t origin;
    int32_t firstbrush;
    int32_t numbrushes;
    epair_t *epairs;
} entity_t;

extern int32_t num_entities;
extern entity_t entities[MAX_MAP_ENTITIES];

void ParseEntities(void);
void UnparseEntities(void);

void SetKeyValue(entity_t *ent, int8_t *key, int8_t *value);
int8_t *ValueForKey(entity_t *ent, int8_t *key);
// will return "" if not present

vec_t FloatForKey(entity_t *ent, int8_t *key);
void GetVectorForKey(entity_t *ent, int8_t *key, vec3_t vec);

epair_t *ParseEpair(void);

#endif
