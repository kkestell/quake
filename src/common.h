// comndef.h  -- general definitions

#include <stdint.h>

#undef true
#undef false

typedef enum {false, true}	bool;

//============================================================================

typedef struct sizebuf_s
{
	bool	allowoverflow;	// if false, do a Sys_Error
	bool	overflowed;		// set to true if the buffer size failed
	uint8_t	*data;
	int32_t		maxsize;
	int32_t		cursize;
} sizebuf_t;

void SZ_Alloc (sizebuf_t *buf, int32_t startsize);
void SZ_Free (sizebuf_t *buf);
void SZ_Clear (sizebuf_t *buf);
void *SZ_GetSpace (sizebuf_t *buf, int32_t length);
void SZ_Write (sizebuf_t *buf, void *data, int32_t length);
void SZ_Print (sizebuf_t *buf, char *data);	// strcats onto the sizebuf

//============================================================================

typedef struct link_s
{
	struct link_s	*prev, *next;
} link_t;


void ClearLink (link_t *l);
void RemoveLink (link_t *l);
void InsertLinkBefore (link_t *l, link_t *before);
void InsertLinkAfter (link_t *l, link_t *after);

// (type *)STRUCT_FROM_LINK(link_t *link, type, member)
// ent = STRUCT_FROM_LINK(link,entity_t,order)
// FIXME: remove this mess!
#define	STRUCT_FROM_LINK(l,t,m) ((t *)((uint8_t *)l - (int32_t)&(((t *)0)->m)))

//============================================================================

#ifndef NULL
#define NULL ((void *)0)
#endif

#define Q_MAXCHAR ((char)0x7f)
#define Q_MAXSHORT ((int16_t)0x7fff)
#define Q_MAXINT	((int32_t)0x7fffffff)
#define Q_MAXLONG ((int32_t)0x7fffffff)
#define Q_MAXFLOAT ((int32_t)0x7fffffff)

#define Q_MINCHAR ((char)0x80)
#define Q_MINSHORT ((int16_t)0x8000)
#define Q_MININT 	((int32_t)0x80000000)
#define Q_MINLONG ((int32_t)0x80000000)
#define Q_MINFLOAT ((int32_t)0x7fffffff)

//============================================================================

extern	bool		bigendien;

extern	int16_t	(*BigShort) (int16_t l);
extern	int16_t	(*LittleShort) (int16_t l);
extern	int32_t	(*BigLong) (int32_t l);
extern	int32_t	(*LittleLong) (int32_t l);
extern	float	(*BigFloat) (float l);
extern	float	(*LittleFloat) (float l);

//============================================================================

void MSG_WriteChar (sizebuf_t *sb, int32_t c);
void MSG_WriteByte (sizebuf_t *sb, int32_t c);
void MSG_WriteShort (sizebuf_t *sb, int32_t c);
void MSG_WriteLong (sizebuf_t *sb, int32_t c);
void MSG_WriteFloat (sizebuf_t *sb, float f);
void MSG_WriteString (sizebuf_t *sb, char *s);
void MSG_WriteCoord (sizebuf_t *sb, float f);
void MSG_WriteAngle (sizebuf_t *sb, float f);

extern	int32_t			msg_readcount;
extern	bool	msg_badread;		// set if a read goes beyond end of message

void MSG_BeginReading (void);
int32_t MSG_ReadChar (void);
int32_t MSG_ReadByte (void);
int32_t MSG_ReadShort (void);
int32_t MSG_ReadLong (void);
float MSG_ReadFloat (void);
char *MSG_ReadString (void);

float MSG_ReadCoord (void);
float MSG_ReadAngle (void);

//============================================================================

void Q_memset (void *dest, int32_t fill, int32_t count);
void Q_memcpy (void *dest, void *src, int32_t count);
int32_t Q_memcmp (void *m1, void *m2, int32_t count);
void Q_strcpy (char *dest, char *src);
void Q_strncpy (char *dest, char *src, int32_t count);
int32_t Q_strlen (char *str);
char *Q_strrchr (char *s, char c);
void Q_strcat (char *dest, char *src);
int32_t Q_strcmp (char *s1, char *s2);
int32_t Q_strncmp (char *s1, char *s2, int32_t count);
int32_t Q_strcasecmp (char *s1, char *s2);
int32_t Q_strncasecmp (char *s1, char *s2, int32_t n);
int32_t	Q_atoi (char *str);
float Q_atof (char *str);

//============================================================================

extern	char		com_token[1024];
extern	bool	com_eof;

char *COM_Parse (char *data);


extern	int32_t		com_argc;
extern	char	**com_argv;

int32_t COM_CheckParm (char *parm);
void COM_Init (char *path);
void COM_InitArgv (int32_t argc, char **argv);

char *COM_SkipPath (char *pathname);
void COM_StripExtension (char *in, char *out);
void COM_FileBase (char *in, char *out);
void COM_DefaultExtension (char *path, char *extension);

char	*va(char *format, ...);
// does a varargs printf into a temp buffer


//============================================================================

extern int32_t com_filesize;
struct cache_user_s;

extern	char	com_gamedir[MAX_OSPATH];

void COM_WriteFile (char *filename, void *data, int32_t len);
int32_t COM_OpenFile (char *filename, int32_t *hndl);
int32_t COM_FOpenFile (char *filename, FILE **file);
void COM_CloseFile (int32_t h);

uint8_t *COM_LoadStackFile (char *path, void *buffer, int32_t bufsize);
uint8_t *COM_LoadTempFile (char *path);
uint8_t *COM_LoadHunkFile (char *path);
void COM_LoadCacheFile (char *path, struct cache_user_s *cu);


extern	struct cvar_s	registered;

extern bool		standard_quake, rogue, hipnotic;
