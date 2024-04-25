// net_loop.h

int32_t			Loop_Init (void);
void		Loop_Listen (bool state);
void		Loop_SearchForHosts (bool xmit);
qsocket_t 	*Loop_Connect (char *host);
qsocket_t 	*Loop_CheckNewConnections (void);
int32_t			Loop_GetMessage (qsocket_t *sock);
int32_t			Loop_SendMessage (qsocket_t *sock, sizebuf_t *data);
int32_t			Loop_SendUnreliableMessage (qsocket_t *sock, sizebuf_t *data);
bool	Loop_CanSendMessage (qsocket_t *sock);
bool	Loop_CanSendUnreliableMessage (qsocket_t *sock);
void		Loop_Close (qsocket_t *sock);
void		Loop_Shutdown (void);
