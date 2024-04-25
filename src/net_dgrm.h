// net_dgrm.h

int32_t Datagram_Init(void);
void Datagram_Listen(bool state);
void Datagram_SearchForHosts(bool xmit);
qsocket_t *Datagram_Connect(char *host);
qsocket_t *Datagram_CheckNewConnections(void);
int32_t Datagram_GetMessage(qsocket_t *sock);
int32_t Datagram_SendMessage(qsocket_t *sock, sizebuf_t *data);
int32_t Datagram_SendUnreliableMessage(qsocket_t *sock, sizebuf_t *data);
bool Datagram_CanSendMessage(qsocket_t *sock);
bool Datagram_CanSendUnreliableMessage(qsocket_t *sock);
void Datagram_Close(qsocket_t *sock);
void Datagram_Shutdown(void);
