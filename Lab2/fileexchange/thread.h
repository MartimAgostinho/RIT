/*****************************************************************************\
 * Redes Integradas de Telecomunicacoes
 * MIEEC/MEEC/MERSIM - FCT NOVA   2024/2025
 *
 * thread.h
 *
 * Header file of module that handles TCP threads and communication
 *
 * @author  Luis Bernardo
\*****************************************************************************/
#ifndef THREAD_INC_
#define THREAD_INC_

#include <gtk/gtk.h>
#include <netinet/in.h>


// File thread (TCP connection) information
typedef struct Thread_Data {
    gboolean sending;	// TRUE: transmitting ; FALSE: receiving
    char fname[256];   	// if (!sending) has the filename being recorded
    int len;		   	// if (!sending) has the block size being received

	char ofilename[512];// if (!sending) has the output file full pathname
	unsigned long long flen; // File length
	uint32_t fhash;		// File hash value received in HIT packet

    pthread_t tid;	   	// Thread ID
    char name_str[80]; 	// Thread name
    int s;			   	// Descriptor of the TCP socket
    FILE *f;		   	// In/out file descriptor
    long long total; 	// Bytes handled in the subprocess
    struct in6_addr ip; // IP address of remote node
    u_short port;		// port number of remote node
	gboolean slow;		// Using slow configuration

    gboolean finished;	// If it finished the transference
    struct Thread_Data *self;	// Self testing pointer, to detected freed memory blocks
} Thread_Data ;


// Directory pathname to write output files
extern char *out_dir;
// List with active TCP connections/subprocesses
extern GList *tcp_conn;



/***********************************************************\
|*  Functions to handle the list of file transfer threads  *|
\***********************************************************/
// Add thread information to process list
Thread_Data *new_thread_desc(gboolean sending, struct in6_addr *ip, u_short port,
		const char *filename, const char *ofilename, unsigned long long h_flen, uint32_t fhash,
		gboolean slow);
// Locate descriptor in subprocess list
Thread_Data *locate_file_thread_in_list(unsigned tid);
// Delete descriptor in subprocess list
gboolean stop_thread_desc(unsigned tid, Thread_Data *pt, gboolean lock_glib);
// Validate if a thread_data pointer is valid
gboolean valid_thread_desc(Thread_Data *pt);

/************************************************************\
|* Functions that implement file transmission subprocesses  *|
\************************************************************/

// Send a signal to a subprocess
void send_kill(int pid, int sig, gboolean may_fail);
// Stop the transmission of all files
void stop_all_threads_GUI(gboolean lock_glib);
// Callback button 'Stop': stops the selected TCP subprocess transmission
void on_buttonStop_clicked(GtkButton *button, gpointer user_data);
// Starts a thread for file reception
Thread_Data *start_file_download_thread (struct in6_addr *ip_file, u_short port,
		const char *filename, const char *ofilename, unsigned long long f_len, uint32_t fhash,
		gboolean slow);
// Starts a thread for sending a file
Thread_Data *start_snd_file_thread (int msgsock, struct in6_addr *ip, u_short port, gboolean slow);

#endif
