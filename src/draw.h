
// draw.h -- these are the only functions outside the refresh allowed
// to touch the vid buffer

extern qpic_t *draw_disc; // also used on sbar

void Draw_Init(void);
void Draw_Character(int32_t x, int32_t y, int32_t num);
void Draw_DebugChar(char num);
void Draw_Pic(int32_t x, int32_t y, qpic_t *pic);
void Draw_TransPic(int32_t x, int32_t y, qpic_t *pic);
void Draw_TransPicTranslate(int32_t x, int32_t y, qpic_t *pic,
                            uint8_t *translation);
void Draw_ConsoleBackground(int32_t lines);
void Draw_BeginDisc(void);
void Draw_EndDisc(void);
void Draw_TileClear(int32_t x, int32_t y, int32_t w, int32_t h);
void Draw_Fill(int32_t x, int32_t y, int32_t w, int32_t h, int32_t c);
void Draw_FadeScreen(void);
void Draw_String(int32_t x, int32_t y, char *str);
qpic_t *Draw_PicFromWad(char *name);
qpic_t *Draw_CachePic(char *path);
