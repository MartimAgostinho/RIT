/*****************************************************************************\
 * Redes Integradas de Telecomunicacoes
 * MIEEC/MEEC/MERSIM - FCT NOVA   2024/2025
 *
 * main.c
 *
 * Main function
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
#include "gui.h"
#include "sock.h"
#include "callbacks.h"
#include "file.h"

/* Public variables */
WindowElements *main_window;	// Pointer to all elements of main window


int main (int argc, char *argv[]) {
    /* allocate the memory needed by our TutorialTextEditor struct */
    main_window = g_slice_new (WindowElements);

    /* init glib threads */
    gdk_threads_init ();

    /* initialize GTK+ libraries */
    gtk_init (&argc, &argv);

    if (init_app (main_window) == FALSE) return 1; /* error loading UI */
	gtk_widget_show (main_window->window);

	// Get local IP
/*	set_local_IP();
	if (valid_local_ipv4)
		set_LocalIPv4(addr_ipv4(&local_ipv4));
	if (valid_local_ipv6)
		set_LocalIPv6(addr_ipv6(&local_ipv6));*/

	// Define the output directory, where the files will be written
    char *homedir= getenv("HOME");
    if (homedir == NULL)
      homedir= getenv("PWD");
    if (homedir == NULL)
      homedir= "/tmp";
    out_dir= g_strdup_printf("%s/out%d", homedir, getpid());
    if (!make_directory(out_dir)) {
      Log("Failed creation of output directory: '");
      Log(out_dir);
      Log("'.\n");
      out_dir= "/tmp";
    }
    Log("Received files will be created at: '");
    Log(out_dir);
    Log("'\n");
    set_OutDir(out_dir);

	add_filelist(get_Filelist_Filename(), TRUE);	// Read filelist from configuration file

	// Make the process ignore SIGPIPE signal to have read and write return -1 on errors
	signal(SIGPIPE, SIG_IGN);

	/* get GTK thread lock */
    gdk_threads_enter ();
	// Infinite loop handled by GTK+3.0
	gtk_main();
	/* release GTK thread lock */
	gdk_threads_leave ();

	/* free memory we allocated for TutorialTextEditor struct */
    g_slice_free (WindowElements, main_window);

	return 0;
}

