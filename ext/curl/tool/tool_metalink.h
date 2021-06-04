#ifndef HEADER_CURL_TOOL_METALINK_H
#define HEADER_CURL_TOOL_METALINK_H
/***************************************************************************
 *                                  _   _ ____  _
 *  Project                     ___| | | |  _ \| |
 *                             / __| | | | |_) | |
 *                            | (__| |_| |  _ <| |___
 *                             \___|\___/|_| \_\_____|
 *
 * Copyright (C) 1998 - 2020, Daniel Stenberg, <daniel@haxx.se>, et al.
 *
 * This software is licensed as described in the file COPYING, which
 * you should have received as part of this distribution. The terms
 * are also available at https://curl.haxx.se/docs/copyright.html.
 *
 * You may opt to use, copy, modify, merge, publish, distribute and/or sell
 * copies of the Software, and permit persons to whom the Software is
 * furnished to do so, under the terms of the COPYING file.
 *
 * This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
 * KIND, either express or implied.
 *
 ***************************************************************************/
#include "tool_setup.h"
#include "tool_sdecls.h"

struct GlobalConfig;
struct OperationConfig;

/* returns 1 for success, 0 otherwise (we use OpenSSL *_Init fncs directly) */
typedef int (*digest_init_func)(void *context);

typedef void (*digest_update_func)(void *context,
                                   const unsigned char *data,
                                   unsigned int len);
typedef void (*digest_final_func)(unsigned char *result, void *context);

struct digest_params {
  digest_init_func     digest_init;   /* Initialize context procedure */
  digest_update_func   digest_update; /* Update context with data */
  digest_final_func    digest_final;  /* Get final result procedure */
  unsigned int         digest_ctxtsize;  /* Context structure size */
  unsigned int         digest_resultlen; /* Result length (bytes) */
};

struct digest_context {
  const struct digest_params *digest_hash; /* Hash function definition */
  void                  *digest_hashctx;   /* Hash function context */
};

struct metalink_digest_def {
  const char *hash_name;
  const struct digest_params *dparams;
};

struct metalink_digest_alias {
  const char *alias_name;
  const struct metalink_digest_def *digest_def;
};

struct metalink_checksum {
  const struct metalink_digest_def *digest_def;
  /* raw digest value, not ascii hex digest */
  unsigned char *digest;
};

struct metalink_resource {
  struct metalink_resource *next;
  char *url;
};

struct metalinkfile {
  struct metalinkfile *next;
  char *filename;
  struct metalink_checksum *checksum;
  struct metalink_resource *resource;
};

#ifdef USE_METALINK

/*
 * curl requires libmetalink 0.1.0 or newer
 */
#define CURL_REQ_LIBMETALINK_MAJOR  0
#define CURL_REQ_LIBMETALINK_MINOR  1
#define CURL_REQ_LIBMETALINK_PATCH  0

#define CURL_REQ_LIBMETALINK_VERS  ((CURL_REQ_LIBMETALINK_MAJOR * 10000) + \
                                    (CURL_REQ_LIBMETALINK_MINOR * 100) + \
                                     CURL_REQ_LIBMETALINK_PATCH)

extern const struct digest_params MD5_DIGEST_PARAMS[1];
extern const struct digest_params SHA1_DIGEST_PARAMS[1];
extern const struct digest_params SHA256_DIGEST_PARAMS[1];

#include <metalink/metalink.h>

/*
 * Counts the resource in the metalinkfile.
 */
int count_next_metalink_resource(struct metalinkfile *mlfile);

void delete_metalinkfile(struct metalinkfile *mlfile);
void clean_metalink(struct OperationConfig *config);

/*
 * Performs final parse operation and extracts information from
 * Metalink and creates metalinkfile structs.
 *
 * This function returns 0 if it succeeds without warnings, or one of
 * the following negative error codes:
 *
 * -1: Parsing failed; or no file is found
 * -2: Parsing succeeded with some warnings.
 */
int parse_metalink(struct OperationConfig *config, struct OutStruct *outs,
                   const char *metalink_url);

/*
 * Callback function for CURLOPT_WRITEFUNCTION
 */
size_t metalink_write_cb(void *buffer, size_t sz, size_t nmemb,
                         void *userdata);

/*
 * Returns nonzero if content_type includes "application/metalink+xml"
 * media-type. The check is done in case-insensitive manner.
 */
int check_metalink_content_type(const char *content_type);

/*
 * Check checksum of file denoted by filename.
 *
 * This function returns 1 if the checksum matches or one of the
 * following integers:
 *
 * 0:
 *   Checksum didn't match.
 * -1:
 *   Could not open file; or could not read data from file.
 * -2:
 *   No checksum in Metalink supported, hash algorithm not available, or
 *   Metalink does not contain checksum.
 */
int metalink_check_hash(struct GlobalConfig *config,
                        struct metalinkfile *mlfile,
                        const char *filename);

/*
 * Release resources allocated at global scope.
 */
void metalink_cleanup(void);

#else /* USE_METALINK */

#define count_next_metalink_resource(x)  0
#define delete_metalinkfile(x)  (void)x
#define clean_metalink(x)  (void)x

/* metalink_cleanup() takes no arguments */
#define metalink_cleanup() Curl_nop_stmt

#endif /* USE_METALINK */

#endif /* HEADER_CURL_TOOL_METALINK_H */
