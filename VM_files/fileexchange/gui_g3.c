/*****************************************************************************\
 * Redes Integradas de Telecomunicacoes
 * MIEEC/MEEC/MERSIM - FCT NOVA   2024/2025
 *
 * gui_g3.c
 *
 * Functions that handle graphical user interface interaction
 *
 * @author  Luis Bernardo
\*****************************************************************************/
#include <gtk/gtk.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <assert.h>
#include <time.h>
#include <memory.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <glib.h>
#include <glib/gi18n.h>
#include "gui.h"
#include "file.h"
#include "callbacks.h"

// Set here the glade file name
#define GLADE_FILE "fileexchange.glade"


// Mutex to synchronize changes to GUI log window
pthread_mutex_t lmutex = PTHREAD_MUTEX_INITIALIZER;
// Mutex to synchronize changes to GUI database of file transfer threads
pthread_mutex_t gmutex = PTHREAD_MUTEX_INITIALIZER;
// Mutex to synchronize changes to GUI database of files
pthread_mutex_t fmutex = PTHREAD_MUTEX_INITIALIZER;

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



/** Initialization function */
gboolean
init_app (WindowElements *win)
{
        GtkBuilder              *builder;
        GError                  *err=NULL;

        /* use GtkBuilder to build our interface from the XML file */
        builder = gtk_builder_new ();
        if (gtk_builder_add_from_file (builder, GLADE_FILE, &err) == 0)
        {
                error_message (err->message);
                g_error_free (err);
                return FALSE;
        }

        /* get the widgets which will be referenced in callbacks */
        win->window = GTK_WIDGET (gtk_builder_get_object (builder,
                                                             "window1"));
        win->entryIPv6 = GTK_ENTRY (gtk_builder_get_object (builder,
                                                             "entryIPv6"));
        win->entryIPv4 = GTK_ENTRY (gtk_builder_get_object (builder,
                                                             "entryIPv4"));
        win->entryMPort = GTK_ENTRY (gtk_builder_get_object (builder,
                                                             "entryMPort"));
        win->entryTCPPort = GTK_ENTRY (gtk_builder_get_object (builder,
                                                             "entryTCPPort"));
        win->entryPID = GTK_ENTRY (gtk_builder_get_object (builder,
                                                             "entryPID"));
        win->treeFile = GTK_TREE_VIEW (gtk_builder_get_object (builder,
                                                             "treeview1"));
        win->listFile = GTK_LIST_STORE (gtk_builder_get_object (builder,
                                                             "liststore_files"));
        win->entryFilelistName = GTK_ENTRY (gtk_builder_get_object (builder,
                                                             "entryFilelistName"));
        win->entryFileQuery = GTK_ENTRY (gtk_builder_get_object (builder,
                                                             "entryFileQuery"));
        win->entryOutDir = GTK_ENTRY (gtk_builder_get_object (builder,
                                                             "entryOutDir"));
        win->checkSlow = GTK_TOGGLE_BUTTON(gtk_builder_get_object(builder,
        													 "checkbuttonSlow"));
        win->treeThread = GTK_TREE_VIEW (gtk_builder_get_object (builder,
                                                             "treeview2"));
        win->listThread = GTK_LIST_STORE (gtk_builder_get_object (builder,
                                                             "liststore_transf"));
        win->textView = GTK_TEXT_VIEW (gtk_builder_get_object (builder,
                                                                "textview1"));

        /* connect signals, passing our TutorialTextEditor struct as user data */
        gtk_builder_connect_signals (builder, win);

        /* free memory used by GtkBuilder object */
        g_object_unref (G_OBJECT (builder));

        return TRUE;
}


/** Log the message str to the textview and command line */
void Log (const gchar * str)
{
  GtkTextBuffer *textbuf;
  GtkTextIter tend;

  // Adds text to the command line
  g_print("%s", str);

  pthread_mutex_lock( &lmutex );
  textbuf = GTK_TEXT_BUFFER (gtk_text_view_get_buffer (main_window->textView));
  gtk_text_buffer_get_iter_at_offset (textbuf, &tend, -1);	// Gets reference to the last position
  // Adds text to the textview
  gtk_text_buffer_insert (textbuf, &tend, g_strdup (str), strlen (str));
  pthread_mutex_unlock( &lmutex );
}

/** Get the content of entryIPv6, validating if it is a valid IPv6 address.
 *   addrv6 is returned with the address in numerical format.
 *   Returns the address name, or NULL if it is not valid */
const gchar *get_IPv6Multicast(struct in6_addr *addrv6) {
	struct in_addr addrv4;
	struct in6_addr addrv6_aux, *addrv6_pt;
	const gchar *textIP;

	textIP= gtk_entry_get_text(main_window->entryIPv6);
	if (textIP == NULL)
		return NULL;
	addrv6_pt= (addrv6==NULL) ? &addrv6_aux : addrv6;

	if (inet_pton(AF_INET, textIP, &addrv4)) {
		Log("Invalid IPv6 address: sockets IPv6 cannot use IPv4 multicast addresses\n");
		return NULL;
	} else if (inet_pton(AF_INET6, textIP, addrv6_pt)) {
		if ((textIP[0]=='f' || textIP[0]=='F') && (textIP[1]=='f' || textIP[1]=='F'))
			return textIP;
		else {
			Log("Invalid IPv6 address: it is not multicast (must start with FF01:: - FF1E::)\n");
			return NULL;
		}
	} else {
		Log("Invalid IPv6 address\n");
		return NULL;
	}
}

/** Set the content of entryIPv6 */
void set_IPv6Multicast(const char *addr) {
	assert(addr != NULL);
	gtk_entry_set_text(main_window->entryIPv6, addr);
}

/** Get the content of entryIPv4, validating if it is a valid IPv4 address.
 *   addrv4 is returned with the address in numerical format.
 *   Returns the address name, or NULL if it is not valid */
const gchar *get_IPv4Multicast(struct in_addr *addrv4) {
	struct in_addr addrv4_aux, *addrv4_pt;
	const gchar *textIP;

	textIP= gtk_entry_get_text(main_window->entryIPv4);
	if (textIP == NULL)
		return NULL;
	addrv4_pt= (addrv4==NULL) ? &addrv4_aux : addrv4;

	if (inet_pton(AF_INET, textIP, addrv4_pt)) {
		if (!IN_MULTICAST(ntohl(addrv4_pt->s_addr))) {
			Log("Invalid IPv4 address: not multicast\n");
			return NULL;
		} else
			return textIP;
	} else {
		Log("Invalid IPv4 address\n");
		return NULL;
	}
}

/** Set the content of entryIPv4 */
void set_IPv4Multicast(const char *addr) {
	assert(addr != NULL);
	gtk_entry_set_text(main_window->entryIPv4, addr);
}

// Translate a string into its numerical value
static int get_number_from_text(const gchar *text) {
  int n= 0;
  char *pt;

  if ((text != NULL) && (strlen(text)>0)) {
    n= strtol(text, &pt, 10);
    return ((pt==NULL) || (*pt)) ? -1 : n;
  } else
    return -1;
}

/** Validate a port number */
static short get_PortNumber(GtkEntry *entryText) {
	const char *textPort= gtk_entry_get_text(entryText);
	int port= get_number_from_text(textPort);
	if (port>(powl(2,15)-1))
		port= -1;
	return (short)port;
}

/** Get the Multicast port number. Return the port number, or -1 if error */
short get_PortMulticast() {
	return get_PortNumber(main_window->entryMPort);
}

/** Set the Multicast port number entry */
void set_PortMulticast(unsigned short port) {
	char buf[10];
	sprintf(buf, "%hu", port);
	gtk_entry_set_text(main_window->entryMPort, buf);
}

/** Get the TCP port number. Return the port number, or -1 if error */
short get_PortTCP() {
	return get_PortNumber(main_window->entryTCPPort);
}

/** Set the TCP port number entry */
void set_PortTCP(unsigned short port) {
	char buf[10];
	sprintf(buf, "%hu", port);
	gtk_entry_set_text(main_window->entryTCPPort, buf);
}

/** Get the PID number. Return -1 if error */
int get_PID() {
	return get_number_from_text(gtk_entry_get_text(main_window->entryPID));
}

/** Set the PID number entry */
void set_PID(int pid) {
	char buf[10];
	sprintf(buf, "%d", pid);
	gtk_entry_set_text(main_window->entryPID, buf);
}


/** Get the name of the text file with the file list */
const gchar *get_Filelist_Filename() {
	return gtk_entry_get_text(main_window->entryFilelistName);
}

/** Set the name of the text file with the file list */
void set_Filelist_Filename(const char *filename) {
	gtk_entry_set_text(main_window->entryFilelistName, filename);
}

/** Get the file name in the query box */
const gchar *get_QueryFile() {
	return gtk_entry_get_text(main_window->entryFileQuery);
}

/** Set the file name in the query box */
void set_QueryFile(const char *addr) {
	gtk_entry_set_text(main_window->entryFileQuery, addr);
}

/** Get the output directory box's contents */
const gchar *get_OutDir() {
	return gtk_entry_get_text(main_window->entryOutDir);
}

/** Set the output directory box's contents */
void set_OutDir(const char *dir) {
	gtk_entry_set_text(main_window->entryOutDir, dir);
}


/** Return the value of the CheckButton "Slow" */
gboolean get_slow(void) {
	return gtk_toggle_button_get_active(main_window->checkSlow);
}


/** Block or unblock the configuration GtkEntry boxes in the GUI */
void block_entrys(gboolean block)
{
  gtk_editable_set_editable(GTK_EDITABLE(main_window->entryIPv4), !block);
  gtk_editable_set_editable(GTK_EDITABLE(main_window->entryIPv6), !block);
  gtk_editable_set_editable(GTK_EDITABLE(main_window->entryMPort), !block);
  gtk_editable_set_editable(GTK_EDITABLE(main_window->entryOutDir), !block);
}



/*************************************************\
|*  Functions that manage the filelist TreeView  *|
\*************************************************/

/** Search for a filename in the file treeview list */
gboolean locate_File(const char *filename, GtkTreeIter *iter, gboolean incl_path, gboolean lock_gdk) {
	assert(filename != NULL);
	assert(iter != NULL);
	GtkTreeModel *list_store;
	gboolean valid;
	gboolean ok= FALSE;

	if (lock_gdk) {
		/* get GTK thread lock */
		gdk_threads_enter ();
	}

	list_store= GTK_TREE_MODEL(main_window->listFile);
	// Get the first iter in the list
	valid = gtk_tree_model_get_iter_first (list_store, iter);

	while (valid)
	{
		// Walk through the list, reading each row
		gchar *str_filename;

		// Make sure you terminate calls to gtk_tree_model_get() with a '-1' value
		gtk_tree_model_get (list_store, iter,
				0, &str_filename,
				-1);
		const char *str_fname= incl_path ? str_filename : get_trunc_filename(str_filename);

		// Do something with the data
#ifdef DEBUG
		g_print ("Filename: %s - %s\n", str_fname, str_filename);
#endif
		if (!strcmp(str_fname, filename)) {
			g_free (str_filename);
			ok= TRUE;
			break;
		}
		valid = gtk_tree_model_iter_next (list_store, iter);
		g_free (str_filename);
	}

	if (lock_gdk) {
		/* release GTK thread lock */
		gdk_threads_leave ();
	}
	return ok;
}


/** Return the full pathname associated to 'filename' */
gboolean get_File_fullname(const char *filename, const char **fullname, gboolean lock_gdk) {
	if ((filename == NULL) || (fullname == NULL)) {
		Log("ERROR: Invalid parameters in get_File_fullname()\n");
		return FALSE;
	}

	if (lock_gdk) {
		/* get GTK thread lock */
		gdk_threads_enter ();
	}
	GtkTreeModel *list_store= GTK_TREE_MODEL(main_window->listFile);
	GtkTreeIter iter;
	gboolean ok= FALSE;
	if (locate_File(filename, &iter, FALSE, FALSE)) {
		gtk_tree_model_get (list_store, &iter, 0, fullname, -1);
		ok= TRUE;
	}

	if (lock_gdk) {
		/* release GTK thread lock */
		gdk_threads_leave ();
	}
	return ok;
}


/** Return length and hash value of file 'filename' */
gboolean get_File_details(const char *filename, unsigned long long *flen, unsigned long long *fhash, gboolean lock_gdk) {
	if ((filename == NULL) || (flen == NULL) || (fhash == NULL)) {
		Log("ERROR: Invalid parameters in get_File_details()\n");
		return FALSE;
	}

	if (lock_gdk) {
		/* get GTK thread lock */
		gdk_threads_enter ();
	}

	GtkTreeModel *list_store= GTK_TREE_MODEL(main_window->listFile);
	GtkTreeIter iter;
	unsigned long lenaux, hashaux;
	gboolean ok= FALSE;
	if (locate_File(filename, &iter, FALSE, FALSE)) {
		gtk_tree_model_get (list_store, &iter, 1, &lenaux, 2, &hashaux, -1);
		*flen= (unsigned long long)lenaux;
		*fhash= (unsigned long long)hashaux;
		ok= TRUE;
	}

	if (lock_gdk) {
		/* release GTK thread lock */
		gdk_threads_leave ();
	}
	return ok;
}


/** Add a file to the file table */
gboolean add_File(const char *filename, gboolean lock_gdk) {
	assert(filename != NULL);

	GtkTreeIter iter;

	if (lock_gdk) {
		/* get GTK thread lock */
		gdk_threads_enter ();
	}

	if (locate_File(filename, &iter, TRUE, FALSE)) {
		Log("Replacing file\n");
	} else {
		// new file
		gtk_list_store_append(main_window->listFile, &iter);
	}
	unsigned long alength= (unsigned long)get_filesize(filename);
	unsigned long afilehash= (unsigned long)fhash_filename(filename);
	gtk_list_store_set(main_window->listFile, &iter, 0, filename, 1, alength, 2, afilehash, -1);

	if (lock_gdk) {
		/* release GTK thread lock */
		gdk_threads_leave ();
	}
	return TRUE;
}

/** Delete a file from the file table */
gboolean del_File(const char *filename, gboolean lock_gdk) {
	assert(filename != NULL);
	GtkTreeIter iter;
	gboolean ok= FALSE;

	if (lock_gdk) {
		/* get GTK thread lock */
		gdk_threads_enter ();
	}
	if (locate_File(filename, &iter, TRUE, FALSE)) {
		if (!gtk_list_store_remove(main_window->listFile, &iter)) {
			Log("Failed to remove file from list\n");
			return FALSE;
		}
		ok= TRUE;
	}
	if (lock_gdk) {
		/* release GTK thread lock */
		gdk_threads_leave ();
	}
	return ok;
}


// Add all files in 'filename' file to the list
gboolean add_filelist(const char *filelist_filename, gboolean lock_gdk) {
	char buf[256];	// Maximum filename size

	if ((filelist_filename != NULL) && (strlen(filelist_filename)>0)) {
		FILE *f= fopen(filelist_filename, "r");
		if (f == NULL) {
			sprintf(buf, "Open(r) of filelist file '%s' failed\n", filelist_filename);
			Log(buf);
			return FALSE;
		}
		while(!feof(f)) {
			buf[0]= '\0';
			if (fgets(buf, sizeof(buf), f) != NULL) {
				if (strlen(buf)>1) {
					buf[strlen(buf)-1]= '\0';	// Clear '\n' in Linux
					add_File(buf, lock_gdk);
				}
			}
		}
		return TRUE;
	} else
		return FALSE;
}


// Write the filelist contents to a file
gboolean write_filelist(const char *filelist_filename, gboolean lock_gdk) {
	if ((filelist_filename != NULL) && (strlen(filelist_filename)>0)) {
		char tmp_buf[100];
		FILE *f= fopen(filelist_filename, "w");
		GtkTreeIter iter;
		GtkTreeModel *list_store;
		gboolean valid;

		if (f == NULL) {
			sprintf(tmp_buf, "Could not open file list '%s' for writing\n", filelist_filename);
			Log(tmp_buf);
			return FALSE;
		}

		if (lock_gdk) {
			/* get GTK thread lock */
			gdk_threads_enter ();
		}
		list_store= GTK_TREE_MODEL(main_window->listFile);
		// Get the first iter in the list
		valid = gtk_tree_model_get_iter_first (list_store, &iter);

		while (valid)
		{
			// Walk through the list, reading the filename in each row
			gchar *str_filename;

			// Make sure you terminate calls to gtk_tree_model_get() with a '-1' value
			gtk_tree_model_get (list_store, &iter,
					0, &str_filename,
					-1);
			// Do something with the data
#ifdef DEBUG
			g_print ("Filename: %s\n", str_filename);
#endif
			if (fprintf(f, "%s\n", str_filename)<= 0) {
				sprintf(tmp_buf, "Failed writing to file list '%s'\n", filelist_filename);
				Log(tmp_buf);
				g_free (str_filename);
				fclose(f);
				return FALSE;
			}
			g_free (str_filename);
			valid = gtk_tree_model_iter_next (list_store, &iter);
		}
		fclose(f);
		if (lock_gdk) {
			/* release GTK thread lock */
			gdk_threads_leave ();
		}
		return TRUE;

	} else
		return FALSE;

}


/** Deletes all files from the file table */
void clear_Files(gboolean lock_gdk) {
	if (lock_gdk) {
		/* get GTK thread lock */
		gdk_threads_enter ();
	}
	gtk_list_store_clear(main_window->listFile);
	if (lock_gdk) {
		/* release GTK thread lock */
		gdk_threads_leave ();
	}
}



/***********************************************************\
|* Functions that handle File transfer TreeView management *|
\***********************************************************/

// Search for 'tid' in file transfer list; returns iter
gboolean GUI_locate_thread_by_id(unsigned tid, GtkTreeIter *iter, gboolean lock_glib) {
	assert(iter != NULL);
	gboolean valid;
	gboolean ok= FALSE;

	if (lock_glib) {
		/* get GTK thread lock */
		gdk_threads_enter ();
	}
	GtkTreeModel *list_store= GTK_TREE_MODEL(main_window->listThread);
	// Get the first iter in the list
	valid = gtk_tree_model_get_iter_first (list_store, iter);

	while (valid)
	{
		// Walk through the list, reading each row
		int pid_val;

		// Make sure you terminate calls to gtk_tree_model_get() with a '-1' value
		gtk_tree_model_get (list_store, iter,
				0, &pid_val,
				-1);
		if (tid == pid_val) {
			ok= TRUE;
			break;
		}
		valid = gtk_tree_model_iter_next (list_store, iter);
	}

	if (lock_glib) {
		/* release GTK thread lock */
		gdk_threads_leave ();
	}
	return ok;
}


// Add line with thread 'tid' data to list
gboolean GUI_regist_thread(unsigned tid, gboolean is_snd, const char *f_name, const char *of_name, gboolean lock_glib)
{
	assert(f_name != NULL);
	assert(of_name != NULL);
#ifdef DEBUG
	g_print("Thread %d added to the list\n", tid);
#endif

	GtkTreeIter iter;

	if (lock_glib) {
		/* get GTK thread lock */
		gdk_threads_enter ();
	}
	LOCK_MUTEX(&gmutex, "lock_g1\n");
	if (GUI_locate_thread_by_id(tid, &iter, FALSE)) {
		// PID already in the table
		char tmp_buf[80];
		sprintf(tmp_buf, "Replacing tid %u in table\n", tid);
		Log(tmp_buf);
	} else {
		// new file
		gtk_list_store_append(main_window->listThread, &iter);
	}
	gtk_list_store_set(main_window->listThread, &iter, 0, tid, 1, is_snd ? "SND" : "RCV",
			2, 0, 3, strdup(f_name), 4, strdup(of_name), -1);
	UNLOCK_MUTEX(&gmutex, "unlock_g1\n");
	if (lock_glib) {
		/* release GTK thread lock */
		gdk_threads_leave ();
	}
	return TRUE;
}


// Update the file information in thread tid information - for senders
gboolean GUI_update_filename(unsigned tid, const char *f_name, gboolean lock_glib) {
	assert(f_name != NULL);
#ifdef DEBUG
	g_print("Thread %u updated file \"%s\"\n", tid, f_name);
#endif

	GtkTreeIter iter;
	gboolean ok;
	if (lock_glib) {
		/* get GTK thread lock */
		gdk_threads_enter ();
	}
	LOCK_MUTEX(&gmutex, "lock_g2\n");
	if (!GUI_locate_thread_by_id(tid, &iter, FALSE)) {
		UNLOCK_MUTEX(&gmutex, "unlock_g2\n");
		ok= FALSE;
	} else {
		gtk_list_store_set(main_window->listThread, &iter, 3, strdup(f_name), -1);
		UNLOCK_MUTEX(&gmutex, "unlock_g2\n");
		ok= TRUE;
	}

	if (lock_glib) {
		/* release GTK thread lock */
		gdk_threads_leave ();
	}
	return ok;
}


// Update the percentage information in thread tid information - for both
gboolean GUI_update_bytes_sent(unsigned tid, int trans, gboolean lock_glib) {
	GtkTreeIter iter;
	gboolean ok= FALSE;
#ifdef DEBUG
	g_print("Thread %u updated trans %d\n", tid, trans);
#endif

	if (lock_glib) {
		/* get GTK thread lock */
		gdk_threads_enter ();
	}
	LOCK_MUTEX(&gmutex, "lock_g3\n");
	if (!GUI_locate_thread_by_id(tid, &iter, FALSE)) {
		Log("Internal error: update of a non existing thread\n");
		ok= FALSE;
	} else {
		gtk_list_store_set(main_window->listThread, &iter, 2, trans, -1);
		ok= TRUE;
	}
	UNLOCK_MUTEX(&gmutex, "unlock_g3\n");

	if (lock_glib) {
		/* release GTK thread lock */
		gdk_threads_leave ();
	}
	return ok;
}


// Get all information from the line with 'tid' in the thread list
gboolean GUI_get_thread_info(unsigned tid, const char **sndrcv,
		int *transf, const char **filename, const char *ofilename, gboolean lock_glib) {
	GtkTreeIter iter;
	gboolean ok= FALSE;

	if (lock_glib) {
		/* get GTK thread lock */
		gdk_threads_enter ();
	}
	LOCK_MUTEX(&gmutex, "lock_g4\n");
	if (!GUI_locate_thread_by_id(tid, &iter, FALSE)) {
		UNLOCK_MUTEX(&gmutex, "unlock_g4\n");
		ok= FALSE;
	} else {
		GtkTreeModel *list_store= GTK_TREE_MODEL(main_window->listThread);
		gtk_tree_model_get (list_store, &iter, 1, sndrcv,
				2, transf, 3, filename, 4, ofilename, -1);
		UNLOCK_MUTEX(&gmutex, "unlock_g4\n");
		ok= TRUE;
	}

	if (lock_glib) {
		/* release GTK thread lock */
		gdk_threads_leave ();
	}
	return ok;
}


// Delete the line with 'tid' in threads list
gboolean GUI_del_thread(unsigned tid, gboolean lock_glib) {
	GtkTreeIter iter;
	gboolean ok;
#ifdef DEBUG
	g_print("Thread %u deleted from list\n", tid);
#endif
	if (lock_glib) {
		/* get GTK thread lock */
		gdk_threads_enter ();
	}
	LOCK_MUTEX(&gmutex, "lock_g5\n");
	if (GUI_locate_thread_by_id(tid, &iter, FALSE)) {
		gtk_list_store_remove(main_window->listThread, &iter);
		UNLOCK_MUTEX(&gmutex, "unlock_g5-f\n");
		ok= TRUE;
	} else {
		UNLOCK_MUTEX(&gmutex, "unlock_g5-nf\n");
		ok= FALSE;
	}

	if (lock_glib) {
		/* release GTK thread lock */
		gdk_threads_leave ();
	}
	return ok;
}


// Delete all threads from the threads table
void GUI_clear_threads(gboolean lock_glib) {
	if (lock_glib) {
		/* get GTK thread lock */
		gdk_threads_enter ();
	}
	LOCK_MUTEX(&gmutex, "lock_g6\n");
	gtk_list_store_clear(main_window->listThread);
	UNLOCK_MUTEX(&gmutex, "unlock_g6\n");
	if (lock_glib) {
		/* release GTK thread lock */
		gdk_threads_leave ();
	}
}



/***************************\
|*   Auxiliary functions   *|
\***************************/

/** Create a window with an error message and outputs it to the command line */
void
error_message (const gchar *message)
{
        GtkWidget               *dialog;

        /* log to terminal window */
        g_warning ("%s", message);

        /* create an error message dialog and display modally to the user */
        dialog = gtk_message_dialog_new (NULL,
                                         GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT,
                                         GTK_MESSAGE_ERROR,
                                         GTK_BUTTONS_OK,
                                         "%s", message);

        gtk_window_set_title (GTK_WINDOW (dialog), "Error!");
        gtk_dialog_run (GTK_DIALOG (dialog));
        gtk_widget_destroy (dialog);
}


/** Open a dialog window to choose a filename to read. Return the
    newly allocated filename or NULL. */
const gchar *get_open_filename (WindowElements *win)
{
        GtkWidget               *chooser;
        gchar                   *filename=NULL;

        chooser = gtk_file_chooser_dialog_new ("Open File...",
                                               GTK_WINDOW (win->window),
                                               GTK_FILE_CHOOSER_ACTION_OPEN,
                                               _("Cancel"), GTK_RESPONSE_CANCEL,
                                               _("Open"), GTK_RESPONSE_OK,
                                               NULL);

        if (gtk_dialog_run (GTK_DIALOG (chooser)) == GTK_RESPONSE_OK)
        {
                filename = gtk_file_chooser_get_filename (GTK_FILE_CHOOSER (chooser));
        }

        gtk_widget_destroy (chooser);
        return filename;
}


/** Open a dialog window to choose a filename to save. Return the
    newly allocated filename or NULL. */
const gchar *get_save_filename (WindowElements *win)
{
        GtkWidget               *chooser;
        gchar                   *filename=NULL;

        chooser = gtk_file_chooser_dialog_new ("Save File...",
                                               GTK_WINDOW (win->window),
                                               GTK_FILE_CHOOSER_ACTION_SAVE,
                                               _("Cancel"), GTK_RESPONSE_CANCEL,
                                               _("Save"), GTK_RESPONSE_OK,
                                               NULL);

        if (gtk_dialog_run (GTK_DIALOG (chooser)) == GTK_RESPONSE_OK)
        {
                filename = gtk_file_chooser_get_filename (GTK_FILE_CHOOSER (chooser));
        }

        gtk_widget_destroy (chooser);
        return filename;
}



/***********************\
|*   Event handlers    *|
\***********************/

/** Handle 'Clear' button - clears textMemo */
void on_buttonClear_clicked (GtkButton * button, gpointer user_data)
{
  GtkTextBuffer *textbuf;
  GtkTextIter tbegin, tend;

  pthread_mutex_lock( &lmutex );
  textbuf = GTK_TEXT_BUFFER (gtk_text_view_get_buffer (main_window->textView));
  gtk_text_buffer_get_iter_at_offset (textbuf, &tbegin, 0);
  gtk_text_buffer_get_iter_at_offset (textbuf, &tend, -1);
  gtk_text_buffer_delete (textbuf, &tbegin, &tend);
  pthread_mutex_unlock( &lmutex );
}


