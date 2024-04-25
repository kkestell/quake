// net_ser.h

int32_t			Serial_Init (void);
void		Serial_Listen (bool state);
void		Serial_SearchForHosts (bool xmit);
qsocket_t	*Serial_Connect (char *host);
qsocket_t 	*Serial_CheckNewConnections (void);
int32_t			Serial_GetMessage (qsocket_t *sock);
int32_t			Serial_SendMessage (qsocket_t *sock, sizebuf_t *data);
int32_t			Serial_SendUnreliableMessage (qsocket_t *sock, sizebuf_t *data);
bool	Serial_CanSendMessage (qsocket_t *sock);
bool	Serial_CanSendUnreliableMessage (qsocket_t *sock);
void		Serial_Close (qsocket_t *sock);
void		Serial_Shutdown (void);
