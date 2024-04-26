// sys.h -- non-portable functions

//
// file IO
//

// returns the file size
// return -1 if file is not present
// the file should be in BINARY mode for stupid OSs that care
int32_t Sys_FileOpenRead(char *path, int32_t *hndl);

int32_t Sys_FileOpenWrite(char *path);
void Sys_FileClose(int32_t handle);
void Sys_FileSeek(int32_t handle, int32_t position);
int32_t Sys_FileRead(int32_t handle, void *dest, int32_t count);
int32_t Sys_FileWrite(int32_t handle, void *data, int32_t count);
int32_t Sys_FileTime(char *path);
void Sys_mkdir(char *path);

//
// system IO
//
void Sys_DebugLog(char *file, char *fmt, ...);

void Sys_Error(char *error, ...);
// an error will cause the entire program to exit

void Sys_Printf(char *fmt, ...);
// send text to the console

void Sys_Quit(void);

double Sys_FloatTime(void);

char *Sys_ConsoleInput(void);

void Sys_SendKeyEvents(void);
// Perform Key_Event () callbacks until the input que is empty

void Sys_LowFPPrecision(void);
void Sys_HighFPPrecision(void);
void Sys_SetFPCW(void);
