/*****************************************************************************\
 * Redes Integradas de Telecomunicacoes
 * MIEEC/MEEC/MERSIM - FCT NOVA   2024/2025
 *
 * callbacks_socket.c
 *
 * Functions that handle UDP/TCP socket management and UDP communication
 *
 * @author  Luis Bernardo
\*****************************************************************************/
#include <gtk/gtk.h>
#include <arpa/inet.h>
#include <assert.h>
#include <time.h>
#include <memory.h>
#include <stdio.h>
#include <stdlib.h>
#include "file.h"
#include "sock.h"
#include "gui.h"
#include "callbacks.h"
#include "callbacks_socket.h"

#include "thread.h"

#ifdef DEBUG
#define debugstr(x)     g_print(x)
#else
#define debugstr(x)
#endif



gboolean active4 = FALSE; // TRUE if IPv4 multicast is on
gboolean active6 = FALSE; // TRUE if IPv6 multicast is on

guint query_timer_id = 0; // Timer event

u_short port_MCast = 0; // IPv4/IPv6 Multicast port

int sockUDP4 = -1; // IPv4 UDP socket descriptor
const char *str_addr_MCast4 = NULL;// IPv4 multicast address
struct sockaddr_in6 addr_MCast4; // struct with destination data of IPv4 socket
struct ip_mreq imr_MCast4; // struct with IPv4 multicast data
GIOChannel *chanUDP4 = NULL; // GIO channel descriptor of socket UDPv4
guint chanUDP4_id = 0; // Channel number of socket UDPv4

int sockUDP6 = -1; // IPv6 UDP socket descriptor
const char *str_addr_MCast6 = NULL;// IPv6 multicast address
struct sockaddr_in6 addr_MCast6; // struct with destination data of IPv6 socket
struct ipv6_mreq imr_MCast6; // struct with IPv6 multicast data
GIOChannel *chanUDP6 = NULL; // GIO channel descriptor of socket UDPv6
guint chanUDP6_id = 0; // Channel number of socket UDPv6

int sockUDPq= -1;	// IPv6 UDP query socket descriptor (for sending queries)
GIOChannel *chanUDPq= NULL; // GIO channel descriptor for query socket
guint chanUDPq_id= 0;	// Channel number of query socket
u_short portUDPq= 0;	// Port number of query socket

u_short port_TCP = 0; // TCP port
int sockTCP = -1; // IPv6 TCP socket descriptor
GIOChannel *chanTCP = NULL; // GIO channel descriptor of TCPv6 socket
guint chanTCP_id = 0; // Channel number of socket TCPv6

/* Local variables */
static char tmp_buf[8000];



/*****************************************\
|* Functions to write and read messages  *|
\*****************************************/

// Write the QUERY message fields ('seq', 'filename') into buffer 'buf'
//    and returns the length in 'len'
gboolean write_query_message(char *buf, int *len, uint32_t seq, const char* filename) {
	if ((buf == NULL) || (len == NULL) || (filename == NULL))
		return FALSE;

	short int fnlen= strlen(filename)+1;
	unsigned char cod= MSG_QUERY;
	char *pt= buf;
	WRITE_BUF(pt, &cod, 1);
	WRITE_BUF(pt, &seq, 4);
	WRITE_BUF(pt, &fnlen, 2);
	WRITE_BUF(pt, filename, fnlen);
	*len= pt-buf;
	return TRUE;
}


// Read the QUERY message fields ('seq', 'filename') from buffer 'buf'
//    with length 'len'; returns TRUE if successful, or FALSE otherwise
gboolean read_query_message(char *buf, int len, uint32_t *seq, const char **filename) {
	if ((buf == NULL) || (seq == NULL) || (filename == NULL) || (len <= 9))
		return FALSE;

	unsigned char cod;
	char *pt= buf;
	short int fnlen;

	READ_BUF(pt, &cod, 1);
	if (cod != MSG_QUERY)
		return FALSE;
	READ_BUF(pt, seq, 4);
	READ_BUF(pt, &fnlen, 2);
	if ((fnlen != len-7) || (strlen(pt) != fnlen-1))
		return FALSE;
	*filename= strdup(pt);
	return TRUE;
}


// Write the HIT message fields ('seq','filename','flen','fhash','sTCP_port') into buffer 'buf'
//    and returns the length in 'len'
gboolean write_hit_message(char *buf, int *len, uint32_t seq, const char* filename, unsigned long long flen,
							uint32_t fhash, unsigned short sTCP_port, struct in6_addr *srvIP) {
	char *pt= buf;
	short int fnlen;

	if ((buf == NULL) || (len == NULL) || (filename == NULL) || (sTCP_port == 0))
		return FALSE;

	fnlen= strlen(filename)+1;
	unsigned char cod= MSG_HIT;
	WRITE_BUF(pt, &cod, 1);
	WRITE_BUF(pt, &seq, 4);
	WRITE_BUF(pt, &fnlen, 2);
	WRITE_BUF(pt, filename, fnlen);
	WRITE_BUF(pt, &flen, sizeof(unsigned long long));
	WRITE_BUF(pt, &fhash, 4);
	WRITE_BUF(pt, &sTCP_port, sizeof(unsigned short));
	WRITE_BUF(pt, srvIP, sizeof(struct in6_addr));
	*len= pt-buf;
	return TRUE;
}


// Read the HIT message fields ('seq','filename','flen','fhash','sTCP_port') from buffer 'buf'
//    with length 'len'; returns TRUE if successful, or FALSE otherwise
gboolean read_hit_message(char *buf, int len, uint32_t *seq, const char **filename, unsigned long long *flen,
							uint32_t *fhash, unsigned short *sTCP_port, struct in6_addr *srvIP) {
	if ((buf == NULL) || (seq == NULL) || (filename == NULL) || (flen == NULL) ||
			(fhash == NULL) || (sTCP_port == NULL) || (srvIP == NULL))
		return FALSE;

	unsigned char cod;
	char *pt= buf;
	short int fnlen;

	READ_BUF(pt, &cod, 1);
	if (cod != MSG_HIT)
		return FALSE;
	READ_BUF(pt, seq, sizeof(uint32_t));
	READ_BUF(pt, &fnlen, sizeof(fnlen));
	if ((fnlen != len-37) || (strlen(pt) != fnlen-1))
		return FALSE;
	*filename= strdup(pt);
	pt += fnlen;
	READ_BUF(pt, flen, sizeof(unsigned long long));
	READ_BUF(pt, fhash, sizeof(uint32_t));
	READ_BUF(pt, sTCP_port, sizeof(unsigned short));
	READ_BUF(pt, srvIP, sizeof(struct in6_addr));
	return TRUE;
}


/**********************************************\
|* Socket callback and message send functions *|
\**********************************************/


// Send a packet to an UDP socket
gboolean send_unicast(struct in6_addr *ip, u_short port, const char *buf, int n)
{
	struct sockaddr_in6 addr;

	assert(sockUDPq>0);
	assert((ip!=NULL) && (buf!=NULL) && (n>0));
	addr.sin6_family= AF_INET6;
	addr.sin6_flowinfo= 0;
	addr.sin6_port= htons(port);
	memcpy(&addr.sin6_addr, ip, sizeof(struct in6_addr));
	/* Send message. */
	if (sendto(sockUDPq, buf, n, 0, (struct sockaddr *)&addr,
			sizeof(addr)) < 0) {
		perror("send_unicast: Error sending datagram");
		return FALSE;
	}
	return TRUE;
}


// Send packet to the IPv6/IPv4 Multicast address
gboolean send_multicast(const char *buf, int n, gboolean use_IPv6)
{
	assert(sockUDPq>0);
	if (use_IPv6) {
		assert(str_addr_MCast6 != NULL);
	} else {
		assert(str_addr_MCast4 != NULL);
	}
	assert((buf!=NULL) && (n>0));
	/* Send message. */
	const gchar *error_msg= use_IPv6 ?"Error sending datagram to IPv6 group"
			: "Error sending datagram to IPv4 group";
	if (sendto(sockUDPq, buf, n, 0, (struct sockaddr *)(use_IPv6 ? &addr_MCast6 : &addr_MCast4),
			sizeof(addr_MCast6)) < 0) {
		perror(error_msg);
		return FALSE;
	}
	return TRUE;
}


// Callback to receive connections at TCP socket
gboolean callback_connections_TCP(GIOChannel *source, GIOCondition condition,
		gpointer data) {

	assert(active);
	if (condition == G_IO_IN) {
		// Received a new connection
		struct sockaddr_in6 server;
		int msgsock;
		socklen_t length = sizeof(server);

		msgsock = accept(sockTCP, (struct sockaddr *) &server, &length);
		if (msgsock == -1) {
			perror("accept");
			Log("accept failed - aborting\nPlease turn off the application!\n");
			return FALSE; // Turns callback off
		} else {
			sprintf(tmp_buf, "Received connection from %s - %d\n", addr_ipv6(
					&server.sin6_addr), ntohs(server.sin6_port));
			Log(tmp_buf);

			// Starts a thread to read the data from the socket
			start_snd_file_thread(msgsock, &server.sin6_addr, ntohs(server.sin6_port),get_slow());

			return TRUE;
		}

	} else if ((condition == G_IO_NVAL) || (condition == G_IO_ERR)) {
		Log("Detected error in TCP socket\n");
		// Closes the sockets
		close_all();
		// Quits the application
		gtk_main_quit();
		return FALSE; // Stops callback for receiving connections in the socket
	} else {
		assert(0); // Should never reach this line
		return FALSE; // Stops callback for receiving connections in the socket
	}
}


// Callback to receive data from UDP IPv6 unicast socket
gboolean callback_UDPUnicast_data(GIOChannel *source, GIOCondition condition,
		gpointer data) {
	static char buf[MESSAGE_MAX_LENGTH]; // buffer for reading data
	struct in6_addr ipv6;
	char ip_str[81];
	u_short port;
	int n;

	if (!active) {
		debugstr("callback_UDPUnicast_data with active=FALSE\n");
		return FALSE;
	}
	if (condition == G_IO_IN) {
		// Receive packet //
		n = read_data_ipv6(sockUDPq, buf, MESSAGE_MAX_LENGTH, &ipv6, &port);
		strncpy(ip_str, addr_ipv6(&ipv6), sizeof(ip_str)-1);
		ip_str[sizeof(ip_str)-1]= '\0';
		if (n <= 0) {
			Log("Failed reading packet from unicast socket\n");
			return TRUE; // Continue waiting for more events
		} else {
			time_t tbuf;
			unsigned char m;
			char *pt;

			// Read data //
			pt = buf;
			READ_BUF(pt, &m, 1); // Reads type and advances pointer
			// Writes date and sender's data //
			time(&tbuf);
			g_print("%sReceived %d bytes (unicast) from %s#%hu - type %hhd\n",
					ctime(&tbuf), n, ip_str, port, m);
			switch (m) {
			case MSG_HIT:
				handle_Hit(buf, n, &ipv6, port);
				break;

			default:
				sprintf(tmp_buf, "Invalid packet type (%d) in unicast socket - ignored\n",
						(int) m);
				Log(tmp_buf);
			}
			return TRUE; // Keeps receiving more packets
		}
	} else if ((condition == G_IO_NVAL) || (condition == G_IO_ERR)) {
		Log("Error detected in UDP query socket\n");
		// Turns sockets off
		close_all();
		// Closes the application
		gtk_main_quit();
		return FALSE; // Stops callback for receiving packets from socket
	} else {
		assert(0); // Should never reach this line
		return FALSE; // Stops callback for receiving packets from socket
	}
}


// Callback to receive data from UDP IPv6/IPv4 multicast sockets
//   data points to an integer equal to 6 for IPv6 and to 4 for IPv4
gboolean callback_UDPMulticast_data(GIOChannel *source, GIOCondition condition,
		gpointer data) {
	static char buf[MESSAGE_MAX_LENGTH]; // buffer for reading data
	struct in6_addr ipv6;
	struct in_addr ipv4;
	char ip_str[81];
	u_short port;
	int n;
	gboolean from_v6= ((*(int *)data) == 6); // data was set to 4 and to 6!

	if (!active) {
		debugstr("callback_UDPMulticast_data with active=FALSE\n");
		return FALSE;
	}
	if (condition == G_IO_IN) {
		// Receive packet //
		if (from_v6 && active6) {
			n = read_data_ipv6(sockUDP6, buf, MESSAGE_MAX_LENGTH, &ipv6, &port);
			strncpy(ip_str, addr_ipv6(&ipv6), sizeof(ip_str)-1);
			ip_str[sizeof(ip_str)-1]= '\0';
		} else if (!from_v6 && active4) {
			n = read_data_ipv4(sockUDP4, buf, MESSAGE_MAX_LENGTH, &ipv4, &port);
			strncpy(ip_str, addr_ipv4(&ipv4), sizeof(ip_str)-1);
			ip_str[sizeof(ip_str)-1]= '\0';
			if (!translate_ipv4_to_ipv6(ip_str, &ipv6)) {
				debugstr("Error in callback_UDPMulticast_data: converting ipv4 to ipv6\n");
				return FALSE;
			}
		} else {
			debugstr("Error in callback_UDPMulticast_data: no read");
			return FALSE;
			assert(active6 || active4);
		}
		if (n <= 0) {
			Log("Failed reading packet from multicast socket\n");
			return TRUE; // Continue waiting for more events
		} else {
			time_t tbuf;
			unsigned char m;
			char *pt;

			// Read data //
			pt = buf;
			READ_BUF(pt, &m, 1); // Reads type and advances pointer
			// Write date and sender's data //
			time(&tbuf);
			g_print("%sReceived %d bytes (multicast) from %s#%hu - type %hhd\n",
					ctime(&tbuf), n, ip_str, port, m);
			switch (m) {
			case MSG_QUERY:
				handle_Query(buf, n, from_v6, &ipv6, port);
				break;

			default:
				sprintf(tmp_buf, "Invalid packet type (%d) in multicast socket - ignored\n",
						(int) m);
				Log(tmp_buf);
			}
			return TRUE; // Keeps receiving more packets
		}
	} else if ((condition == G_IO_NVAL) || (condition == G_IO_ERR)) {
		Log("Error detected in UDP socket\n");
		// Turn sockets off
		close_all();
		// Close the application
		gtk_main_quit();
		return FALSE; // Stop callback for receiving packets from socket
	} else {
		assert(0); // Should never reach this line
		return FALSE; // Stop callback for receiving packets from socket
	}
}



/******************************************\
|* Functions to initialize/close sockets  *|
\******************************************/


// Close all UDP sockets
void close_sockUDP(void) {
	debugstr("close_sockUDP\n");

	// IPv4
	if (chanUDP4 != NULL) {
		remove_socket_from_mainloop(sockUDP4, chanUDP4_id, chanUDP4);
		chanUDP4 = NULL;
		// Close socket
	} else {
		if (sockUDP4 > 0) {
			if (str_addr_MCast4 != NULL) {
				// Leave the multicast group
				if (setsockopt(sockUDP4, IPPROTO_IP, IP_DROP_MEMBERSHIP,
						(char *) &imr_MCast4, sizeof(imr_MCast4)) == -1) {
					perror("Failed de-association to IPv4 multicast group");
					sprintf(tmp_buf,
							"Failed de-association to IPv4 multicast group (%hu)\n",
							sockUDP4);
					Log(tmp_buf);
				}
			}
			if (close(sockUDP4))
				perror("Error during close of IPv4 multicast socket");
		}
	}
	sockUDP4 = -1;
	str_addr_MCast4 = NULL;
	active4 = FALSE;

	// IPv6
	if (chanUDP6 != NULL) {
		remove_socket_from_mainloop(sockUDP6, chanUDP6_id, chanUDP6);
		chanUDP6 = NULL;
		// Close socket
	} else {
		if (sockUDP6 > 0) {
			if (str_addr_MCast6 != NULL) {
				// Leave the group
				if (setsockopt(sockUDP6, IPPROTO_IPV6, IPV6_LEAVE_GROUP,
						(char *) &imr_MCast6, sizeof(imr_MCast6)) == -1) {
					perror("Failed de-association to IPv6 multicast group");
					sprintf(tmp_buf,
							"Failed de-association to IPv6 multicast group (%hu)\n",
							sockUDP6);
					Log(tmp_buf);
					/* NOTE: Kernel 2.4 has a bug - it does not support de-association of IPv6 groups! */
				}
			}
			if (close(sockUDP6))
				perror("Error during close of IPv6 multicast socket");
		}
	}
	sockUDP6 = -1;
	str_addr_MCast6 = NULL;
	active6 = FALSE;

	// Query socket
	if (chanUDPq != NULL) {
		remove_socket_from_mainloop(sockUDPq, chanUDPq_id, chanUDPq);
		chanUDPq = NULL;
		// Close socket
	} else {
		if (sockUDPq > 0) {
			if (close(sockUDPq))
				perror("Error during close of IPv6 query socket");
		}
	}
	sockUDPq = -1;
	portUDPq = 0;
}


// Close TCP socket
void close_sockTCP(void) {
	if (chanTCP != NULL) {
		remove_socket_from_mainloop(sockTCP, chanTCP_id, chanTCP);
		chanTCP = NULL;
	}
	if (sockTCP > 0) {
		close(sockTCP);
		sockTCP = -1;
	}
	port_TCP = 0;
}


// Create IPv4 UDP multicast socket, configure it, and register its callback
gboolean init_socket_udp4(u_short port_multicast, const char *addr_multicast) {
	// TASK 3
	//		Complete this function to initialize the multicast udp socket "sockUDP4", used to receive
	//			the QUERY messages
	// Inputs:
	//		addr_multicast - string with the IPv4 address in string format
	//		port_MCast	- TCP multicast port
	// Outputs:
	//		struct sockaddr_in6 addr_MCast4	- with the IPv4 multicast data (used while sending data to the group)
	//		struct ip_mreq imr_MCast4		- with the IPv4 multicast data (used to associate with the group)
	//		gboolean active4				- TRUE if IPv4 is active, FALSE otherwise
	//
	//	Suggestion:
	//	Read the tutorial (Introduction to the development of applications in Gnome 3):
	//	- in sections 2.1.1, 2.1.3, 2.1.4.1, 2.1.4.3-2.1.4.5, 3.6.2 for a description of the socket API
	//	- in sections 2.2.2.5 for a description of the interaction with the GUI
	//  - in section 3.1 with an example with some similarities
	//
	//  You can also read the function the initializes the IPv6 socket ...
	// It is also recommend to look at sock.h - there are a few functions that might help ...

	if (active4 || !addr_multicast)
		return TRUE;

	// Prepare the multicast association data structure
	if (!get_IPv4(addr_multicast, &imr_MCast4.imr_multiaddr)) {
		return FALSE;
	}
	imr_MCast4.imr_interface.s_addr = htonl(INADDR_ANY);

	// IPv6 struct used to send data to the IPv4 group from an IPv6 socket
	addr_MCast4.sin6_port = htons(port_multicast);
	addr_MCast4.sin6_family = AF_INET6;
	addr_MCast4.sin6_flowinfo = 0;
	if (!translate_ipv4_to_ipv6(addr_multicast,&addr_MCast4.sin6_addr))
		return FALSE;

	Log("callbacks.c - init_socket_udp4 not implemented yet (Task 3)\n");

	// Create the IPV4 UDP socket -don't forget to share the port
	// ...


	// Join the group
	// ...
	str_addr_MCast4 = addr_multicast; // Memorizes it is associated to a group

	// Configure the socket to receive an echo of the multicast packets sent by this application
	// ...

	// Limit the propagation of the multicast message to the local network
	// ...

	// Regist the socket in the main loop of Gtk+
	// ...
	//      Use the callback function: callback_UDPMulticast_data

	//active4 = TRUE;
	//return TRUE;
	return FALSE;  // REMOVE THIS LINE when the task 3 is done!
}


// Create IPv6 UDP multicast socket, configure it, and register its callback
gboolean init_socket_udp6(u_short port_multicast, const char *addr_multicast) {
	char loop = 1;
	static const int id_ipv6= 6;

	if (active6 || !addr_multicast)
		return TRUE;

	// Prepare the data structures
	if (!get_IPv6(addr_multicast, &addr_MCast6.sin6_addr)) {
		return FALSE;
	}
	addr_MCast6.sin6_port = htons(port_multicast);
	addr_MCast6.sin6_family = AF_INET6;
	addr_MCast6.sin6_flowinfo = 0;
	bcopy(&addr_MCast6.sin6_addr, &imr_MCast6.ipv6mr_multiaddr,
			sizeof(addr_MCast6.sin6_addr));
	imr_MCast6.ipv6mr_interface = 0;

	// Create the IPV4 UDP socket
	sockUDP6 = init_socket_ipv6(SOCK_DGRAM, port_multicast, TRUE);
	fprintf(stderr, "UDP6 = %d\n", sockUDP6);
	if (sockUDP6 < 0) {
		Log("Failed opening IPv6 UDP socket\n");
		return FALSE;
	}
	// Join the multicast group
	if (setsockopt(sockUDP6, IPPROTO_IPV6, IPV6_JOIN_GROUP,
			(char *) &imr_MCast6, sizeof(imr_MCast6)) == -1) {
		perror("Failed association to IPv6 multicast group");
		Log("Failed association to IPv6 multicast group\n");
		return FALSE;
	}
	str_addr_MCast6 = addr_multicast; // Memorize it is associated to a group

	// Configure the socket to receive an echo of the multicast packets sent by this application
	setsockopt(sockUDP6, IPPROTO_IPV6, IPV6_MULTICAST_LOOP, &loop, sizeof(loop));

	// Regist the socket in the main loop of Gtk+
	if (!put_socket_in_mainloop(sockUDP6, (void *)&id_ipv6, &chanUDP6_id, &chanUDP6, G_IO_IN,
			callback_UDPMulticast_data)) {
		Log("Failed registration of UDPv6 socket at Gnome\n");
		close_sockUDP();
		return FALSE;
	}
	active6 = TRUE;
	return TRUE;
}


// Initialize all sockets. Configures the following global variables:
// port_MCast - multicast port
// str_addr_MCast4/6 - IP multicast address
// imr_MCast4/6 - struct for association to to IP Multicast address
// addr_MCast4/6 - struct with UDP socket data for sending packets to the groups
gboolean init_sockets(u_short port_multicast, const char *addr4_multicast, const char *addr6_multicast) {

	gboolean ok= init_socket_udp4(port_multicast, addr4_multicast);
	ok |= init_socket_udp6(port_multicast, addr6_multicast);
	if (!ok)
		return FALSE;

	// Query socket	//////////////////////////////////////////////////////////
	sockUDPq = init_socket_ipv6(SOCK_DGRAM, 0, FALSE);
	if (sockUDPq < 0) {
		Log("Failed opening IPv6 UDP query socket\n");
		close_sockUDP();
		return FALSE;
	}
	portUDPq= get_portnumber(sockUDPq);

	// Regist the socket in the main loop of Gtk+
	if (!put_socket_in_mainloop(sockUDPq, NULL, &chanUDPq_id, &chanUDPq, G_IO_IN,
			callback_UDPUnicast_data)) {
		Log("Failed registration of query UDPv6 socket at Gnome\n");
		close_sockUDP();
		return FALSE;
	}


	// Socket TCP   //////////////////////////////////////////////////////////
	if (sockTCP > 0) {
		debugstr("WARNING: 'init_sockets' closed TCP socket\n");
		close_sockTCP();
	}
	// Create TCP socket
	sockTCP = init_socket_ipv6(SOCK_STREAM, 0, FALSE);
	if (sockTCP < 0) {
		Log("Failed opening IPv6 TCP socket\n");
		close_sockUDP();
		close_sockTCP();
		return FALSE;
	}
	port_TCP = get_portnumber(sockTCP);
	if (port_TCP == 0) {
		Log("Failed to get the TCP port number\n");
		close_sockUDP();
		close_sockTCP();
		return FALSE;
	}
	// Prepare the socket to receive connections
	if (listen(sockTCP, 1) < 0) {
		perror("Listen failed\n");
		Log("Listen failed\n");
		close_sockUDP();
		close_sockTCP();
		return FALSE;
	}

	// Regist the TCP socket in Gtk+ main loop
	if (!put_socket_in_mainloop(sockTCP, main_window, &chanTCP_id, &chanTCP, G_IO_IN,
			callback_connections_TCP)) {
		Log("Failed registration of TCPv6 socket at Gnome\n");
		close_sockUDP();
		close_sockTCP();
		return FALSE;
	}
	return TRUE;
}





