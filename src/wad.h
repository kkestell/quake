// wad.h

//===============
//   TYPES
//===============

#define CMP_NONE 0
#define CMP_LZSS 1

#define TYP_NONE 0
#define TYP_LABEL 1

#define TYP_LUMPY 64 // 64 + grab command number
#define TYP_PALETTE 64
#define TYP_QTEX 65
#define TYP_QPIC 66
#define TYP_SOUND 67
#define TYP_MIPTEX 68

typedef struct
{
    int32_t width, height;
    uint8_t data[4]; // variably sized
} qpic_t;

typedef struct
{
    char identification[4]; // should be WAD2 or 2DAW
    int32_t numlumps;
    int32_t infotableofs;
} wadinfo_t;

typedef struct
{
    int32_t filepos;
    int32_t disksize;
    int32_t size; // uncompressed
    char type;
    char compression;
    char pad1, pad2;
    char name[16]; // must be null terminated
} lumpinfo_t;

extern int32_t wad_numlumps;
extern lumpinfo_t *wad_lumps;
extern uint8_t *wad_base;

void W_LoadWadFile(char *filename);
void W_CleanupName(char *in, char *out);
lumpinfo_t *W_GetLumpinfo(char *name);
void *W_GetLumpName(char *name);
void *W_GetLumpNum(int32_t num);

void SwapPic(qpic_t *pic);
