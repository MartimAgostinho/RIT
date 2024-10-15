/*****************************************************************************\
 * Redes Integradas de Telecomunicacoes
 * MIEEC/MEEC/MERSIM - FCT NOVA   2024/2025
 *
 * callbacks.h
 *
 * Headr file of functions that handle main application logic for UDP communication,
 *    controlling file search
 *
 * @author  Luis Bernardo
\*****************************************************************************/
#ifndef INCL_CALLBACKS_
#define INCL_CALLBACKS_

#include <gtk/gtk.h>
#include <glib.h>
#include "gui.h"


#ifndef FALSE
#define FALSE 0
#endif
#ifndef TRUE
#define TRUE (!FALSE)
#endif


#define QUERY_TIMEOUT		5000	/* 5 seconds */


typedef enum { S_IDLE, S_WAIT_HIT } QueryState;


struct Thread_Data;


/**********************\
|*  Global variables  *|
\**********************/

extern gboolean active; // TRUE if server is active

// Main window
extern WindowElements *main_window;
// Directory pathname to write output files
extern char *out_dir;
// List with active TCP connections/subprocesses
extern GList *tcp_conn;


/*******************************************************\
|* Functions that handle file list management buttons  *|
\*******************************************************/

// Handle button "Add" - add a file to the file list
void on_buttonAdd_clicked (GtkButton *button, gpointer user_data);
// Handle button "Remove" - remove a file from the list
void on_buttonRemove_clicked (GtkButton *button, gpointer user_data);
// Handle button "Open" - read the content of a filelist from a file
void on_buttonOpen_clicked (GtkButton *button, gpointer user_data);
// Handle button "Save" - Write the filelist to a file
void on_buttonSave_clicked (GtkButton *button, gpointer user_data);


/*******************************************************\
|* Functions to control the state of the application   *|
\*******************************************************/

// Handle the reception of a Query packet
void handle_Query(char *buf, int buflen, gboolean from_IPv6, struct in6_addr *ip, u_short port);
// Handle the reception of an Hit packet
void handle_Hit(char *buf, int buflen, struct in6_addr *ip, u_short port);

// Close everything
void close_all(void);

// Button that starts and stops the application
void on_togglebutton1_toggled(GtkToggleButton *togglebutton, gpointer user_data);

// Called when the user clicks "Query file"
void on_buttonQuery_clicked (GtkButton *button, gpointer user_data);

// Callback function that handles the end of the closing of the main window
gboolean on_window1_delete_event (GtkWidget * widget,
		GdkEvent * event, gpointer user_data);

#endif
