
//
// the net drivers should just set the apropriate bits in m_activenet,
// instead of having the menu code look through their internal tables
//
#define MNET_IPX 1
#define MNET_TCP 2

extern int32_t m_activenet;

//
// menus
//
void M_Init(void);
void M_Keydown(int32_t key);
void M_Draw(void);
void M_ToggleMenu_f(void);
