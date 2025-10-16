/*
 * Copyright (c) 2025 PHY Wireless, Inc.
 *
 * SPDX-License-Identifier: LicenseRef-PHYW
 */

#ifndef _OTDOA_FS_H_
#define _OTDOA_FS_H_

/* NORDIC uses Zephyr RTOS FS abstraction layer */
#ifndef HOST

#include "zephyr/fs/fs.h"
typedef struct fs_file_t tOFS_FILE;
typedef struct fs_dir_t tOFS_DIR;
typedef struct fs_dirent tOFS_DIRENT;

/*
 * the zephyr fcntl.h defines macros with the same names.
 * we never use them, so for now undefined them to overwrite
 */
#undef FOPEN
#undef FCLOSE
#undef FREAD
#undef FWRITE
#undef FSEEK
#undef FGETS
#undef FUNLINK
#undef FTELL
#undef FSTAT
#undef FSYNC

#define FOPEN(_p, _f) ofs_fopen(_p, _f)
#define FCLOSE(_p)    ofs_fclose(_p)

#define FREAD(_b, _s, _n, _f)  ofs_read(_f, _b, (_s * _n))
#define FWRITE(_b, _s, _n, _f) ofs_write(_f, _b, (_s * _n))
#define FSEEK(_f, _o, _w)      ofs_fseek(_f, _o, _w)
#define FGETS(_b, _s, _f)      ofs_fgets(_b, _s, _f)
#define FTELL(_b)	           ofs_tell(_b)
#define FSTAT(_p, _e)	       ofs_stat(_p, _e)
#define FSYNC(_b)	           ofs_sync(_b)
#define FUNLINK(_b)	           ofs_unlink(_b)

tOFS_FILE *ofs_fopen(const char *path, const char *psz_mode);
int ofs_fclose(tOFS_FILE *p_file);
int ofs_open(struct fs_file_t *zfp, const char *file_name, fs_mode_t flags);
ssize_t ofs_read(tOFS_FILE *p_file, void *p_src, size_t num_bytes);
ssize_t ofs_write(tOFS_FILE *p_file, const void *p_dest, size_t num_bytes);
char *ofs_fgets(char *p_dest, int size, tOFS_FILE *p_file);
int ofs_fseek(tOFS_FILE *p_file, long offset, int whence);
int ofs_tell(struct fs_file_t *zfp);
int ofs_sync(struct fs_file_t *zfp);
int ofs_unlink(const char *path);
int ofs_stat(const char *path, struct fs_dirent *entry);

/* directory functions */
int ofs_opendir(tOFS_DIR *zdp, const char *path);
int ofs_closedir(tOFS_DIR *zdp);
int ofs_readdir(tOFS_DIR *zdp, tOFS_DIRENT *entry);
int ofs_readmount(int *index, const char **name);
void ofs_dir_t_init(struct fs_dir_t *zdp);
int ofs_statvfs(const char *path, struct fs_statvfs *stat);

/* initialization */
int otdoa_fs_init(void);

#ifndef SEEK_SET
#define SEEK_SET FS_SEEK_SET
#define SEEK_CUR FS_SEEK_CUR
#define SEEK_END FS_SEEK_END
#endif

#else
/* Host Linux (e.g. unit test) targets that use linux-syle FS */
#include <stdio.h>
#include <unistd.h>
#include <sys/stat.h>
typedef FILE tOFS_FILE;
typedef struct stat tOFS_DIRENT;

#define FOPEN(_p, _f)	       fopen(_p, _f)
#define FCLOSE(_p)	       fclose(_p)
#define FREAD(_b, _s, _n, _f)  fread(_b, _s, _n, _f)
#define FWRITE(_b, _s, _n, _f) fwrite(_b, _s, _n, _f)
#define FSEEK(_f, _o, _w)      fseek(_f, _o, _w)
#define FGETS(_b, _s, _f)      fgets(_b, _s, _f)
#define FUNLINK(_b)	       unlink(_b)
#define FTELL(_f)	       ftell(_f)
#define FSTAT(_p, _e)	       stat(_p, _e)
#endif

#endif /* _OTDOA_FS_H_ */
