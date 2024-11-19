/*****************************************************************************\
 * Redes Integradas de Telecomunicacoes
 * MIEEC/MEEC/MERSIM - FCT NOVA   2024/2025
 *
 * thread.c
 *
 * Module that handles TCP threads and communication
 *
 * @author  Luis Bernardo
\*****************************************************************************/
#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include <gtk/gtk.h>
#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <math.h>
#include <sys/time.h>
#include <assert.h>
#include <netinet/in.h>
#include <signal.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <string.h>
#include <errno.h>



#include "thread.h"
#include "callbacks.h"
#include "callbacks_socket.h"
#include "sock.h"
#include "gui.h"
#include "file.h"

#ifdef DEBUG
#define debugstr(x)     g_print("%s", x)
#else
#define debugstr(x)
#endif


#define SLOW_SLEEPTIME	500000		// Sleep time between reads and writes in slow sending
#define RCV_BUFLEN 		65536		// Buffer size used to receive data
#define SND_BUFLEN 		65536		// Buffer size used to send data
#define READ_TIMEOUT	60			// Read timeout - 60 seconds

// List with active TCP connections/threads
GList *tcp_conn = NULL;

// Mutex to synchronize changes to threads list
pthread_mutex_t tmutex = PTHREAD_MUTEX_INITIALIZER;

#ifdef DEBUG
#define LOCK_MUTEX(mutex,str) { \
			fprintf(stderr,str); \
			pthread_mutex_lock( mutex ); \
		}
#else
#define LOCK_MUTEX(mutex,str) { \
			pthread_mutex_lock( mutex ); \
		}
#endif

#ifdef DEBUG
#define UNLOCK_MUTEX(mutex,str) { \
			fprintf(stderr,str); \
			pthread_mutex_unlock( mutex ); \
		}
#else
#define UNLOCK_MUTEX(mutex,str) { \
			pthread_mutex_unlock( mutex ); \
		}
#endif



/***********************************************************\
|* Functions to handle the list of file transfer threads   *|
\***********************************************************/

// Add thread information to thread list
Thread_Data *new_thread_desc(gboolean sending, struct in6_addr *ip, u_short port,
		const char *filename, const char *ofilename, unsigned long long flen, uint32_t fhash,
		gboolean slow) {

	assert(ip != NULL);
	assert(filename != NULL);

	Thread_Data *pt;

	//  g_print("new_desc(pid=%d ; pipe=%d ; %s)\n", pid, pipe, sending?"SND":"RCV");
	pt = (Thread_Data *) malloc(sizeof(Thread_Data));
	pt->sending= sending;
	pt->slow= slow;
	memcpy(&pt->ip, ip, sizeof(struct in6_addr));
	pt->port= port;
	strncpy(pt->fname, filename, sizeof(pt->fname)-1);
	strncpy(pt->ofilename, ofilename, sizeof(pt->ofilename)-1);
	pt->flen= flen;
	pt->fhash= fhash;

	// Default initialization
	pt->len= 0;
	pt->s= 0;
	pt->f= NULL;
	pt->total= 0;
	pt->name_str[0]='\0';
    pt->finished= FALSE;
    pt->self= pt;

    // Add thread descriptor to list
	LOCK_MUTEX(&tmutex, "lock_t1\n");
    tcp_conn = g_list_append(tcp_conn, pt);
	UNLOCK_MUTEX(&tmutex, "unlock_t1\n");
	return pt;
}

// Locate descriptor in thread list
Thread_Data *locate_thead_desc(unsigned tid) {
	GList *list = tcp_conn;
	while (list != NULL) {
		if (((unsigned)((Thread_Data *) list->data)->tid) == tid)
			return (Thread_Data *) list->data;
		list = list->next;
	}
	return NULL;
}


// Remove descriptor from file thread list, stop it and free memory
gboolean stop_thread_desc(unsigned tid, Thread_Data *pt, gboolean lock_glib) {
	LOCK_MUTEX(&tmutex, "lock_t2\n");
	if (pt == NULL)
		pt = locate_thead_desc(tid);
	if (pt != NULL) {
		if (pt->self != pt) {
			UNLOCK_MUTEX(&tmutex, "unlock_t2\n");
			return FALSE;
		}
		if (tcp_conn != NULL) {
			tcp_conn = g_list_remove(tcp_conn, pt);
		}
		pt->finished= TRUE;  // Mark the thread as ending
		// Remove the thread from the GUI
		GUI_del_thread((unsigned)pt->tid, lock_glib);
		// Mark block as freed
		pt->self= NULL;
		UNLOCK_MUTEX(&tmutex, "unlock_t2\n");

		// Close the socket
		if (pt->s>0) {
			close (pt->s);
			pt->s= 0;
		}
		// Close the file
		if (pt->f != NULL) {
			fclose(pt->f);
			pt->f= NULL;
		}

		// Free memory
		free(pt);
		return TRUE;
	} else {
		UNLOCK_MUTEX(&tmutex, "unlock_t2\n");
		return FALSE;
	}
}

// Validate if a thread_data pointer is valid
gboolean valid_thread_desc(Thread_Data *pt) {
	return pt!=NULL && (pt->self==pt);
}



/*******************************************************\
|* Functions that implement file transmission threads  *|
\*******************************************************/


// Auxiliary macro that stops a thread and frees the descriptor
#define STOP_THREAD(pt) { if (valid_thread_desc(pt)) \
							stop_thread_desc((unsigned)pt->tid, pt, TRUE); \
						  return NULL; \
						}

// Auxiliary macro that tests if a thread has been stopped and frees the descriptor
#define TEST_INTERRUPTED(pt) { if (!active || (pt->self!=pt) || pt->finished) {  \
									if (pt->self==pt) \
										g_print("%s interrupted\n", pt->name_str); \
									STOP_THREAD(pt); \
								} \
							 }



/**************************************************\
|* Functions that implement the receiving thread  *|
\**************************************************/

// Starts thread for sending a file
void *file_download_thread (void *ptr)
{
	assert(ptr!=NULL);
	Thread_Data *pt= (Thread_Data *)ptr;

	// Starts a thread that receives data from the TCP socket
	char buf[RCV_BUFLEN+1];
	struct sockaddr_in6 server;
	short int slen;
	struct timeval timeout;	  // To set a timeout for reading from the TCP socket
	struct timeval tv1, tv2;
	struct timezone tz;
	long diff= 0, last_diff= 0;
	int n;

	//*********************************************************************************
	//*      THREAD                                                                   *
	//*********************************************************************************
	sprintf(pt->name_str, "RCV(%u)> ", (unsigned)pt->tid);
	fprintf(stderr, "%sstarted download thread (file= '%s' from [%s]:%hu ; tid = %u)\n",
			pt->name_str, pt->fname, addr_ipv6(&pt->ip), pt->port, (int)pt->tid);
	buf[SND_BUFLEN]= '\0';

	// TASK 5:
	Log("Please complete function thread.file_download_thread (TASK 5)\n");
	STOP_THREAD(pt);  // REMOVE THIS LINE when you start working on TASK 5!

	// Create a new temporary socket TCP IPv6 to receive the file
	// pt->s = ....

	// Connect the socket to (pt->ip : pt->port)
	// if (connect(pt->s, ..., ...) < 0) {
	// 	  perror("RCV>error connecting the TCP socket to receive the file");
	// 	  fprintf(stderr, "%sconnection failed\n", pt->name_str);
	// 	  STOP_THREAD(pt);
	// }

	// Set timeout for reading
	// ...

	if (!pt->slow) {
		Log("Does not optimize socket communication (TASK 6) ...\n");
		// Optimize socket communication
		// ...
	}


	// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
	// Send the request header

	// Send the filename length
	slen= strlen(pt->fname)+1;
	if (send(pt->s, &slen, sizeof(slen), 0) < 0) {
		g_print("%sfailed sending header - aborting\n",
				pt->name_str);
		STOP_THREAD(pt);
	}
	// Send the filename (pt->fname)
	// ...


	// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
	// Receive the reply header

	TEST_INTERRUPTED(pt);
	unsigned long long len_f;

	// Receive the file length and validate if the length is equal to the one received by UDP
	// ...

	if (gettimeofday(&tv1, &tz))
		Log("Error getting the time to start reception\n");

	// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
	// Receive the file

	// Open file
	if ((pt->f= fopen(pt->ofilename, "w")) != NULL) {

		// Receive the file
		// Program receiving loop reading from pt->s to pt->f
		// See the file copy example in the documentation (section 2.1.9) and adapt to a socket scenario ...
		// do { // Loop forever until end of file
		//	 	n= read(pt->s, buf, RCV_BUFLEN);
		//		// write to file
		//		// update pt->total with the total number of bytes received
		// 		// update the % of number of bytes (percent) received using:
		//			GUI_update_bytes_sent((unsigned)pt->tid, perc, TRUE);
		//		// where perc is an integer between 0 and 100 (percentage of the file received)
		//      ...
		//	 	if (pt->slow)
		//		    usleep(SLOW_SLEEPTIME);
		//  } while (active && (n > 0) && ???? );

		// Close file  pt->f
		// ...

		if (gettimeofday(&tv2, &tz)) {
			Log("Error getting the time to stop reception\n");
			diff= 0;
		} else
			diff= (tv2.tv_sec-tv1.tv_sec)*1000000+(tv2.tv_usec-tv1.tv_usec);
	} else {
		perror("Error creating file for writing");
		fprintf(stderr, "%sfailed to create file '%s' for writing\n", pt->name_str, pt->fname);
		STOP_THREAD(pt);
	}


	if (gettimeofday(&tv1, &tz))
		Log("Error getting the time to start reception\n");

	// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
	// Receive the file

	// Open file
	if ((pt->f= fopen(pt->ofilename, "w")) != NULL) {

		// Receive the file
		// Program receiving loop reading from pt->s to pt->f
		//  Include the following sleep instruction in the loop:
		//   if (pt->slow)
		//   	usleep(SLOW_SLEEPTIME);
		//  Update the % of number of bytes (percent) sent using
		//      GUI_update_bytes_sent((unsigned)pt->tid, percent);

		// See the file copy example in the documentation (section 2.1.9) and adapt to a socket scenario ...
		// do { // Loop forever until end of file
		//	 	n= read(pt->s, buf, SND_BUFLEN);
		//		// write to file
		//		// update pt->total with the total number of bytes received
		// 		// update the % of number of bytes (percent) received using:
		//			GUI_update_bytes_sent((unsigned)pt->tid, perc, TRUE);
		//		// where perc is an integer between 0 and 100 (percentage of the file received)
		//      ...
		//	 	if (pt->slow)
		//		    usleep(SLOW_SLEEPTIME);
		//  } while (active && (n > 0) && ???? );

		// Close file
		// ...

		if (gettimeofday(&tv2, &tz)) {
			Log("Error getting the time to stop reception\n");
			diff= 0;
		} else
			diff= (tv2.tv_sec-tv1.tv_sec)*1000000+(tv2.tv_usec-tv1.tv_usec);

	} else {
		perror("Error creating file for writing");
		fprintf(stderr, "%sfailed to create file '%s' for writing\n", pt->name_str, pt->fname);
		STOP_THREAD(pt);
	}

	sprintf(buf, "%sreceiving thread ended - read %lld of %lld bytes in %ld usec\n",
			pt->name_str, pt->total, pt->flen, diff);
	Log(buf);

	STOP_THREAD(pt);
	//*************************************************************************************
	//*      end of Thread                                                        *
	//*************************************************************************************
}


// Starts a thread for file reception
Thread_Data *start_file_download_thread (struct in6_addr *ip_file, u_short port,
		const char *filename, const char *ofilename, unsigned long long f_len, uint32_t fhash,
		gboolean slow)
{
	assert(ip_file != NULL);
	assert(filename != NULL);

	Thread_Data *pt= new_thread_desc(FALSE, ip_file, port, filename, ofilename, f_len, fhash, slow);

	// Start the thread
	if (pthread_create(&pt->tid, NULL, file_download_thread, (void *)pt)) {
		fprintf(stderr, "main: error starting thread\n");
		free (pt);
		return NULL;
	}

	// Update the FList table
	GUI_regist_thread((unsigned)pt->tid, FALSE, filename, ofilename, TRUE);

	return pt;
}


/************************************************\
|* Functions that implement the sending thread  *|
\************************************************/

// Starts a thread for sending a file
void *snd_file_thread (void *ptr)
{
	assert(ptr!=NULL);
	Thread_Data *pt= (Thread_Data *)ptr;

	// Starts a thread that receives data from the TCP socket
	char buf[SND_BUFLEN+1];
	char nome_f[257];
	int n= 0;
	short int slen;
	struct timeval timeout;	  // To set a timeout for reading from the TCP socket
	struct timeval tv1, tv2;
	struct timezone tz;
	long diff= 0;

	// *************************************************************************************
	// *      THREAD                                                                   *
	// *************************************************************************************
	sprintf(pt->name_str, "SND(%u)> ", (unsigned)pt->tid);
	fprintf(stderr, "%s started sending thread (tid = %u)\n", pt->name_str,
			(unsigned)pt->tid);
	buf[RCV_BUFLEN]= '\0';

	// Set timeout for reading
	timeout.tv_sec= READ_TIMEOUT;	// Segundos
	timeout.tv_usec= 0;	// uSegundos
	if (setsockopt(pt->s, SOL_SOCKET, SO_RCVTIMEO, (char *)&timeout, sizeof(timeout)) <0) {
		perror ("Error defining a timeout for reading");
		// Ignore error
	}
	if (!pt->slow) {
		// TASK 6 - Optimize the TCP communication
		// Don't forget to configure your socket to set buffers or other any configuration
		// that maximizes throughput
		//	e.g. SO_SNDBUF, SO_RECVBUF, timeout, etc.
		// The two best groups will receive bonus points at the end.
	}

	// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
	// Read the request header

	// Read and validate the filename length
	if (!active || pt->finished || read(pt->s, &slen, sizeof(slen)) != sizeof(slen)) {
		g_print("%sdid not receive the file name length - aborting\n", pt->name_str);
		STOP_THREAD(pt);
	}
	if (slen> 257) {
		g_print("%sinvalid file name length - aborting\n", pt->name_str);
		STOP_THREAD(pt);
	}
	TEST_INTERRUPTED(pt);
	// Read and validate the filename string
	if (!active || pt->finished || read(pt->s, nome_f, slen) != slen) {
		g_print("%sdid not receive the file name - aborting\n", pt->name_str);
		STOP_THREAD(pt);
	}
	if (nome_f[slen-1] != '\0') {
		g_print("%sfile name does not have '\\0'- aborting\n", pt->name_str);
		STOP_THREAD(pt);
	}
	TEST_INTERRUPTED(pt);

	GUI_update_filename((unsigned)pt->tid, nome_f, TRUE);
	pt->total= 0;
	pt->flen= 0L;

	// Get the file details
	const char *fullname= NULL;
	if (!get_File_fullname(nome_f, &fullname, TRUE)) {
		if (fullname != NULL)
			free((void *)fullname);
		g_print("%sfile %s not found. Ending connection\n", pt->name_str, nome_f);

		// Sends the file length of 0 to the receiver
		if (send(pt->s, &pt->flen, sizeof(pt->flen), 0) < 0) {
			g_print("%sfailed sending header - aborting\n",
					pt->name_str);
		}
		STOP_THREAD(pt);
	}

	g_print("%ssending file %s\n", pt->name_str, nome_f);

		// Open file
	if ((pt->f= fopen(fullname, "r")) != NULL) {
		// TASK 4 - Complete the sending of the headers and the file contents
		Log("thread.c - snd_file_thread is incomplete (Task 4)\n");

		// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
		pt->flen= get_filesize(fullname);
		// Send the reply header with the file length
		// ...


		// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
		// Send the file

		if (gettimeofday(&tv1, &tz))
			Log("Error getting the time to start sending\n");

		// Send the file contents from pt->f to pt->s
		// See the file copy example in the documentation (section 2.1.9) and adapt to a socket scenario ...
		// do { // Loop forever until end of file
		//	 n= fread(buf, 1, SND_BUFLEN, pt->f);
		//		write to socket
		//		update pt->total with the total number of bytes received
		// 		update the % of file received using:
		//			GUI_update_bytes_sent((unsigned)pt->tid, perc, TRUE);
		//		where perc is an integer between 0 and 100 (percentage of the file received)
		//   ...
		//	 if (pt->slow)
		//		usleep(SLOW_SLEEPTIME);
		//  } while (active && (n > 0) && ???? );

		// Close file
		if ((pt->self==pt) && (pt->f!=NULL)) {
			fclose(pt->f);
			pt->f= NULL;
		}


	} else {

		perror("Error opening file - sending length 0");
		// Sends the file length
		if (send(pt->s, &pt->flen, sizeof(pt->flen), 0) < 0) {
			g_print("%sfailed sending header - aborting\n",
					pt->name_str);
		}
		STOP_THREAD(pt);
	}

	TEST_INTERRUPTED(pt);
	if (gettimeofday(&tv2, &tz)) {
		Log("Error getting the time to stop sending\n");
		diff= 0;
	} else
		diff= (tv2.tv_sec-tv1.tv_sec)*1000000+(tv2.tv_usec-tv1.tv_usec);
	TEST_INTERRUPTED(pt);
	sprintf(buf, "%ssending thread ended - sent %lld of %lld bytes in %ld usec\n",
			pt->name_str, pt->total, pt->flen, diff);
	Log(buf);

	STOP_THREAD(pt);
	//*********************************************************************************
	//*      end of thread                                                            *
	//*********************************************************************************
}


// Starts a thread for sending a file
Thread_Data *start_snd_file_thread (int msgsock, struct in6_addr *ip, u_short port, gboolean slow)
{
	if (!active)
		return NULL;

	Thread_Data *pt= new_thread_desc(TRUE, ip, port, "", "", 0L, 0, slow);

	// Store the socket information
	pt->s= msgsock;
	// Start the thread
	if (pthread_create(&pt->tid, NULL, snd_file_thread, (void *)pt)) {
		fprintf(stderr, "main: error starting thread\n");
		free (pt);
		return NULL;
	}
	// Adds to the FList table
	char tmp_buf[100];
	sprintf(tmp_buf, "to [%s] : %hu", addr_ipv6(ip), port);
	GUI_regist_thread((unsigned)pt->tid, TRUE, "?", tmp_buf, TRUE);

	return pt;
}


// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%



// Stop the transmission of all files
void stop_all_threads_GUI(gboolean lock_glib) {
	GtkTreeIter	iter;
	GtkTreeModel *list_store;
	gboolean valid;

	if (lock_glib) {
		/* get GTK thread lock */
		gdk_threads_enter ();
	}
	list_store= GTK_TREE_MODEL(main_window->listThread);
	// Get the first iter in the list
	valid = gtk_tree_model_get_iter_first (list_store, &iter);

	while (valid)
	{
		// Walk through the list, reading each row
		unsigned tid;

		// Make sure you terminate calls to gtk_tree_model_get() with a '-1' value
		gtk_tree_model_get (list_store, &iter, 0, &tid, -1);

		// Stop thread
		stop_thread_desc(tid, NULL, FALSE);

		valid = gtk_tree_model_iter_next (list_store, &iter);
	}
	if (lock_glib) {
		/* release GTK thread lock */
		gdk_threads_leave ();
	}
}


// Callback button 'Stop': stops the selected TCP subprocess transmission
void on_buttonStop_clicked(GtkButton *button, gpointer user_data) {
	GtkTreeSelection *selection;
	GtkTreeModel	*model;
	GtkTreeIter	iter;
	unsigned tid;

	selection= gtk_tree_view_get_selection(main_window->treeThread);
	if (gtk_tree_selection_get_selected(selection, &model, &iter)) {
		gtk_tree_model_get (model, &iter, 0, &tid, -1);
#ifdef DEBUG
		g_print ("Thread %u will be stopped\n", tid);
#endif
	} else {
		Log ("No thread selected\n");
		/* release GTK thread lock */
		gdk_threads_leave ();
		return;
	}
	if (tid <= 0) {
		Log("Invalid TID in the selected line\n");
		/* release GTK thread lock */
		gdk_threads_leave ();
		return;
	}

	if (!gtk_list_store_remove(main_window->listThread, &iter)) {
		Log("Failed to stop thread from list\n");
	}
	// Stop thread
	stop_thread_desc(tid, NULL, FALSE);
}

