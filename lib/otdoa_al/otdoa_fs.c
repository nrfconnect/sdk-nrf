/*
 * Copyright (c) 2025 PHY Wireless, Inc.
 *
 * SPDX-License-Identifier: LicenseRef-PHYW
 */

/*
 * otdoa_fs.c
 *
 * This file is mostly just a wrapper around the Zephyr file system
 * functions (fs_open, etc.).  It adds mutex protection for the file
 * access functions.
 *
 */

#include <stddef.h>
#include <zephyr/fs/fs.h>
#include <zephyr/fs/fs_interface.h>
#include <zephyr/sys/mutex.h>
#include <string.h>
#include <otdoa_al/otdoa_fs.h>

K_MUTEX_DEFINE(otdoa_fs_mutex);

#define FS_LOCK_WAIT (K_MSEC(1000))
/* _frv is the failure return value.  i.e. the value used in the return statement */
#define OFS_LOCK(_frv)                                                                             \
	{                                                                                          \
		if (k_mutex_lock(&otdoa_fs_mutex, FS_LOCK_WAIT) != 0) {                            \
			printk("%s: Failed to get OTDOA FS mutex  owner = %p  lock_count = %u  "   \
			       "RETRYING...\n",                                                    \
			       __func__, otdoa_fs_mutex.owner, otdoa_fs_mutex.lock_count);         \
			if (k_mutex_lock(&otdoa_fs_mutex, FS_LOCK_WAIT) != 0) {                    \
				printk("%s: Failed to get OTDOA FS mutex  owner = %p  lock_count " \
				       "= %u\n",                                                   \
				       __func__, otdoa_fs_mutex.owner, otdoa_fs_mutex.lock_count); \
				return _frv;                                                       \
			}                                                                          \
		}                                                                                  \
	}

#define OFS_UNLOCK() k_mutex_unlock(&otdoa_fs_mutex)

/* convert mode string to zephyr-compatible mode flags */
static int get_mode_flags(const char *psz_mode, fs_mode_t *p_mode_flags)
{
	fs_mode_t mode_flags = 0;

	switch (psz_mode[0]) {
	case 'r':
		mode_flags = FS_O_READ;
		if (psz_mode[1] == '+') {
			mode_flags = FS_O_RDWR;
		}
		break;
	case 'w':
		mode_flags = FS_O_WRITE | FS_O_CREATE;
		if (psz_mode[1] == '+') {
			mode_flags = FS_O_RDWR | FS_O_CREATE;
		}
		break;
	case 'a':
		mode_flags = FS_O_RDWR | FS_O_CREATE | FS_O_APPEND;
		break;
	default:
		return -1;
	}
	*p_mode_flags = mode_flags;
	return 0;
}

#define FS_MAX_FILES	     10
#define FS_FILE_TABLE_OFFSET 10

tOFS_FILE file_table[FS_MAX_FILES] = {0};

/* returns NULL for error, otherwise pointer to allocated file */
tOFS_FILE *file_table_alloc(void)
{
	tOFS_FILE *p_file = NULL;

	OFS_LOCK(NULL); /* exit this func with rv NULL on failure */

	for (int idx = 0; idx < FS_MAX_FILES; idx++) {
		if (file_table[idx].flags == 0) {
			p_file = file_table + idx;
			/* set the flags so the entry is no longer free */
			p_file->flags = FS_O_READ;
			break;
		}
	}
	OFS_UNLOCK();
	return p_file;
}

/* Zero (mark available) a file table entry */
int file_table_release(tOFS_FILE *p_file)
{
	int i_ret = -1;

	OFS_LOCK(-EAGAIN);
	if (p_file - file_table < FS_MAX_FILES) {
		memset(p_file, 0, sizeof(*p_file));
		i_ret = 0;
	}
	OFS_UNLOCK();
	return i_ret;
}

tOFS_FILE *ofs_fopen(const char *path, const char *psz_mode)
{
	int i_ret = 0;
	fs_mode_t mode_flags = 0;

	i_ret = get_mode_flags(psz_mode, &mode_flags);
	if (i_ret != 0) {
		return NULL;
	}

	OFS_LOCK(NULL); /* exit this func with rv NULL on failure */

	tOFS_FILE *p_file = file_table_alloc();

	if (p_file == NULL) {
		OFS_UNLOCK();
		return p_file;
	}

	fs_file_t_init(p_file);

	/* Hack to delete file for "w" modes */
	if (psz_mode[0] == 'w') {
		struct fs_dirent deFile = {0};
		int iFS = fs_stat(path, &deFile);

		if (0 == iFS) {
			printk("ofs_fopen(\"%s\",\"%s\") unlink existing file\n", path, psz_mode);
			fs_unlink(path);
		}
	}

	i_ret = fs_open(p_file, path, mode_flags);

	if (i_ret != 0) {
		file_table_release(p_file);
		p_file = NULL;
	}

	OFS_UNLOCK();

	return p_file;
}

int ofs_fclose(tOFS_FILE *p_file)
{
	OFS_LOCK(-EAGAIN);
	int i_ret = fs_close(p_file);

	file_table_release(p_file);
	OFS_UNLOCK();
	return i_ret;
}

int ofs_open(struct fs_file_t *zfp, const char *file_name, fs_mode_t flags)
{
	OFS_LOCK(-EAGAIN);

	int i_ret = fs_open(zfp, file_name, flags);

	OFS_UNLOCK();
	return i_ret;
}

ssize_t ofs_read(tOFS_FILE *p_file, void *p_src, size_t num_bytes)
{
	OFS_LOCK(-EAGAIN);

	int i_ret = fs_read(p_file, p_src, num_bytes);

	OFS_UNLOCK();
	return i_ret;
}

ssize_t ofs_write(tOFS_FILE *p_file, const void *p_dest, size_t num_bytes)
{
	OFS_LOCK(-EAGAIN);

	int i_ret = fs_write(p_file, p_dest, num_bytes);

	OFS_UNLOCK();
	return i_ret;
}

int ofs_fseek(tOFS_FILE *p_file, long offset, int whence)
{
	OFS_LOCK(-EAGAIN);

	int i_ret = fs_seek(p_file, offset, whence);

	OFS_UNLOCK();
	return i_ret;
}

int ofs_tell(struct fs_file_t *p_file)
{
	OFS_LOCK(-EAGAIN);

	int i_ret = fs_tell(p_file);

	OFS_UNLOCK();
	return i_ret;
}

/* directory functions */
int ofs_opendir(tOFS_DIR *zdp, const char *path)
{
	OFS_LOCK(-EAGAIN);

	int i_ret = fs_opendir(zdp, path);

	OFS_UNLOCK();
	return i_ret;
}
int ofs_closedir(tOFS_DIR *zdp)
{
	OFS_LOCK(-EAGAIN);

	int i_ret = fs_closedir(zdp);

	OFS_UNLOCK();
	return i_ret;
}

int ofs_readdir(tOFS_DIR *zdp, tOFS_DIRENT *entry)
{
	OFS_LOCK(-EAGAIN);

	int i_ret = fs_readdir(zdp, entry);

	OFS_UNLOCK();
	return i_ret;
}

int ofs_readmount(int *index, const char **name)
{
	OFS_LOCK(-EAGAIN);

	int i_ret = fs_readmount(index, name);

	OFS_UNLOCK();
	return i_ret;
}

void ofs_dir_t_init(struct fs_dir_t *zdp)
{
	OFS_LOCK();

	fs_dir_t_init(zdp);

	OFS_UNLOCK();
}

int ofs_sync(struct fs_file_t *zfp)
{
	OFS_LOCK(-EAGAIN);

	int i_ret = fs_sync(zfp);

	OFS_UNLOCK();
	return i_ret;
}

int ofs_statvfs(const char *path, struct fs_statvfs *stat)
{
	OFS_LOCK(-EAGAIN);

	int i_ret = fs_statvfs(path, stat);

	OFS_UNLOCK();
	return i_ret;
}

int ofs_stat(const char *path, struct fs_dirent *entry)
{
	OFS_LOCK(-EAGAIN);

	int i_ret = fs_stat(path, entry);

	OFS_UNLOCK();
	return i_ret;
}

int ofs_unlink(const char *path)
{
	OFS_LOCK(-EAGAIN);

	int i_ret = fs_unlink(path);

	OFS_UNLOCK();
	return i_ret;
}

char *ofs_fgets(char *p_dest, int size, tOFS_FILE *p_file)
{
	memset(p_dest, 0, size);

	OFS_LOCK(NULL); /* exit this func with rv NULL on failure */
	char *p_ret = p_dest;

	while (size > 1) {
		/* always return null-term string */
		int i_ret = fs_read(p_file, p_dest, 1);

		if (i_ret == 1) {
			size--;
			/* check for end of line */
			if (*p_dest++ == '\n') {
				break;
			}
		} else {
			/* Error return */
			p_ret = NULL;
			break;
		}
	}
	OFS_UNLOCK();
	return p_ret;
}
