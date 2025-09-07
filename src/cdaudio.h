#include <stdbool.h>

int32_t CDAudio_Init(void);
void CDAudio_Play(uint8_t track, bool looping);
void CDAudio_Stop(void);
void CDAudio_Pause(void);
void CDAudio_Resume(void);
void CDAudio_Shutdown(void);
void CDAudio_Update(void);
