/*****************************************************************************\
 * Redes Integradas de Telecomunicacoes
 * MIEEC/MEEC/MERSIM - FCT NOVA   2024/2025
 *
 * file.h
 *
 * Header file of functions that handle the shared file list GUI and file management
 *
 * @author  Luis Bernardo
\*****************************************************************************/
#ifndef FILE_H_
#define FILE_H_

// Create a directory and sets permissions that allow creation of new files
gboolean make_directory(const char *dirname);

// Extract the directory name from a full path name
const char *get_trunc_filename(const char *FileName);

// Return the file length
unsigned long long get_filesize(const char *FileName);

// Return a XOR HASH value for the contents of a file
uint32_t fhash(FILE *f);

// Return a XOR HASH value for the contents of a file
uint32_t fhash_filename(const char *FileName);


#endif
