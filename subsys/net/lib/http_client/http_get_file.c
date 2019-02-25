#include <bsd.h>
#include <net/http_client.h>
#include <net/http_get_file.h>
#include <logging/log.h>
#include <net/socket.h>
#include <zephyr/types.h>
#include <stdio.h>
#include <strings.h>

LOG_MODULE_REGISTER(http_get_file);

const u32_t family = AF_INET;
const u32_t proto = IPPROTO_TCP;

static int get_content_length(char *rsp)
{
	u32_t len = 0;
	u32_t accumulator = 1;
	char *start = rsp;

	LOG_DBG("Looking for content len");
	while (strncasecmp(rsp++, "Content-Length: ", 16) != 0) {
		if (rsp - start > 500) {
			return -1;
		}
	}
	LOG_DBG("Found 'Content-Length:' at address: %p", rsp);
	rsp += 16;
	start = rsp-1;

	while (*(rsp++) != '\n')
		;

	rsp -= 2;

	while (rsp-- != start) {
		u32_t val = rsp[0] - '0';

		LOG_DBG("Converting: %c, val: %d, current len: %d", rsp[0],
				val, len);
		len += val * accumulator;
		accumulator *= 10;
	}

	LOG_DBG("Length: %d", len);
	return len;
}

static unsigned int get_chunk_size(char *rsp)
{
	char *plen = rsp;
	unsigned int len = 0;

	LOG_DBG("Getting chunk size");
	while (*plen == '\r' || *plen == '\n')
		plen++;
	while (*plen != '\r' && *plen != '\n') {
		char c = *plen;

		len <<= 4;
		if (c <= 'F' && c >= 'A')
			len += c-'A'+10;
		else if (*plen <= '9')
			len += c-'0';
		else
			len += c-'a'+10;
		plen++;
	}
	return len;
}

int read_chunk(int fd, char *buf, size_t buf_len, int read_len,
				void (*callback)(char *, int))
{
	int recv_len = 0;
	int reqv_len;

	while (1) {
		reqv_len = (buf_len < (read_len - recv_len)) ?
					buf_len : (read_len - recv_len);
		if (reqv_len == 0)
			break;
		int len = httpc_recv(fd, buf, reqv_len, 0);

		callback(buf, len);
		recv_len += len;
	}
	return recv_len;
}

int http_get_file(struct get_file_param *param)
{
	int len = 0;
	int fd;
	int tot_len = 0;
	unsigned int payload_size = 0;
	int payload_idx;

	bsd_init();

	snprintf(param->req_buf, param->req_buf_len,
			"GET /%s HTTP/1.1\r\nHost: %s\r\n\r\n", param->filename,
			param->host);

	fd = httpc_connect(param->host, NULL, family, proto);
	if (fd < 0) {
		LOG_ERR("httpc_connect() failed, err %d", errno);
		return -1;
	}

	/* Get first load of packet, extract payload size */
	(void) httpc_request(fd, param->req_buf, strlen(param->req_buf));

	while (1) {
		len = httpc_recv(fd, param->resp_buf, param->resp_buf_len,
			MSG_PEEK);
		if (len == -1) {
			if (errno == EAGAIN) {
				continue;
			}
			LOG_ERR("httpc_recv() failed, err %d", errno);
			return -1;
		}
		char *pstr = strstr(param->resp_buf, "\r\n\r\n");

		if (pstr != NULL) {
			payload_idx = pstr - param->resp_buf;
			if (payload_idx > 0) {
				payload_idx += 4;
				break;
			}
		}
	}
	payload_size = get_content_length(param->resp_buf);
	if (payload_size == -1) {
		/* Assuming Transfer-Encoding: chunked when
		 * Content Length not found
		 */
		/* Remove header except "\r\n": */
		httpc_recv(fd, param->resp_buf, payload_idx-2, 0);
		while (1) {
			memset(param->resp_buf, 0, 20);
			/* Read chunk size */
			httpc_recv(fd, param->resp_buf, 20, MSG_PEEK);
			payload_size = get_chunk_size(param->resp_buf);
			if (payload_size == 0)
				break;
			char *pstr = strstr(&param->resp_buf[2], "\r\n");

			if (pstr == NULL) {
				LOG_ERR("Unrecognized response");
				return -1;
			}
			payload_idx = pstr - param->resp_buf;
			if (payload_idx > 0)
				payload_idx += 2;
			/* Remove chunk size: */
			len = httpc_recv(fd, param->resp_buf, payload_idx, 0);
			/* Read chunk: */
			tot_len += read_chunk(fd, param->resp_buf, param->resp_buf_len,
						payload_size, param->callback);
		}
	} else {
		/* Found Content-Length, Read everything in one go */
		/* Remove header: */
		httpc_recv(fd, param->resp_buf, payload_idx, 0);
		/* Read chunk: */
		tot_len += read_chunk(fd, param->resp_buf, param->resp_buf_len,
					payload_size, param->callback);
	}
	return tot_len;
}
