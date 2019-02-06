/*
 * Copyright (c) 2014 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
 */

#include <logging/log.h>
#define LOG_LEVEL CONFIG_NRF_COAP_LOG_LEVEL
LOG_MODULE_REGISTER(coap_transport);

#include <stdint.h>
#include <string.h>
#include <stdbool.h>
#include <errno.h>
#include <fcntl.h>

#include <net/coap_api.h>
#include <net/coap_transport.h>

#include "coap.h"

#if !(defined COAP_SESSION_COUNT)
#define COAP_SESSION_COUNT 0
#endif

/** Maximum sockets that can be managed by this module. */
#define COAP_SOCKET_COUNT (COAP_PORT_COUNT + COAP_SESSION_COUNT)

/**@brief UDP port information. */
typedef struct {
	/** Socket identifier. */
	int socket_fd;

	/** Local endpoint - address and port. Provision for maximum size. */
	struct sockaddr_in6 local;
} transport_t;

/**@brief Session information. */
typedef struct {
	/** Remote endpoint - address and port. Provision for maximum size. */
	struct sockaddr_in6 remote;

	transport_t *local;
	/** Local endpoint associated with the session. */
} session_t;

/** Table maintaining association between CoAP local ports and corresponding
 *  UDP socket identifiers.
 */
static transport_t port_table[COAP_SOCKET_COUNT];

#if (COAP_SESSION_COUNT > 0)
/** Table maintaining association between CoAP remote and end point used for
 *  a session.
 */
static session_t session_table[COAP_SESSION_COUNT];
#endif /* COAP_SESSION_COUNT */

/**@brief Internal method to get address length based on the address family.
 *
 * @note The internal method relies on the calling function to have done
 *       necessary parameter validation before calling this method. These
 *       validation include NULL parameter check and address family validation.
 *
 * @param addr Address whose length is requested.
 *
 * @retval sizeof(sockaddr_in) for address family AF_INET, else,
 *         sizeof(sockaddr_in6).
 */
static socklen_t address_length_get(struct sockaddr const *addr)
{
	return (addr->sa_family == AF_INET) ? sizeof(struct sockaddr_in) :
					      sizeof(struct sockaddr_in6);
}

#if (COAP_SESSION_COUNT > 0)
/**@brief Internal method to compare address.
 *
 * @note The internal method relies on the calling function to have done
 *       necessary parameter validation before calling this method. These
 *       validation include NULL parameter check and address family validation.
 *
 * @param addr1 Address to be compared.
 * @param addr2 Address against which addr1 is compared with.
 *
 * @retval true if the addresses match, else, false.
 */
static bool address_compare(struct sockaddr const *addr1,
			    struct sockaddr const *addr2)
{
	if (addr1->sa_family != addr2->sa_family) {
		return false;
	}

	if (addr1->sa_family == AF_INET) {
		const struct sockaddr_in *addr4_1 = (struct sockaddr_in *)addr1;
		const struct sockaddr_in *addr4_2 = (struct sockaddr_in *)addr2;

		if (addr4_1->sin_port == addr4_2->sin_port) {
			if (memcmp(&addr4_1->sin_addr, &addr4_2->sin_addr,
				   sizeof(struct in_addr)) == 0) {
				return true;
			}
		}
	} else if (addr1->sa_family == AF_INET6) {
		const struct sockaddr_in6 *addr6_1 =
						(struct sockaddr_in6 *)addr1;
		const struct sockaddr_in6 *addr6_2 =
						(struct sockaddr_in6 *)addr2;

		if (addr6_1->sin6_port == addr6_2->sin6_port) {
			if (memcmp(&addr6_1->sin6_addr, &addr6_2->sin6_addr,
				   sizeof(struct in6_addr)) == 0) {
				return true;
			}
		}
	}

	return false;
}
#endif /* (COAP_SESSION_COUNT > 0) */

#if (COAP_SESSION_COUNT > 0)
/**@brief Internal method to free a session.
 *
 * @note The internal method relies on the calling function to have done
 *       necessary parameter validation before calling this method, that is,
 *       session shall not be NULL.
 *
 * @param session Identifies the session being freed.
 */
static void session_free(session_t *session)
{
	close(session->local->socket_fd);
	memset(session->local, 0, sizeof(transport_t));
	memset(session, 0, sizeof(session_t));
}


/**@brief Internal method to find a session based on given local and remote
 *        endpoints.
 *
 * @note The internal method relies on the calling function to have done
 *       necessary parameter validation before calling this method.
 *       NULL check and any address family checks must be performed by the
 *       calling function.
 *
 * @param local Identifies the local endpoint.
 * @param remote Identifies the remote endpoint.
 *
 * @retval A valid session if the procedure succeeds, else, NULL.
 */
static session_t *session_find(const struct sockaddr *local,
			       const struct sockaddr *remote)
{
	session_t *session;

	for (int index = 0; index < COAP_SESSION_COUNT; index++) {
		session = &session_table[index];
		if (address_compare(remote,
				    (struct sockaddr *)&session->remote)) {
			/* Verify if the local endpoint, if any, match. */
			if ((session->local != NULL) &&
			    (address_compare(
				local,
				(struct sockaddr *)&session->local->local))) {
				/* Session already exists. */
				/* TODO: we do not compare if the security
				 * parameters used are the same yet.
				 */
				return session;
			}
		}
	}

	return NULL;
}
#endif /* (COAP_SESSION_COUNT > 0) */


/**@brief Internal method to find the session or port corresponding to the
 *        handle.
 *
 * @param[in] handle Identifies the socket descriptor, also the the handle for
 *                   the transport.
 *
 * @retval Pointer to a valid transport in case of success, else, NULL.
 */
static int local_endpoint_find(coap_transport_handle_t handle)
{
	for (int index = 0; index < COAP_SOCKET_COUNT; index++) {
		if (port_table[index].socket_fd == handle) {
			return index;
		}
	}

	return -1;
}

#if (COAP_SESSION_COUNT > 0)
/**@brief Internal method to find the session or port corresponding to the
 *        handle.
 *
 * @param[in] index Identifies the index of port_table.
 *
 * @retval true if the endpoint was secure else false.
 */
static bool secure_endpoint_check(u32_t index)
{
	return index < COAP_PORT_COUNT ? false : true;
}
#endif

/**@brief Creates socket and binds to local address and port as requested port.
 *
 * @details Creates port as requested in port.
 *
 * @param[in] index Index to the port_table where entry of the port created is
 *                  to be made.
 * @param[in] local Port information to be created.
 *
 * @retval -1 if the menthod failed, else, the socket descriptor.
 */
static coap_transport_handle_t socket_create_and_bind(u32_t index,
						      coap_local_t *local)
{
	/* Request new socket creation. */
	const int family = local->addr->sa_family;

	int socket_fd = socket(family, SOCK_DGRAM, local->protocol);

	socklen_t address_len = address_length_get(local->addr);

	if (socket_fd != -1) {
		if (local->protocol == IPPROTO_DTLS_1_2) {
			/* Set the security configuration for the socket. */
			int err = setsockopt(socket_fd, SOL_TLS, TLS_DTLS_ROLE,
					     &local->setting->role,
					     sizeof(int));
			if (err == 0) {
				err = setsockopt(socket_fd, SOL_TLS,
						 TLS_SEC_TAG_LIST,
						 local->setting->sec_tag_list,
						 (local->setting->sec_tag_count
						 * sizeof(sec_tag_t)));
			}

			if (err) {
				/* Not all procedures succeeded with the socket
				 * creation and initialization, hence free it.
				 */
				(void)close(socket_fd);
				return -1;
			}
		}

		int retval = bind(socket_fd, local->addr, address_len);

		if (retval != -1) {
			memcpy(&port_table[index].local, local->addr,
			       address_len);
			port_table[index].socket_fd = socket_fd;
		} else {
			/* Not all procedures succeeded with the socket creation
			 * and initialization, hence free it.
			 */
			(void)close(socket_fd);
			return -1;
		}
	}

	return socket_fd;
}


u32_t coap_transport_init(coap_transport_init_t *param)
{
	u32_t index;

	NULL_PARAM_CHECK(param);
	NULL_PARAM_CHECK(param->port_table);

#if (COAP_SESSION_COUNT > 0)
	memset(session_table, 0, sizeof(session_table));
#endif /* (COAP_SESSION_COUNT > 0) */

	for (index = 0; index < COAP_PORT_COUNT; index++) {
		coap_transport_handle_t transport;

		/* Create end point for each of the CoAP ports. */
		transport = socket_create_and_bind(index,
						   &param->port_table[index]);
		if (transport == -1) {
			/* TODO: close any previous sockets? */
			return EIO;
		}

		param->port_table[index].transport =
				port_table[index].socket_fd;
	}

	return 0;
}


u32_t coap_transport_write(const coap_transport_handle_t transport,
			   const struct sockaddr *remote, const u8_t *data,
			   u16_t datalen)
{
	u32_t err_code = ENOENT;
	int retval;
	int index;

	NULL_PARAM_CHECK(remote);
	NULL_PARAM_CHECK(data);

	COAP_MUTEX_UNLOCK();

	index = local_endpoint_find(transport);
	if (index == -1) {
		err_code = EBADF;
	} else {

#if (COAP_SESSION_COUNT > 0)
		if (secure_endpoint_check(index)) {
			retval = send(transport, data, datalen, 0);
		} else
#endif /* (COAP_SESSION_COUNT > 0) */
		{
			/* Send on UDP port. */
			retval = sendto(transport, data, datalen, 0, remote,
					address_length_get(remote));
		}

		if (retval == -1) {
			/* Error in sendto. */
			err_code = EIO;
		} else {
			err_code = 0;
		}
	}

	COAP_MUTEX_LOCK();

	return err_code;
}


void coap_transport_process(void)
{
	/* Intentionally empty. */
}


u32_t coap_security_setup(coap_local_t *local, struct sockaddr const *remote)
{
	NULL_PARAM_CHECK(local);
	NULL_PARAM_CHECK(remote);
	NULL_PARAM_CHECK(local->addr);
	NULL_PARAM_CHECK(local->setting);

	if (local->protocol != IPPROTO_DTLS_1_2) {
		return EINVAL;
	}
	if ((remote->sa_family != AF_INET) && (remote->sa_family != AF_INET6)) {
		return EINVAL;
	}
	if ((local->addr->sa_family != AF_INET)  &&
	    (local->addr->sa_family != AF_INET6)) {
		return EINVAL;
	}
#if (COAP_SESSION_COUNT > 0)
	/* Search if the entry already exists in the port table. */
	session_t *session = session_find(local->addr, remote);

	if (session != NULL) {
		local->transport = session->local->socket_fd;

		/* Note that we do not validate if the security parameters
		 * requested match the existing session or not.
		 * TODO: Do something about this.
		 */
		return 0;
	}

	/* The session does not exist, we create one. */
	for (int index = 0; index < COAP_SESSION_COUNT; index++) {
		session = &session_table[index];

		if (session->local == NULL) {
			coap_transport_handle_t transport;

			/* We have a free slot available.
			 * Lets create a socket for the session.
			 * Note that the first COAP_PORT_COUNT are already
			 * created on init.
			 * So, we now request an entry at COAP_PORT_COUNT+index.
			 */
			const int port_entry = COAP_PORT_COUNT + index;

			transport = socket_create_and_bind(port_entry, local);

			if (transport != -1) {
				int flags = 0;

				session->local = &port_table[port_entry];

				if (local->non_blocking) {
					/* Set non-blocking mode. */
					flags = fcntl(session->local->socket_fd,
						      F_GETFL, 0);
					if ((flags == -1) ||
					    (fcntl(session->local->socket_fd,
						   F_SETFL,
						   flags | O_NONBLOCK) == -1)) {
						session_free(session);
						return EIO;
					}
				}

				/* Initiate a connection. */
				int err = connect(session->local->socket_fd,
						  remote,
						  address_length_get(remote));

				if (err && errno != EINPROGRESS) {
					/* Free the allocated session. */
					session_free(session);
					return EIO;
				}

				if (local->non_blocking) {
					/* Remove non-blocking mode. */
					if (fcntl(session->local->socket_fd,
						  F_SETFL, flags) == -1) {
						session_free(session);
						return EIO;
					}
				}

				memcpy(&session->remote, remote,
				       address_length_get(remote));
				local->transport = transport;

				return err ? errno : 0;
			}
		}
	}
#endif /* (COAP_SESSION_COUNT > 0) */
	return ENOMEM;
}


u32_t coap_security_destroy(coap_transport_handle_t transport)
{
#if (COAP_SESSION_COUNT > 0)
	int index = local_endpoint_find(transport);

	if (index == -1) {
		return EBADF;
	}

	if (secure_endpoint_check(index)) {
		session_t   *session = &session_table[index - COAP_PORT_COUNT];

		/* Free the session. */
		session_free(session);
		return 0;
	}
#endif /* (COAP_SESSION_COUNT > 0) */

	return ENOENT;
}


/* lint --e{14} */
/*suppress "Symbol 'coap_transport_input(void)' previously defined" (WEAK) */
void coap_transport_input(void)
{
	socklen_t address_length;
	struct sockaddr_in remote4;
	struct sockaddr_in remote6;
	struct sockaddr *remote;
	struct sockaddr *local;
	transport_t *port;

	static u8_t read_mem[COAP_MESSAGE_DATA_MAX_SIZE];

#if (COAP_SESSION_COUNT > 0)
	for (u32_t index = 0; index < COAP_SESSION_COUNT; index++) {
		const int port_entry = index + COAP_PORT_COUNT;

		port = &port_table[port_entry];

		int bytes_read = recv(port->socket_fd, read_mem,
				      COAP_MESSAGE_DATA_MAX_SIZE, MSG_DONTWAIT);

		if (bytes_read >= 0) {
			const session_t *session = &session_table[index];

			/* Notify the CoAP module of received data. */
			int retval = coap_transport_read(
					port->socket_fd,
					(struct sockaddr *)&session->remote,
					(struct sockaddr *)&port->local,
					0, read_mem, (u16_t)bytes_read);

			/* Nothing much to do if CoAP could not interpret the
			 * datagram.
			 */
			(void)(retval);
		}
	}
#endif /* (COAP_SESSION_COUNT > 0) */

	for (u32_t index = 0; index < COAP_PORT_COUNT; index++) {
		port = &port_table[index];
		local = (struct sockaddr *)&port->local;

		if ((local->sa_family) == AF_INET6) {
			address_length = sizeof(struct sockaddr_in6);
			remote = (struct sockaddr *)&remote6;
		} else {
			address_length = sizeof(struct sockaddr_in);
			remote = (struct sockaddr *)&remote4;
		}

		int bytes_read = recvfrom(port->socket_fd,
					  read_mem,
					  COAP_MESSAGE_DATA_MAX_SIZE,
					  MSG_DONTWAIT,
					  remote,
					  &address_length);
		if (bytes_read >= 0) {
			/* Notify the CoAP module of received data. */
			int retval = coap_transport_read(
					port->socket_fd,
					remote, local, 0, read_mem,
					(u16_t)bytes_read);

			/* Nothing much to do if CoAP could not interpret the
			 * datagram.
			 */
			(void)(retval);
		} else {
			/* Error in recvfrom(). Placeholder for debugging. */
		}
	}
}
