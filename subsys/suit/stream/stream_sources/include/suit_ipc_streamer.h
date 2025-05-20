/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef IPC_STREAMER_H__
#define IPC_STREAMER_H__

/*
 * SDFW (Secure Domain Firmware) is unable to fetch images from external sources on its own.
 * Instead - it is able to invoke companion image(s) on MCU(s) belonging to other domain(s),
 * typically Application MCU. Such companion image contains instructions to fetch required
 * image from image source and deliver it to SDFW via IPC (Inter-processor communication)
 * in form of image chunks.
 *
 * Implementation choice about supported protocols (i.e. http, ftp) is delegated out of SDFW,
 * allowing to keep SDFW image size and security risks manageable.
 *
 * Solution is composed of two parts - ipc streamer requestor executed on Secure Domain MCU and
 * ipc streamer provider executed on local Domain MCU, typically Application Domain
 *
 * Once image chunk is delivered to streamer requestor, information about chunk is enqueued
 * for future processing. This enables simultonous processing (i.e. writing to NVM) of previously
 * obtained chunk by the streamer requestor and preparing (i.e. receiving) new chunk
 * by the streamer provider
 */
#include <suit_sink.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Provided by streamer requestor to its clients, i.e. SUIT fetch directive implementation.
 * Streams an image from source to sink
 *
 * @param[in]   resource_id		Resource identifier, typically in form of URI. Not
 *interpreted by streamer requestor but just passed to streamer provider
 *
 * @param[in]   resource_id_length	Length of resource_id, in bytes
 *
 * @param[in]   sink			Function pointers to pass image chunk to next chunk
 *processing element. Non-'null' write_ptr is always required. Non-'null' seek_ptr is required if
 *streamer provider delivers image in non-continuous chunks. In event of failure of any sink-related
 *operations suit_ipc_streamer_stream will fail as well.
 *
 * @param[in]   inter_chunk_timeout_ms	Function will fail (timeout) in case of streamer provider
 *inactivity period longer then this parameter value
 *
 * @param[in]   requesting_period_ms	streamer requestor will notify streamer provider
 *periodically about pending image request. It will do this till first response from streamer
 *provider or till inter-chunk timeout is triggered.
 *
 * @return SUIT_PLAT_SUCCESS on success
 *		SUIT_PLAT_ERR_NOMEM not enough resources to open new session
 *		SUIT_PLAT_ERR_CRASH one of sink operations, i.e. write, seek, failed
 *		SUIT_PLAT_ERR_TIME timeout error. streamer provider did not respond or stopped
 *to respond
 */
suit_plat_err_t suit_ipc_streamer_stream(const uint8_t *resource_id, size_t resource_id_length,
			struct stream_sink *sink, uint32_t inter_chunk_timeout_ms,
			uint32_t requesting_period_ms);

/**
 * @brief Initialization function for streamer requestor (when executed in SDFW) or streamer
 * requestor proxy (when executed on local Domain MCU). In case of execution on local Domain MCU -
 * It is called directly from suit_ipc_streamer_provider_init implementation.
 *
 * @return SUIT_PLAT_SUCCESS on success, error code in case of failure
 */
suit_plat_err_t suit_ipc_streamer_requestor_init(void);

/**
 * @brief Initialization function for streamer provider.
 *
 * @return SUIT_PLAT_SUCCESS on success, error code in case of failure
 */
suit_plat_err_t suit_ipc_streamer_provider_init(void);

typedef enum {
	PENDING = 0,
	PROCESSED = 1,
	REFUSED = 2
} suit_ipc_streamer_chunk_status_t;

typedef struct {
	uint32_t chunk_id;
	suit_ipc_streamer_chunk_status_t status;

} suit_ipc_streamer_chunk_info_t;

/**
 * @brief Enqueues image chunk for future processing. Implemented by streamer requestor and exposed
 *as streamer requestor proxy to local Domain MCU. Attention! Return code SUIT_PLAT_ERR_BUSY
 *does not mean irrecoverable failure. One of chunks will eventually be processed by streamer
 *requestor and necessary space will be reclaimed
 *
 *
 * @param[in]   stream_session_id	Session identifier - see
 *suit_ipc_streamer_missing_image_notify_fn
 *
 * @param[in]   chunk_id		Chunk identifier assigned by streamer provider, not
 *interpreted by streamer requestor, but provided back as output of
 *suit_ipc_streamer_chunk_status_req
 *Non-zero value assigned by streamer provider, unique for chunk in space of stream_session_id. It
 *is utilized by streamer provider for buffer mananagment.
 *
 * @param[in]   offset			Chunk offset in image.
 *
 * @param[in]   address			Location of memory buffer where image chunk is stored. Once
 *passed to streamer requestor
 *					- its content shall not be modified by streamer provider
 *till the end of processing by streamer requestor. See suit_ipc_streamer_chunk_status_req.
 *It is allowed *to pass 'null' address and 0 bytes size, to signalize seeking operation or
 *to report end of stream.
 *
 * @param[in]   size			Size of image chunk, in bytes
 *
 * @param[in]   last_chunk		'true' signalizes end of stream. Once end of stream is
 *signalized - streamer requestor will not accept any new chunks. It will process remaining enqueued
 *chunks and close session
 *
 * @return SUIT_PLAT_SUCCESS on success
 *		SUIT_PLAT_ERR_INVAL invalid parameter
 *		SUIT_PLAT_ERR_INCORRECT_STATE there is no open session identified by
 *		stream_session_id or session is closing, and another chunk cannot be accepted
 *		SUIT_PLAT_ERR_BUSY not enough space to enqueue this chunk. Try again later
 */
suit_plat_err_t suit_ipc_streamer_chunk_enqueue(uint32_t stream_session_id, uint32_t chunk_id,
						uint32_t offset, uint8_t *address, uint32_t size,
						bool last_chunk);

/**
 * @brief Returns information about chunks enqueued by streamer requestor. Implemented by streamer
 *requestor and exposed as streamer requestor proxy to local Domain MCU. Attention! Return code
 *SUIT_PLAT_ERR_BUSY does not mean irrecoverable failure. Every execution of this function
 *causes that streamer requestor releases information about 'PROCESSED' or 'REFUSED' chunks, so
 *function will succeed eventually.
 *
 * @param[in]   stream_session_id	Session identifier - see
 *suit_ipc_streamer_missing_image_notify_fn
 *
 * @param[out]  chunk_info		An array, holding information about chunks enqueued by
 *streamer requestor. At time when chunk is passed to streamer requestor - see
 *suit_ipc_streamer_chunk_enqueue, its status is set to 'PENDING'
 *- see suit_ipc_streamer_chunk_status_t.
 *Once *chunk processing is completed, its status is set to 'PROCESSED' or 'REFUSED' based on
 *processing results.
 *
 *
 * @param[in,out]  chunk_info_count	as input - maximal amount of elements an chunk_info array
 *can hold, as output - amount of stored elements
 *
 *
 * @retval SUIT_PLAT_SUCCESS on success
 * @retval SUIT_PLAT_ERR_INVAL invalid parameter, i.e. null pointer
 * @retval SUIT_PLAT_ERR_INCORRECT_STATE there is no open session identified by stream_session_id
 * @retval SUIT_PLAT_ERR_BUSY chunk_info array is too small
 *
 */
suit_plat_err_t suit_ipc_streamer_chunk_status_req(uint32_t stream_session_id,
						   suit_ipc_streamer_chunk_info_t *chunk_info,
						   size_t *chunk_info_count);

/**
 * @brief Chunk status update notification function prototype
 *
 * @param[in]   stream_session_id	Session identifier. Non-zero value assigned by streamer
 *requestor, unique for image request.
 *
 * @param[in]   context			parameter of suit_ipc_streamer_chunk_status_evt_subscribe
 *provided by streamer requestor or streamer requestor proxy
 *
 * @return SUIT_PLAT_ERR_INCORRECT_STATE on success, error code in case of failure
 */
typedef suit_plat_err_t (*suit_ipc_streamer_chunk_status_notify_fn)(uint32_t stream_session_id,
								    void *context);

/**
 * @brief Registers chunk status update notification function. Implemented individually by streamer
 *requestor and streamer requestor proxy.
 *
 * @param[in]   notify_fn		see suit_ipc_streamer_chunk_status_notify_fn
 *
 * @param[in]   context			parameter not interpreted by streamer requestor or streamer
 *requestor proxy, provided back to suit_ipc_streamer_chunk_status_notify_fn
 *
 * @return SUIT_PLAT_SUCCESS on success,
 *		SUIT_PLAT_ERR_NOMEM not enough space to register another notification function
 *
 */
suit_plat_err_t suit_ipc_streamer_chunk_status_evt_subscribe(
					    suit_ipc_streamer_chunk_status_notify_fn notify_fn,
					    void *context);

/**
 * @brief Unregisters chunk status update notification function. Implemented individually by
 * streamer requestor and streamer requestor proxy.
 *
 */
void suit_ipc_streamer_chunk_status_evt_unsubscribe(void);

/**
 * @brief Missing image notification function prototype
 *
 * @param[in]   resource_id		requested resource identifier, typically URI. See
 *suit_ipc_streamer_stream
 *
 * @param[in]   resource_id_length	Length of resource_id, in bytes
 *
 * @param[in]   stream_session_id	Session identifier. Non-zero value assigned by streamer
 *requestor, unique for image request, as result of suit_ipc_streamer_stream call
 *
 * @param[in]   context			parameter of suit_ipc_streamer_missing_image_evt_subscribe
 *provided by streamer requestor or streamer requestor proxy
 *
 * @return SUIT_PLAT_SUCCESS on success, error code in case of failure
 */
typedef suit_plat_err_t (*suit_ipc_streamer_missing_image_notify_fn)(const uint8_t *resource_id,
								     size_t resource_id_length,
								     uint32_t stream_session_id,
								     void *context);

/**
 * @brief Registers missing image notification function. Implemented individually by streamer
 *requestor and streamer requestor proxy.
 *
 * @param[in]   notify_fn		see suit_ipc_streamer_missing_image_notify_fn
 *
 * @param[in]   context			parameter not interpreted by streamer requestor or streamer
 *requestor proxy, provided back to suit_ipc_streamer_missing_image_notify_fn
 *
 * @return SUIT_PLAT_SUCCESS on success,
 *		SUIT_PLAT_ERR_NOMEM not enough space to register another notification function
 *
 */
suit_plat_err_t suit_ipc_streamer_missing_image_evt_subscribe(
					     suit_ipc_streamer_missing_image_notify_fn notify_fn,
					     void *context);

/**
 * @brief Unregisters missing image notification function. Implemented individually by streamer
 * requestor and streamer requestor proxy.
 *
 */
void suit_ipc_streamer_missing_image_evt_unsubscribe(void);

#ifdef __cplusplus
}
#endif

#endif /* IPC_STREAMER_H__ */
