#*****************************************************************************#
#* Redes Integradas de Telecomunicacoes
#* MIEEC/MEEC/MERSIM - FCT NOVA   2024/2025
#*
#* Makefile
#*
#* @author  Luis Bernardo
#*****************************************************************************#

GNOME_INCLUDES= `pkg-config --cflags --libs gtk+-3.0`
CFLAGS= -Wall -Wno-deprecated-declarations -Wno-unused-value -g -DDEBUG
# CFLAGS= -Wall -Wno-deprecated-declarations -Wno-unused-value -g
# CFLAGS= -Wall -Wno-deprecated-declarations -Wno-unused-value -O3

APP_NAME= fileexchange
APP_MODULES= sock.o gui_g3.o callbacks.o callbacks_socket.o file.o thread.o

all: $(APP_NAME)
	
clean: 
	rm -f $(APP_NAME) *.o


$(APP_NAME): main.c $(APP_MODULES) gui.h sock.h callbacks.h file.h thread.h
	gcc $(CFLAGS) -o $(APP_NAME) main.c $(APP_MODULES) $(GNOME_INCLUDES) -lm -export-dynamic

sock.o: sock.c sock.h gui.h
	gcc $(CFLAGS) -c $(GNOME_INCLUDES) sock.c -export-dynamic

gui_g3.o: gui_g3.c gui.h
	gcc $(CFLAGS) -c $(GNOME_INCLUDES) gui_g3.c -export-dynamic
	
callbacks.o: callbacks.c callbacks.h sock.h
	gcc $(CFLAGS) -c $(GNOME_INCLUDES) callbacks.c -export-dynamic

callbacks_socket.o: callbacks_socket.c callbacks_socket.h callbacks.h sock.h
	gcc $(CFLAGS) -c $(GNOME_INCLUDES) callbacks_socket.c -export-dynamic

file.o: file.c file.h
	gcc $(CFLAGS) -c $(GNOME_INCLUDES) file.c -export-dynamic

thread.o: thread.c thread.h sock.h
	gcc $(CFLAGS) -c $(GNOME_INCLUDES) thread.c -export-dynamic

