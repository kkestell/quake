#include <stdbool.h>

extern int32_t con_totallines;
extern int32_t con_backscroll;
extern bool con_forcedup; // because no entities to refresh
extern bool con_initialized;
extern uint8_t *con_chars;
extern int32_t con_notifylines; // scan lines to clear for notify lines

void Con_DrawCharacter(int32_t cx, int32_t line, int32_t num);

void Con_CheckResize(void);
void Con_Init(void);
void Con_DrawConsole(int32_t lines, bool drawinput);
void Con_Print(char *txt);
void Con_Printf(char *fmt, ...);
void Con_DPrintf(char *fmt, ...);
void Con_SafePrintf(char *fmt, ...);
void Con_Clear_f(void);
void Con_DrawNotify(void);
void Con_ClearNotify(void);
void Con_ToggleConsole_f(void);

void Con_NotifyBox(char *text); // during startup for sound / cd warnings
