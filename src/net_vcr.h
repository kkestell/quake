// net_vcr.h

#define VCR_OP_CONNECT					1
#define VCR_OP_GETMESSAGE				2
#define VCR_OP_SENDMESSAGE				3
#define VCR_OP_CANSENDMESSAGE			4
#define VCR_MAX_MESSAGE					4

int32_t			VCR_Init (void);
void		VCR_Listen (bool state);
void		VCR_SearchForHosts (bool xmit);
qsocket_t 	*VCR_Connect (char *host);
qsocket_t 	*VCR_CheckNewConnections (void);
int32_t			VCR_GetMessage (qsocket_t *sock);
int32_t			VCR_SendMessage (qsocket_t *sock, sizebuf_t *data);
bool	VCR_CanSendMessage (qsocket_t *sock);
void		VCR_Close (qsocket_t *sock);
void		VCR_Shutdown (void);
