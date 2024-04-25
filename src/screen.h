// screen.h

void SCR_Init (void);

void SCR_UpdateScreen (void);


void SCR_SizeUp (void);
void SCR_SizeDown (void);
void SCR_BringDownConsole (void);
void SCR_CenterPrint (char *str);

void SCR_BeginLoadingPlaque (void);
void SCR_EndLoadingPlaque (void);

int32_t SCR_ModalMessage (char *text);

extern	float		scr_con_current;
extern	float		scr_conlines;		// lines of console to display

extern	int32_t			scr_fullupdate;	// set to 0 to force full redraw
extern	int32_t			sb_lines;

extern	int32_t			clearnotify;	// set to 0 whenever notify text is drawn
extern	bool	scr_disabled_for_loading;
extern	bool	scr_skipupdate;

extern	cvar_t		scr_viewsize;

extern cvar_t scr_viewsize;

// only the refresh window will be updated unless these variables are flagged 
extern	int32_t			scr_copytop;
extern	int32_t			scr_copyeverything;

extern bool		block_drawing;

void SCR_UpdateWholeScreen (void);
