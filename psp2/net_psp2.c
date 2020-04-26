// net_psp2.c

#include "../qcommon/qcommon.h"

#include <unistd.h>
#include <vitasdk.h>

#ifdef NeXT
#include <libc.h>
#endif

netadr_t	net_local_adr;

int net_hostport = 26000;
int errno;

#define	LOOPBACK	0x7f000001

#define	MAX_LOOPBACK	4

typedef struct
{
	byte	data[MAX_MSGLEN];
	int		datalen;
} loopmsg_t;

typedef struct
{
	loopmsg_t	msgs[MAX_LOOPBACK];
	int			get, send;
} loopback_t;

loopback_t	loopbacks[2];
int			ip_sockets[2];
int			ipx_sockets[2];

int NET_Socket (char *net_interface, int port);
char *NET_ErrorString (void);

#define MAX_NAME 512
struct hostent{
  char  *h_name;         /* official (cannonical) name of host               */
  char **h_aliases;      /* pointer to array of pointers of alias names      */
  int    h_addrtype;     /* host address type: AF_INET                       */
  int    h_length;       /* length of address: 4                             */
  char **h_addr_list;    /* pointer to array of pointers with IPv4 addresses */
};
#define h_addr h_addr_list[0]

// Copy-pasted from xyz code
static struct hostent *gethostbyname(const char *name)
{
    static struct hostent ent;
    static char sname[MAX_NAME] = "";
    static struct SceNetInAddr saddr = { 0 };
    static char *addrlist[2] = { (char *) &saddr, NULL };

    int rid;
    int err;
    rid = sceNetResolverCreate("resolver", NULL, 0);
    if(rid < 0) {
        return NULL;
    }

    err = sceNetResolverStartNtoa(rid, name, &saddr, 0, 0, 0);
    sceNetResolverDestroy(rid);
    if(err < 0) {
        return NULL;
    }

    ent.h_name = sname;
    ent.h_aliases = 0;
    ent.h_addrtype = SCE_NET_AF_INET;
    ent.h_length = sizeof(struct SceNetInAddr);
    ent.h_addr_list = addrlist;
    ent.h_addr = addrlist[0];

    return &ent;
}

//=============================================================================

void NetadrToSockadr (netadr_t *a, struct SceNetSockaddrIn *s)
{
	memset (s, 0, sizeof(*s));

	if (a->type == NA_BROADCAST)
	{
		s->sin_family = SCE_NET_AF_INET;

		s->sin_port = a->port;
		s->sin_addr.s_addr = -1;
	}
	else if (a->type == NA_IP)
	{
		s->sin_family = SCE_NET_AF_INET;

		s->sin_addr.s_addr = *(int *)&a->ip;
		s->sin_port = a->port;
	}
}

void SockadrToNetadr (struct SceNetSockaddrIn *s, netadr_t *a)
{
	*(int *)&a->ip = s->sin_addr.s_addr;
	a->port = s->sin_port;
	a->type = NA_IP;
}


qboolean	NET_CompareAdr (netadr_t a, netadr_t b)
{
	if (a.ip[0] == b.ip[0] && a.ip[1] == b.ip[1] && a.ip[2] == b.ip[2] && a.ip[3] == b.ip[3] && a.port == b.port)
		return true;
	return false;
}

/*
===================
NET_CompareBaseAdr

Compares without the port
===================
*/
qboolean	NET_CompareBaseAdr (netadr_t a, netadr_t b)
{
	if (a.type != b.type)
		return false;

	if (a.type == NA_LOOPBACK)
		return true;

	if (a.type == NA_IP)
	{
		if (a.ip[0] == b.ip[0] && a.ip[1] == b.ip[1] && a.ip[2] == b.ip[2] && a.ip[3] == b.ip[3])
			return true;
		return false;
	}

	if (a.type == NA_IPX)
	{
		if ((memcmp(a.ipx, b.ipx, 10) == 0))
			return true;
		return false;
	}
	
	return false;
}

char	*NET_AdrToString (netadr_t a)
{
	static	char	s[64];
	
	Com_sprintf (s, sizeof(s), "%i.%i.%i.%i:%i", a.ip[0], a.ip[1], a.ip[2], a.ip[3], sceNetNtohs(a.port));

	return s;
}

char	*NET_BaseAdrToString (netadr_t a)
{
	static	char	s[64];
	
	Com_sprintf (s, sizeof(s), "%i.%i.%i.%i", a.ip[0], a.ip[1], a.ip[2], a.ip[3]);

	return s;
}

/*
=============
NET_StringToAdr

localhost
idnewt
idnewt:28000
192.246.40.70
192.246.40.70:28000
=============
*/
qboolean	NET_StringToSockaddr (char *s, struct SceNetSockaddr *sadr)
{
	struct hostent	*h;
	char	*colon;
	char	copy[128];
	
	memset (sadr, 0, sizeof(*sadr));
	((struct SceNetSockaddrIn *)sadr)->sin_family = SCE_NET_AF_INET;
	
	((struct SceNetSockaddrIn *)sadr)->sin_port = 0;

	strcpy (copy, s);
	// strip off a trailing :port if present
	for (colon = copy ; *colon ; colon++)
		if (*colon == ':')
		{
			*colon = 0;
			((struct SceNetSockaddrIn *)sadr)->sin_port = sceNetHtons((short)atoi(colon+1));	
		}
	
	if (copy[0] >= '0' && copy[0] <= '9')
	{
		sceNetInetPton(SCE_NET_AF_INET, copy, (int *)&((struct SceNetSockaddrIn *)sadr)->sin_addr);
	}
	else
	{
		if (! (h = gethostbyname(copy)) )
			return 0;
		*(int *)&((struct SceNetSockaddrIn *)sadr)->sin_addr.s_addr = *(int *)h->h_addr_list[0];
	}
	
	return true;
}

/*
=============
NET_StringToAdr

localhost
idnewt
idnewt:28000
192.246.40.70
192.246.40.70:28000
=============
*/
qboolean	NET_StringToAdr (char *s, netadr_t *a)
{
	struct SceNetSockaddrIn sadr;
	
	if (!strcmp (s, "localhost"))
	{
		memset (a, 0, sizeof(*a));
		a->type = NA_LOOPBACK;
		return true;
	}

	if (!NET_StringToSockaddr (s, (struct SceNetSockaddr	*)&sadr))
		return false;
	
	SockadrToNetadr (&sadr, a);

	return true;
}


qboolean	NET_IsLocalAddress (netadr_t adr)
{
	return NET_CompareAdr (adr, net_local_adr);
}

/*
=============================================================================

LOOPBACK BUFFERS FOR LOCAL PLAYER

=============================================================================
*/

qboolean	NET_GetLoopPacket (netsrc_t sock, netadr_t *net_from, sizebuf_t *net_message)
{
	int		i;
	loopback_t	*loop;

	loop = &loopbacks[sock];

	if (loop->send - loop->get > MAX_LOOPBACK)
		loop->get = loop->send - MAX_LOOPBACK;

	if (loop->get >= loop->send)
		return false;

	i = loop->get & (MAX_LOOPBACK-1);
	loop->get++;

	memcpy (net_message->data, loop->msgs[i].data, loop->msgs[i].datalen);
	net_message->cursize = loop->msgs[i].datalen;
	*net_from = net_local_adr;
	return true;

}


void NET_SendLoopPacket (netsrc_t sock, int length, void *data, netadr_t to)
{
	int		i;
	loopback_t	*loop;

	loop = &loopbacks[sock^1];

	i = loop->send & (MAX_LOOPBACK-1);
	loop->send++;

	memcpy (loop->msgs[i].data, data, length);
	loop->msgs[i].datalen = length;
}

//=============================================================================

qboolean	NET_GetPacket (netsrc_t sock, netadr_t *net_from, sizebuf_t *net_message)
{
	int 	ret;
	struct  SceNetSockaddrIn	from;
	int		fromlen;
	int		net_socket;
	int		protocol;
	int		err;

	if (NET_GetLoopPacket (sock, net_from, net_message))
		return true;

	for (protocol = 0 ; protocol < 2 ; protocol++)
	{
		if (protocol == 0)
			net_socket = ip_sockets[sock];
		else
			net_socket = ipx_sockets[sock];

		if (!net_socket)
			continue;

		fromlen = sizeof(from);
		ret = sceNetRecvfrom(net_socket, net_message->data, net_message->maxsize
			, 0, (struct SceNetSockaddr *)&from, &fromlen);
		
		SockadrToNetadr (&from, net_from);
			
		if (ret < 0)
		{
			err = ret;

			if (err == SCE_NET_EWOULDBLOCK || err == SCE_NET_ECONNREFUSED  || err == SCE_NET_ERROR_EAGAIN)
				continue;
			
			errno = err;
			Com_Printf ("NET_GetPacket: %s", NET_ErrorString());
			continue;
		}

		if (ret == net_message->maxsize)
		{
			Com_Printf ("Oversize packet from %s\n", NET_AdrToString (*net_from));
			continue;
		}

		net_message->cursize = ret;
		SockadrToNetadr (&from, net_from);
		return true;
	}

	return false;
}

//=============================================================================

void NET_SendPacket (netsrc_t sock, int length, void *data, netadr_t to)
{
	int		ret;
	struct SceNetSockaddrIn	addr;
	int		net_socket = 0;

	if ( to.type == NA_LOOPBACK )
	{
		NET_SendLoopPacket (sock, length, data, to);
		return;
	}

	if (to.type == NA_BROADCAST)
	{
		net_socket = ip_sockets[sock];
		if (!net_socket)
			return;
	}
	else if (to.type == NA_IP)
	{
		net_socket = ip_sockets[sock];
		if (!net_socket)
			return;
	}
	else if (to.type == NA_IPX)
	{
		net_socket = ipx_sockets[sock];
		if (!net_socket)
			return;
	}
	else if (to.type == NA_BROADCAST_IPX)
	{
		net_socket = ipx_sockets[sock];
		if (!net_socket)
			return;
	}
	else
		Com_Error (ERR_FATAL, "NET_SendPacket: bad address type");

	NetadrToSockadr (&to, &addr);

	ret = sceNetSendto(net_socket, data, length, 0, (struct SceNetSockaddr *)&addr, sizeof(addr) );
	if (ret < 0)
	{
		errno = ret;
		Com_Printf ("NET_SendPacket ERROR: %i\n", NET_ErrorString());
	}
}


//=============================================================================




/*
====================
NET_OpenIP
====================
*/
void NET_OpenIP (void)
{
	cvar_t	*port, *ip;

	port = Cvar_Get ("port", va("%i", PORT_SERVER), CVAR_NOSET);
	ip = Cvar_Get ("ip", "localhost", CVAR_NOSET);

	if (!ip_sockets[NS_SERVER])
		ip_sockets[NS_SERVER] = NET_Socket (ip->string, port->value);
	if (!ip_sockets[NS_CLIENT])
		ip_sockets[NS_CLIENT] = NET_Socket (ip->string, PORT_ANY);
}

/*
====================
NET_OpenIPX
====================
*/
void NET_OpenIPX (void)
{
}


/*
====================
NET_Config

A single player game will only use the loopback code
====================
*/
void	NET_Config (qboolean multiplayer)
{
	int		i;

	if (!multiplayer)
	{	// shut down any existing sockets
		for (i=0 ; i<2 ; i++)
		{
			if (ip_sockets[i])
			{
				sceNetSocketClose (ip_sockets[i]);
				ip_sockets[i] = 0;
			}
			if (ipx_sockets[i])
			{
				sceNetSocketClose (ipx_sockets[i]);
				ipx_sockets[i] = 0;
			}
		}
	}
	else
	{	// open sockets
		NET_OpenIP ();
		NET_OpenIPX ();
	}
}


//===================================================================


/*
====================
NET_Init
====================
*/
static void *net_memory = NULL;
#define NET_INIT_SIZE 1*1024*1024
void NET_Init (void)
{
	
	SceNetInitParam initparam;
	
	// Start SceNet & SceNetCtl
	int ret = sceNetShowNetstat();
	if (ret == SCE_NET_ERROR_ENOTINIT) {
		net_memory = malloc(NET_INIT_SIZE);

		initparam.memory = net_memory;
		initparam.size = NET_INIT_SIZE;
		initparam.flags = 0;

		ret = sceNetInit(&initparam);
		if (ret < 0) return;
		
	}
	ret = sceNetCtlInit();
	if (ret < 0){
		sceNetTerm();
		free(net_memory);
		return;
	}
	
}


/*
====================
NET_Socket
====================
*/
int NET_Socket (char *net_interface, int port)
{
	int newsocket;
	struct SceNetSockaddrIn address;
	uint32_t _true = true;
	int	i = 1;
	int ret;
	
	if ((newsocket = sceNetSocket("socket", SCE_NET_AF_INET, SCE_NET_SOCK_DGRAM, SCE_NET_IPPROTO_UDP)) < 0)
	{
		errno = newsocket;
		Com_Printf ("ERROR: UDP_OpenSocket: socket:%s\n", NET_ErrorString());
		return 0;
	}

	// make it non-blocking
	if ((ret=sceNetSetsockopt(newsocket, SCE_NET_SOL_SOCKET, SCE_NET_SO_NBIO, &_true, sizeof(uint32_t))) < 0)
	{
		errno = ret;
		Com_Printf ("ERROR: UDP_OpenSocket: setsockopt SO_NBIO:%s\n", NET_ErrorString());
		return 0;
	}

	// make it broadcast capable
	if ((ret=sceNetSetsockopt(newsocket, SCE_NET_SOL_SOCKET, SCE_NET_SO_BROADCAST, (char *)&i, sizeof(i))) < 0)
	{
		errno = ret;
		Com_Printf ("ERROR: UDP_OpenSocket: setsockopt SO_BROADCAST:%s\n", NET_ErrorString());
		return 0;
	}

	if (!net_interface || !net_interface[0] || !strcasecmp(net_interface, "localhost"))
		address.sin_addr.s_addr = SCE_NET_INADDR_ANY;
	else
		NET_StringToSockaddr (net_interface, (struct SceNetSockaddr *)&address);

	if (port == PORT_ANY)
		address.sin_port = sceNetHtons(net_hostport++);
	else
		address.sin_port = sceNetHtons((short)port);

	address.sin_family = SCE_NET_AF_INET;

	if((ret=sceNetBind(newsocket, (void *)&address, sizeof(address))) < 0)
	{
		errno = ret;
		Com_Printf ("ERROR: UDP_OpenSocket: bind: %s\n", NET_ErrorString());
		sceNetSocketClose (newsocket);
		return 0;
	}

	return newsocket;
}


/*
====================
NET_Shutdown
====================
*/
void	NET_Shutdown (void)
{
	NET_Config (false);	// close sockets
	sceNetTerm();
	free(net_memory);
}


/*
====================
NET_ErrorString
====================
*/
char err[256];
char *NET_ErrorString (void)
{
	sprintf(err, "errorcode %d", errno);
	return err;
}

// sleeps msec or until net socket is ready
void NET_Sleep(int msec)
{
}

