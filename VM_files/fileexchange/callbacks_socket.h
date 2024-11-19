/*****************************************************************************\
 * Redes Integradas de Telecomunicacoes
 * MIEEC/MEEC/MERSIM - FCT NOVA   2024/2025
 *
 * callbacks_socket.h
 *
 * Header file of functions that handle UDP/TCP socket management and UDP communication
 *
 * @author  Luis Bernardo
\*****************************************************************************/
#ifndef INCL_CALLBACKS_SOCKET_H
#define INCL_CALLBACKS_SOCKET_H

#include <gtk/gtk.h>
#include <glib.h>
#include "gui.h"

#ifndef FALSE
#define FALSE 0
#endif
#ifndef TRUE
#define TRUE (!FALSE)
#endif


#define MESSAGE_MAX_LENGTH	9000

#define QUERY_TIMEOUT		5000	/* 5 seconds */

/* Packet type */
#define MSG_QUERY		20
#define MSG_HIT			10


/*********************\
|* Global variables  *|
\*********************/

extern gboolean active4; // TRUE if IPv4 multicast is on
extern gboolean active6; // TRUE if IPv6 multicast is on

extern guint query_timer_id; // Timer event

extern u_short port_MCast; // IPv4/IPv6 Multicast port

extern int sockUDP4; // IPv4 UDP socket descriptor
extern const char *str_addr_MCast4;// IPv4 multicast address
extern struct sockaddr_in6 addr_MCast4; // struct with destination data of IPv4 socket
extern struct ip_mreq imr_MCast4; // struct with IPv4 multicast data
extern GIOChannel *chanUDP4; // GIO channel descriptor of socket UDPv4
extern guint chanUDP4_id; // Channel number of socket UDPv4

extern int sockUDP6; // IPv6 UDP socket descriptor
extern const char *str_addr_MCast6;// IPv6 multicast address
extern struct sockaddr_in6 addr_MCast6; // struct with destination data of IPv6 socket
extern struct ipv6_mreq imr_MCast6; // struct with IPv6 multicast data
extern GIOChannel *chanUDP6; // GIO channel descriptor of socket UDPv6
extern guint chanUDP6_id; // Channel number of socket UDPv6

extern int sockUDPq;	// IPv6 UDP query socket descriptor (for sending queries)
extern GIOChannel *chanUDPq; // GIO channel descriptor for query socket
extern guint chanUDPq_id;	// Channel number of query socket
extern u_short portUDPq;	// Port number of query socket

extern u_short port_TCP; // TCP port
extern int sockTCP; // IPv6 TCP socket descriptor
extern GIOChannel *chanTCP; // GIO channel descriptor of TCPv6 socket
extern guint chanTCP_id; // Channel number of socket TCPv6


/*****************************************\
|* Functions to write and read messages  *|
\*****************************************/

// Write the QUERY message fields ('seq', 'filename') into buffer 'buf'
//    and returns the length in 'len'
gboolean write_query_message(char *buf, int *len, uint32_t seq, const char* filename);

// Read the QUERY message fields ('seq', 'filename') from buffer 'buf'
//    with length 'len'; returns TRUE if successful, or FALSE otherwise
gboolean read_query_message(char *buf, int len, uint32_t *seq, const char **filename);

// Write the HIT message fields ('seq','filename','flen','fhash','sTCP_port') into buffer 'buf'
//    and returns the length in 'len'
gboolean write_hit_message(char *buf, int *len, uint32_t seq, const char* filename, unsigned long long flen,
							uint32_t fhash, unsigned short sTCP_port, struct in6_addr *srvIP);

// Read the HIT message fields ('seq','filename','flen','fhash','sTCP_port') from buffer 'buf'
//    with length 'len'; returns TRUE if successful, or FALSE otherwise
gboolean read_hit_message(char *buf, int len, uint32_t *seq, const char **filename, unsigned long long *flen,
							uint32_t *fhash, unsigned short *sTCP_port, struct in6_addr *srvIP);


/**********************************************\
|* Socket callback and message send functions *|
\**********************************************/


// Send a packet to an UDP socket
gboolean send_unicast(struct in6_addr *ip, u_short port, const char *buf, int n);

// Send packet to the IPv6/IPv4 Multicast address
gboolean send_multicast(const char *buf, int n, gboolean use_IPv6);

// Callback to receive connections at TCP socket
gboolean callback_connections_TCP(GIOChannel *source, GIOCondition condition,
		gpointer data);

/// Callback to receive data from UDP IPv6 unicast socket
gboolean callback_UDPUnicast_data(GIOChannel *source, GIOCondition condition,
		gpointer data);

// Callback to receive data from UDP IPv6/IPv4 multicast sockets
//   data is equal to (int)6 for IPv6 and to (int)4 for IPv4
gboolean callback_UDPMulticast_data(GIOChannel *source, GIOCondition condition,
		gpointer data);



/******************************************\
|* Functions to initialize/close sockets  *|
\******************************************/

// Close all UDP sockets
void close_sockUDP(void);

// Close TCP socket
void close_sockTCP(void);

// Create IPv4 UDP multicast socket, configure it, and register its callback
gboolean init_socket_udp4(u_short port_multicast, const char *addr_multicast);

// Create IPv6 UDP multicast socket, configure it, and register its callback
gboolean init_socket_udp6(u_short port_multicast, const char *addr_multicast);

// Initialize all sockets. Configures the following global variables:
// port_MCast - multicast port
// str_addr_MCast4/6 - IP multicast address
// imr_MCast4/6 - struct for association to to IP Multicast address
// addr_MCast4/6 - struct with UDP socket data for sending packets to the groups
gboolean init_sockets(u_short port_multicast, const char *addr4_multicast, const char *addr6_multicast);

#endif
