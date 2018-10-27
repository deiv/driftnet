/**
 * @file tmpdir.h
 *
 * @brief Temporary directory helpers.
 * @author David Suárez
 * @date Sun, 21 Oct 2018 18:41:11 +0200
 *
 * Copyright (c) 2018 David Suárez.
 * Email: david.sephirot@gmail.com
 *
 */

#ifndef __TMPDIR_H__
#define __TMPDIR_H__

#ifdef HAVE_CONFIG_H
    #include <config.h>
#endif

#include <stddef.h>

/**
 * @brief Max filename length for a temp file
 */
#define TMPNAMELEN 64

/**
 * @brief Type of temporary directory.
 */
typedef enum {
    /** The directory is owned by us, we can delete all without any risk */
    TMPDIR_APP_OWNED = 0,

    /** The directory belongs to the user, we can't simply remove all the content and go */
    TMPDIR_USER_OWNED = 1
} tmpdir_type_t;

/**
 * @brief Get the system tmp dir path.
 *
 * @return tmp dir path
 */
const char* get_sys_tmpdir(void);

/**
 * @brief Generates a random temporal filename.
 *
 * @param extension the new filename extension
 * @return filename
 */
const char* generate_new_tmp_filename(const char* extension);

/**
 * @brief Configure the tmp dir options
 *
 * @param dir tmp dir path
 * @param type who owns the tmpdir: the user or us
 * @param max_files maximum number of files
 * @param preserve_files preserve files on exit
 */
void set_tmpdir(const char *dir, tmpdir_type_t type, int max_files, int preserve_files);

/**
 * @brief Get the configured tmp dir path.
 *
 * @return tmp dir path
 */
const char* get_tmpdir(void);

/**
 * @brief Cleans the tmpdir taking in care if we should remove any files.
 */
void clean_tmpdir(void);

/**
 * @brief Makes a new temporary directory in system tmp dir.
 *
 * @return the new tmpdir path
 */
const char* make_tmpdir(void);

/**
 * @brief Check if we can write on dir.
 *
 * @param tmpdir path directory to check for
 * @return TRUE if we can write, FALSE on any error
 */
int check_dir_is_rw(const char* tmpdir);

/**
 * @brief Check if the configured maximum number of files is reached
 *
 * @return FALSE if not exceed, TRUE if yes
 */
int tmpfiles_limit_reached(void);

/**
 * @brief Writes a file to the temporary directory.
 *
 * @param filename filename of the file to create
 * @param file_data data to write
 * @param data_len size of data
 */
void tmpfile_write_file(const char* filename, const unsigned char *file_data, const size_t data_len);

/**
 * @brief Deletes a file from the temp dir.
 *
 * @param filename filename to delete
 */
void tmpfile_delete_file(char* filename);

#endif /* __TMPDIR_H__ */
