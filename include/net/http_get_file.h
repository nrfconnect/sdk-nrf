#include <stddef.h>

struct get_file_param {
	const char *const host;
	const char *const port;
	const char *filename;
	char *req_buf;
	size_t req_buf_len;
	char *resp_buf;
	size_t resp_buf_len;
	void (*callback)(char *, int);
};

int http_get_file(struct get_file_param *param);
