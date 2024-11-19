/*****************************************************************************\
 * Redes Integradas de Telecomunicacoes
 * MIEEC/MEEC/MERSIM - FCT NOVA   2024/2025
 *
 * gui.h
 *
 * Header file of functions that handle graphical user interface interaction
 *
 * @author  Luis Bernardo
\*****************************************************************************/
#ifndef _INCL_GUI_H
#define _INCL_GUI_H

#include <gtk/gtk.h>
#include <netinet/in.h>

/* store the widgets which may need to be accessed in a typedef struct */
typedef struct
{
        GtkWidget               *window;
        GtkEntry				*entryIPv6;
        GtkEntry				*entryIPv4;
        GtkEntry				*entryMPort;
        GtkEntry				*entryTCPPort;
        GtkEntry				*entryPID;
        GtkTreeView				*treeFile;
        GtkListStore			*listFile;
        GtkEntry				*entryFilelistName;
        GtkEntry				*entryFileQuery;
        GtkEntry				*entryOutDir;
        GtkToggleButton			*checkSlow;
        GtkTreeView				*treeThread;
        GtkListStore			*listThread;
        GtkTextView				*textView;
} WindowElements;

// Global pointer to the main window elements
extern WindowElements *main_window;

// Initialization function
gboolean init_app (WindowElements *window);

// Log the message str to the textview and command line
void Log (const gchar * str);



/************************************************\
|*   Functions that read and update text boxes  *|
\************************************************/

/** Get the content of entryIPv6, validating if it is a valid IPv6 address.
 *   addrv6 is returned with the address in numerical format.
 *   Returns the address name, or NULL if it is not valid */
const gchar *get_IPv6Multicast(struct in6_addr *addrv6);
/** Set the content of entryIPv6 */
void set_IPv6Multicast(const char *addr);

/** Get the content of entryIPv4, validating if it is a valid IPv4 address.
 *   addrv4 is returned with the address in numerical format.
 *   Returns the address name, or NULL if it is not valid */
const gchar *get_IPv4Multicast(struct in_addr *addrv4);
/** Sets the content of entryIPv4 */
void set_IPv4Multicast(const char *addr);

/** Get the Multicast port number. Return the port number, or -1 if error */
short get_PortMulticast();
/** Set the Multicast port number entry */
void set_PortMulticast(unsigned short port);

/** Get the TCP port number. Return the port number, or -1 if error */
short get_PortTCP();
/** Set the TCP port number entry */
void set_PortTCP(unsigned short port);

/** Get the PID number. Return -1 if error */
int get_PID();
/** Set the PID number entry */
void set_PID(int pid);

/** Get the name of the text file with the file list */
const gchar *get_Filelist_Filename();
/** Set the name of the text file with the file list */
void set_Filelist_Filename(const char *addr);

/** Get the file name in the query box */
const gchar *get_QueryFile();
/** Set the file name in the query box */
void set_QueryFile(const char *addr);

/** Get the output directory box's contents */
const gchar *get_OutDir();
/** Set the output directory box's contents */
void set_OutDir(const char *addr);

/** Return the value of the CheckButton "Slow" */
gboolean get_slow(void);

// Block or unblock the configuration GtkEntry boxes in the GUI
void block_entrys(gboolean block);


/*************************************************\
|*  Functions that manage the filelist TreeView  *|
\*************************************************/

/** Search for a filename in the file treeview list */
gboolean locate_File(const char *filename, GtkTreeIter *iter, gboolean incl_path, gboolean lock_gdk);

/** Return the full pathname associated to 'filename' */
gboolean get_File_fullname(const char *filename, const char **fullname, gboolean lock_gdk);

/** Return length and hash value of file 'filename' */
gboolean get_File_details(const char *filename, unsigned long long *flen, unsigned long long *fhash, gboolean lock_gdk);

/** Add a file to the file table */
gboolean add_File(const char *filename, gboolean lock_gdk);

/** Delete a file from the file table */
gboolean del_File(const char *filename, gboolean lock_gdk);

// Add all files in 'filename' file to the list
gboolean add_filelist(const char *filelist_filename, gboolean lock_gdk);

// Write the filelist contents to a file
gboolean write_filelist(const char *filelist_filename, gboolean lock_gdk);

/** Delete all files from the file table */
void clear_Files(gboolean lock_gdk);


/***********************************************************\
|* Functions that handle File transfer TreeView management *|
\***********************************************************/

// Search for 'tid' in file transfer list; returns iter
gboolean GUI_locate_thread_by_id(unsigned tid, GtkTreeIter *iter, gboolean lock_gdk);
// Add line with thread 'tid' data to list
gboolean GUI_regist_thread(unsigned tid, gboolean is_snd, const char *f_name, const char *of_name, gboolean lock_gdk);
// Update the file information in thread tid information - for senders
gboolean GUI_update_filename(unsigned tid, const char *f_name, gboolean lock_gdk);
// Update the percentage information in thread tid information - for both
gboolean GUI_update_bytes_sent(unsigned tid, int trans, gboolean lock_gdk);
// Get all information from the line with 'tid' in the thread list
gboolean GUI_get_thread_info(unsigned tid, const char **sndrcv,
		int *transf, const char **filename, const char *ofilename, gboolean lock_gdk);
// Delete the line with 'tid' in thread list
gboolean GUI_del_thread(unsigned tid, gboolean lock_gdk);
// Delete all threads from the threads table
void GUI_clear_threads(gboolean lock_gdk);


/***************************\
|*   Auxiliary functions   *|
\***************************/

/** Create a window with an error message and outputs it to the command line */
void error_message (const gchar *message);
/** Open a dialog window to choose a filename to read. Return the
    newly allocated filename or NULL. */
const gchar *get_open_filename (WindowElements *win);
/** Open a dialog window to choose a filename to save. Return the
    newly allocated filename or NULL. */
const gchar *get_save_filename (WindowElements *win);


/***********************\
|*   Event handlers    *|
\***********************/
// Event handlers in gui_g3.c

// Handle 'Clear' button - clears textMemo
void on_buttonClear_clicked (GtkButton *button, gpointer user_data);


// External event handlers in file.c and callbacks.c
void on_togglebutton1_toggled (GtkToggleButton *togglebutton, gpointer user_data);

void on_buttonAdd_clicked (GtkButton *button, gpointer user_data);
void on_buttonRemove_clicked (GtkButton *button, gpointer user_data);
void on_buttonOpen_clicked (GtkButton *button, gpointer user_data);
void on_buttonSave_clicked (GtkButton *button, gpointer user_data);

void on_buttonQuery_clicked (GtkButton *button, gpointer user_data);
void on_buttonStop_clicked (GtkButton *button, gpointer user_data);

#endif



