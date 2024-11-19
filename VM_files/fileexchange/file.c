/*****************************************************************************\
 * Redes Integradas de Telecomunicacoes
 * MIEEC/MEEC/MERSIM - FCT NOVA   2024/2025
 *
 * file.c
 *
 * Functions that handle the shared file list GUI and file management
 *
 * @author  Luis Bernardo
\*****************************************************************************/
#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include <gtk/gtk.h>
#include <assert.h>
#include <sys/stat.h>
#include <errno.h>
#include <string.h>
#include <stdlib.h>
#include "gui.h"



// Create a directory and set permissions that allow creation of new files
gboolean make_directory(const char *dirname) {
  return !mkdir(dirname, S_IRUSR|S_IWUSR|S_IXUSR|S_IRGRP|S_IWGRP|S_IXGRP|S_IROTH|S_IWOTH|S_IXOTH)
	 || (errno==EEXIST);
}


// Extract the directory name from a full path name
const char *get_trunc_filename(const char *FileName) {
  char *pt= strrchr(FileName, (int)'/');
  if (pt != NULL)
    return pt+1;
  else
    return FileName;
}


// Return the file length
unsigned long long get_filesize(const char *FileName)
{
  struct stat file;
  if(!stat(FileName, &file))
  {
    return (long long)file.st_size;
  }
  return 0;
}


// Return a XOR HASH value for the contents of a file
uint32_t fhash(FILE *f) {
  assert(f != NULL);
  rewind(f);
  uint32_t sum= 0;
  uint32_t aux= 0;
  while (fread(&aux, 1, sizeof(uint32_t), f) > 0)
      sum ^= aux;
  return sum;
}


// Return a XOR HASH value for the contents of a file
uint32_t fhash_filename(const char *FileName) {
	FILE *f= fopen(FileName, "r");
	if (f == NULL)
		return 0;
	uint32_t result= fhash(f);
	fclose(f);
	return result;
}

