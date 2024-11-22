/*****************************************************************************\
 * Redes Integradas de Telecomunicacoes
 * MIEEC/MEEC/MERSIM - FCT NOVA   2024/2025
 *
 * callbacks.c
 *
 * Functions that handle main application logic for UDP communication,
 *    controlling file search
 *
 * @author  Luis Bernardo
\*****************************************************************************/
#include <gtk/gtk.h>
#include <arpa/inet.h>
#include <assert.h>
#include <time.h>
#include <memory.h>
#include <glib.h>
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


/**********************\
|*  Global variables  *|
\**********************/

gboolean active = FALSE; 	// TRUE if server is active

gboolean waitingForHIT = FALSE;

uint32_t lasthit;

guint t_id;

// Directory pathname to write output files
char *out_dir= NULL;

// Query control variables
int			qid;			// Sequence number
char		qname[81]; 		// Name looked up

// Auxiliary variable set to TRUE when the filelist is modified
gboolean filelist_modified= FALSE;



/*********************\
|*  Local variables  *|
\*********************/

static int counter = 0; // Used to define unique numbers for incoming files
static char tmp_buf[8000];



/*******************************************************\
|* Functions that handle file list management buttons  *|
\*******************************************************/

// Handles button "Add" - adds a file to the file list
void on_buttonAdd_clicked (GtkButton *button, gpointer user_data) {
	const gchar *filename= get_open_filename (main_window);
	if (filename != NULL) {
		if (!add_File(filename, FALSE)) {
			sprintf(tmp_buf, "File %s not added to file list\n", filename);
			Log("File not added");
		}
	}
	filelist_modified= TRUE;
}


// Handles button "Remove" - removes a file from the list
void on_buttonRemove_clicked (GtkButton *button, gpointer user_data)
{
	GtkTreeSelection *selection;
	GtkTreeModel	*model;
	GtkTreeIter	iter;
	gchar *str_filename;

	selection= gtk_tree_view_get_selection(main_window->treeFile);
	if (gtk_tree_selection_get_selected(selection, &model, &iter)) {
		gtk_tree_model_get (model, &iter, 0, &str_filename, -1);
#ifdef DEBUG
		g_print ("File %s will be removed\n", str_filename);
#endif
		g_free (str_filename);
	} else {
		Log ("No file selected\n");
		return;
	}

	if (!gtk_list_store_remove(main_window->listFile, &iter)) {
		Log("Failed to remove file from list\n");
	}
	filelist_modified= TRUE;
}


// Handle button "Open" - read the content of a filelist from a file
void on_buttonOpen_clicked (GtkButton *button, gpointer user_data)
{
	add_filelist(get_Filelist_Filename(), FALSE);
	filelist_modified= TRUE;
	Log("File list opened\n");
}


// Handle button "Save" - Write the filelist to a file
void on_buttonSave_clicked (GtkButton *button, gpointer user_data)
{
	write_filelist(get_Filelist_Filename(), FALSE);
	filelist_modified= FALSE;
	Log("File list saved\n");
}


/*******************************************************\
|* Functions to control the state of the application   *|
\*******************************************************/


// Handle the reception of a Query packet
void handle_Query(char *buf, int buflen, gboolean from_IPv6, struct in6_addr *ip, u_short port) {
	uint32_t seq;
	const char *fname;

	if (!read_query_message(buf, buflen, &seq, &fname)) {
		Log("Invalid Query packet\n");
		return;
	}

	assert ((fname != NULL) && (ip != NULL));
	if (strcmp(fname, get_trunc_filename(fname))) {
		Log("ERROR: The Query must not include the pathname - use 'get_trunc_filename'\n");
		return;
	}
	sprintf(tmp_buf, "Received Query '%s' from [%s]:%hu\n", fname, addr_ipv6(ip), port);
	Log(tmp_buf);

	unsigned long long flen;
	unsigned long long fhash;
	if (!get_File_details(fname, &flen, &fhash, TRUE)) {
		g_print("File not found\n");
		free((void *)fname);
		return;
	}

	// Prepare Hit packet
	int hlen;
	static char hbuf[MESSAGE_MAX_LENGTH];  // sending buffer
	struct in6_addr srvIP;

	// Get the server IP address
	if (from_IPv6)
		memcpy(&srvIP, &local_ipv6, sizeof(struct in6_addr));
	else
		translate_ipv4_to_ipv6(addr_ipv4(&local_ipv4), &srvIP);

	if (!write_hit_message(hbuf, &hlen, seq, fname, flen, fhash, port_TCP, &srvIP)) {
		Log("ERROR: writing Hit message\n");
		free((void *)fname);
		return;
	}
	// Send packet
	send_unicast(ip, port, hbuf, hlen);
	free((void *)fname);
}


// Handle the reception of an Hit packet
void handle_Hit(char *buf, int buflen, struct in6_addr *ip, u_short port) {
	static int counter= 0;
	uint32_t seq;
	const char *fname;
	char ofname[256];
	unsigned long long flen;
	uint32_t fhash;
	unsigned short sTCPport;
	struct in6_addr srvIP;


	if (!read_hit_message(buf, buflen, &seq, &fname, &flen, &fhash, &sTCPport, &srvIP)) {
		Log("Invalid Hit packet\n");
		return ;
	}

	sprintf(tmp_buf, "Received Hit '%s' (IP= %s; port= %hu; Len=%llu; Hash=%u)\n", fname,
			addr_ipv6(&srvIP), sTCPport, flen, fhash);

	//Log("Please complete the function callbacks.handle_Hit (TASK 2) to handle the Hit message\n");
	waitingForHIT = FALSE;

	if(seq == lasthit) {
			Log("Already received hit for this file.");
				return;
		}
	g_source_remove(t_id);	// Cancel timer

	Log(tmp_buf);

	lasthit = seq;

	// TASK 2
	//  Put here the code to handle the HIT message
	// 	Don't forget to cancel the timer.
	//  You should only use the first HIT received and ignore the other ones for each QUERY.
	//  You must only accept HITs with the same ID and File of the QUERY
	//  ...


	// Set the filename where the received data will be created
	sprintf(ofname, "%s/file%d.out", out_dir, counter++);
	// Start new download
	start_file_download_thread(ip, sTCPport, fname, ofname, flen, fhash, get_slow());
}


// Close everything
void close_all(void) {
	stop_all_threads_GUI(FALSE);
	close_sockUDP();
	close_sockTCP();
	GUI_clear_threads(FALSE);
}


// Button that starts and stops the application
void on_togglebutton1_toggled(GtkToggleButton *togglebutton, gpointer user_data) {

	if (gtk_toggle_button_get_active(togglebutton)) {

		// *** Start the server ***
		const gchar *addr4_str, *addr6_str;

		// Get local IP
		set_local_IP();
		int n = get_PortMulticast();
		if (n < 0) {
			Log("Invalid multicast port number\n");
			gtk_toggle_button_set_active(togglebutton, FALSE); // Turns button off
			return;
		}
		port_MCast = (unsigned short) n;

		addr6_str = get_IPv6Multicast(NULL);
		addr4_str = get_IPv4Multicast(NULL);
		if (!addr6_str && !addr4_str) {
			gtk_toggle_button_set_active(togglebutton, FALSE); // Turns button off
			return;
		}
		if (!init_sockets(port_MCast, addr4_str, addr6_str)) {
			Log("Failed configuration of server\n");
			gtk_toggle_button_set_active(togglebutton, FALSE); // Turns button off
			return;
		}
		set_PortTCP(port_TCP);
		set_PID(getpid());
		//
		block_entrys(TRUE);
		active = TRUE;
		Log("fileexchange active\n");

	} else {

		// *** Stop the server ***
		close_all();
		block_entrys(FALSE);
		set_PID(0);
		active = FALSE;
		Log("fileexchange stopped\n");
	}

}
gboolean callback_QUERY_timer (gpointer data)
{
	waitingForHIT = FALSE;
	Log("QUERY TIMED OUT!");
	return FALSE; // cancels the timer
}

// Called when the user clicks "Query file"
void on_buttonQuery_clicked (GtkButton *button, gpointer user_data)
{

	if (!active) {
		Log("fileexchange is not active\n");
		return;
	}

	const gchar *name;
	if (waitingForHIT) {
		Log("Still waiting for timeout!");
		return;
	}
	// Read parameters
	name= get_QueryFile();
	if ((name == NULL) || (strlen(name) == 0)) {
		Log("Empty file name is query\n");
		return;
	}
	if (strcmp(name, get_trunc_filename(name))) {
		Log("ERROR: the query file name must not include the pathname\n");
		return;
	}

	// Prepare query message
	qid= ++counter;							// Query ID
	strncpy(qname, name, sizeof(qname)-1);
	qname[sizeof(qname)-1]= '\0';			// Query name
	int qlen;
	if (!write_query_message(tmp_buf, &qlen, qid, qname)) {
		Log("ERROR: failed to prepare Query message\n");
		return;
	}

	// Send query message
	gboolean sent= FALSE;
	if (active4) {
		// Send QUERY to the IPv4 group
		if (!send_multicast(tmp_buf, qlen, FALSE)) {
			Log("Error sending IPv4 multicast\n");
		} else
			sent= TRUE;
	}
	if (active6) {
		// Send QUERY to the IPv6 group
		if (!send_multicast(tmp_buf, qlen, TRUE)) {
			Log("Error sending IPv6 multicast\n");
		} else
			sent= TRUE;
	}
	if (!sent) {
		Log("fileexchange failed to send multicast packet - terminating\n");
		close_all();
		return;
	}

	// Log("Please complete the function callbacks.on_buttonQuery_clicked (TASK 1) to handle the 'Query file' button\n");


	// TASK 1 - complete this function
	//
	//  After sending the Query, fileexchange must wait for an HIT for up to during 5 seconds and
	//			prevent sending more QUERYs
	//	You must:
	//		1) Add a new variables to know that you are waiting for an HIT message
	//		2) add a new function (callback_QUERY_timer) to handle the timeout event
	//		2) start a timer (you need a new variable), to count the QUERY_TIMEOUT maximum waiting time
	//		3) modify the source code above to avoid having more QUERYs sent while waiting for an HIT or TIMEOUT
	//
	// Suggestion:
	//	Read the tutorial (Introduction to the development of applications in Gnome 3):
	//	- in section 2.2.2.6, an explanation on how to use timers;
	waitingForHIT = TRUE;
	t_id = g_timeout_add(QUERY_TIMEOUT, callback_QUERY_timer,NULL);
}


// Callback function that handles the end of the closing of the main window
gboolean on_window1_delete_event (GtkWidget * widget,
		GdkEvent * event, gpointer user_data)
{
	if (filelist_modified)
		write_filelist(get_Filelist_Filename(), FALSE);
	stop_all_threads_GUI(FALSE);
	gtk_main_quit ();		// Close Gtk main cycle
	return FALSE;			// Must always return FALSE; otherwise the window is not closed.
}


