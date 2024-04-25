// net.h -- quake's interface to the networking layer

struct qsockaddr
{
	short sa_family;
	unsigned char sa_data[14];
};


#define	NET_NAMELEN			64

#define NET_MAXMESSAGE		8192
#define NET_HEADERSIZE		(2 * sizeof(uint32_t))
#define NET_DATAGRAMSIZE	(MAX_DATAGRAM + NET_HEADERSIZE)

// NetHeader flags
#define NETFLAG_LENGTH_MASK	0x0000ffff
#define NETFLAG_DATA		0x00010000
#define NETFLAG_ACK			0x00020000
#define NETFLAG_NAK			0x00040000
#define NETFLAG_EOM			0x00080000
#define NETFLAG_UNRELIABLE	0x00100000
#define NETFLAG_CTL			0x80000000


#define NET_PROTOCOL_VERSION	3

// This is the network info/connection protocol.  It is used to find Quake
// servers, get info about them, and connect to them.  Once connected, the
// Quake game protocol (documented elsewhere) is used.
//
//
// General notes:
//	game_name is currently always "QUAKE", but is there so this same protocol
//		can be used for future games as well; can you say Quake2?
//
// CCREQ_CONNECT
//		string	game_name				"QUAKE"
//		byte	net_protocol_version	NET_PROTOCOL_VERSION
//
// CCREQ_SERVER_INFO
//		string	game_name				"QUAKE"
//		byte	net_protocol_version	NET_PROTOCOL_VERSION
//
// CCREQ_PLAYER_INFO
//		byte	player_number
//
// CCREQ_RULE_INFO
//		string	rule
//
//
//
// CCREP_ACCEPT
//		long	port
//
// CCREP_REJECT
//		string	reason
//
// CCREP_SERVER_INFO
//		string	server_address
//		string	host_name
//		string	level_name
//		byte	current_players
//		byte	max_players
//		byte	protocol_version	NET_PROTOCOL_VERSION
//
// CCREP_PLAYER_INFO
//		byte	player_number
//		string	name
//		long	colors
//		long	frags
//		long	connect_time
//		string	address
//
// CCREP_RULE_INFO
//		string	rule
//		string	value

//	note:
//		There are two address forms used above.  The short form is just a
//		port number.  The address that goes along with the port is defined as
//		"whatever address you receive this reponse from".  This lets us use
//		the host OS to solve the problem of multiple host addresses (possibly
//		with no routing between them); the host will use the right address
//		when we reply to the inbound connection request.  The long from is
//		a full address and port in a string.  It is used for returning the
//		address of a server that is not running locally.

#define CCREQ_CONNECT		0x01
#define CCREQ_SERVER_INFO	0x02
#define CCREQ_PLAYER_INFO	0x03
#define CCREQ_RULE_INFO		0x04

#define CCREP_ACCEPT		0x81
#define CCREP_REJECT		0x82
#define CCREP_SERVER_INFO	0x83
#define CCREP_PLAYER_INFO	0x84
#define CCREP_RULE_INFO		0x85

typedef struct qsocket_s
{
	struct qsocket_s	*next;
	double			connecttime;
	double			lastMessageTime;
	double			lastSendTime;

	qboolean		disconnected;
	qboolean		canSend;
	qboolean		sendNext;
	
	int32_t				driver;
	int32_t				landriver;
	int32_t				socket;
	void			*driverdata;

	uint32_t	ackSequence;
	uint32_t	sendSequence;
	uint32_t	unreliableSendSequence;
	int32_t				sendMessageLength;
	byte			sendMessage [NET_MAXMESSAGE];

	uint32_t	receiveSequence;
	uint32_t	unreliableReceiveSequence;
	int32_t				receiveMessageLength;
	byte			receiveMessage [NET_MAXMESSAGE];

	struct qsockaddr	addr;
	char				address[NET_NAMELEN];

} qsocket_t;

extern qsocket_t	*net_activeSockets;
extern qsocket_t	*net_freeSockets;
extern int32_t			net_numsockets;

typedef struct
{
	char		*name;
	qboolean	initialized;
	int32_t			controlSock;
	int32_t			(*Init) (void);
	void		(*Shutdown) (void);
	void		(*Listen) (qboolean state);
	int32_t 		(*OpenSocket) (int32_t port);
	int32_t 		(*CloseSocket) (int32_t socket);
	int32_t 		(*Connect) (int32_t socket, struct qsockaddr *addr);
	int32_t 		(*CheckNewConnections) (void);
	int32_t 		(*Read) (int32_t socket, byte *buf, int32_t len, struct qsockaddr *addr);
	int32_t 		(*Write) (int32_t socket, byte *buf, int32_t len, struct qsockaddr *addr);
	int32_t 		(*Broadcast) (int32_t socket, byte *buf, int32_t len);
	char *		(*AddrToString) (struct qsockaddr *addr);
	int32_t 		(*StringToAddr) (char *string, struct qsockaddr *addr);
	int32_t 		(*GetSocketAddr) (int32_t socket, struct qsockaddr *addr);
	int32_t 		(*GetNameFromAddr) (struct qsockaddr *addr, char *name);
	int32_t 		(*GetAddrFromName) (char *name, struct qsockaddr *addr);
	int32_t			(*AddrCompare) (struct qsockaddr *addr1, struct qsockaddr *addr2);
	int32_t			(*GetSocketPort) (struct qsockaddr *addr);
	int32_t			(*SetSocketPort) (struct qsockaddr *addr, int32_t port);
} net_landriver_t;

#define	MAX_NET_DRIVERS		8
extern int32_t 				net_numlandrivers;
extern net_landriver_t	net_landrivers[MAX_NET_DRIVERS];

typedef struct
{
	char		*name;
	qboolean	initialized;
	int32_t			(*Init) (void);
	void		(*Listen) (qboolean state);
	void		(*SearchForHosts) (qboolean xmit);
	qsocket_t	*(*Connect) (char *host);
	qsocket_t 	*(*CheckNewConnections) (void);
	int32_t			(*QGetMessage) (qsocket_t *sock);
	int32_t			(*QSendMessage) (qsocket_t *sock, sizebuf_t *data);
	int32_t			(*SendUnreliableMessage) (qsocket_t *sock, sizebuf_t *data);
	qboolean	(*CanSendMessage) (qsocket_t *sock);
	qboolean	(*CanSendUnreliableMessage) (qsocket_t *sock);
	void		(*Close) (qsocket_t *sock);
	void		(*Shutdown) (void);
	int32_t			controlSock;
} net_driver_t;

extern int32_t			net_numdrivers;
extern net_driver_t	net_drivers[MAX_NET_DRIVERS];

extern int32_t			DEFAULTnet_hostport;
extern int32_t			net_hostport;

extern int32_t net_driverlevel;
extern cvar_t		hostname;
extern char			playername[];
extern int32_t			playercolor;

extern int32_t		messagesSent;
extern int32_t		messagesReceived;
extern int32_t		unreliableMessagesSent;
extern int32_t		unreliableMessagesReceived;

qsocket_t *NET_NewQSocket (void);
void NET_FreeQSocket(qsocket_t *);
double SetNetTime(void);


#define HOSTCACHESIZE	8

typedef struct
{
	char	name[16];
	char	map[16];
	char	cname[32];
	int32_t		users;
	int32_t		maxusers;
	int32_t		driver;
	int32_t		ldriver;
	struct qsockaddr addr;
} hostcache_t;

extern int32_t hostCacheCount;
extern hostcache_t hostcache[HOSTCACHESIZE];

#if !defined(_WIN32 ) && !defined (__linux__) && !defined (__sun__)
#ifndef htonl
extern uint32_t htonl (uint32_t hostlong);
#endif
#ifndef htons
extern unsigned short htons (unsigned short hostshort);
#endif
#ifndef ntohl
extern uint32_t ntohl (uint32_t netlong);
#endif
#ifndef ntohs
extern unsigned short ntohs (unsigned short netshort);
#endif
#endif

#ifdef IDGODS
qboolean IsID(struct qsockaddr *addr);
#endif

//============================================================================
//
// public network functions
//
//============================================================================

extern	double		net_time;
extern	sizebuf_t	net_message;
extern	int32_t			net_activeconnections;

void		NET_Init (void);
void		NET_Shutdown (void);

struct qsocket_s	*NET_CheckNewConnections (void);
// returns a new connection number if there is one pending, else -1

struct qsocket_s	*NET_Connect (char *host);
// called by client to connect to a host.  Returns -1 if not able to

qboolean NET_CanSendMessage (qsocket_t *sock);
// Returns true or false if the given qsocket can currently accept a
// message to be transmitted.

int32_t			NET_GetMessage (struct qsocket_s *sock);
// returns data in net_message sizebuf
// returns 0 if no data is waiting
// returns 1 if a message was received
// returns 2 if an unreliable message was received
// returns -1 if the connection died

int32_t			NET_SendMessage (struct qsocket_s *sock, sizebuf_t *data);
int32_t			NET_SendUnreliableMessage (struct qsocket_s *sock, sizebuf_t *data);
// returns 0 if the message connot be delivered reliably, but the connection
//		is still considered valid
// returns 1 if the message was sent properly
// returns -1 if the connection died

int32_t			NET_SendToAll(sizebuf_t *data, int32_t blocktime);
// This is a reliable *blocking* send to all attached clients.


void		NET_Close (struct qsocket_s *sock);
// if a dead connection is returned by a get or send function, this function
// should be called when it is convenient

// Server calls when a client is kicked off for a game related misbehavior
// like an illegal protocal conversation.  Client calls when disconnecting
// from a server.
// A netcon_t number will not be reused until this function is called for it

void NET_Poll(void);


typedef struct _PollProcedure
{
	struct _PollProcedure	*next;
	double					nextTime;
	void					(*procedure)();
	void					*arg;
} PollProcedure;

void SchedulePollProcedure(PollProcedure *pp, double timeOffset);

extern	qboolean	serialAvailable;
extern	qboolean	ipxAvailable;
extern	qboolean	tcpipAvailable;
extern	char		my_ipx_address[NET_NAMELEN];
extern	char		my_tcpip_address[NET_NAMELEN];
extern void (*GetComPortConfig) (int32_t portNumber, int32_t *port, int32_t *irq, int32_t *baud, qboolean *useModem);
extern void (*SetComPortConfig) (int32_t portNumber, int32_t port, int32_t irq, int32_t baud, qboolean useModem);
extern void (*GetModemConfig) (int32_t portNumber, char *dialType, char *clear, char *init, char *hangup);
extern void (*SetModemConfig) (int32_t portNumber, char *dialType, char *clear, char *init, char *hangup);

extern	qboolean	slistInProgress;
extern	qboolean	slistSilent;
extern	qboolean	slistLocal;

void NET_Slist_f (void);
