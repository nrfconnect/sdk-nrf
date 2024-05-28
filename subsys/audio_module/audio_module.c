/*
 * Copyright(c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include "audio_module/audio_module.h"

#include <stdlib.h>
#include <stdio.h>
#include <ctype.h>
#include <zephyr/kernel.h>
#include <zephyr/shell/shell.h>
#include <data_fifo.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(audio_module, CONFIG_AUDIO_MODULE_LOG_LEVEL);

/* Define a timeout to prevent system locking */
#define LOCK_TIMEOUT_US (K_USEC(100))

/**
 * @brief Helper function to validate the module state.
 *
 * @param state  [in]  The module's state.
 *
 * @return true if state is not undefined, false otherwise.
 */
static bool state_not_undefined(enum audio_module_state state)
{
	if (state == AUDIO_MODULE_STATE_CONFIGURED || state == AUDIO_MODULE_STATE_RUNNING ||
	    state == AUDIO_MODULE_STATE_STOPPED) {
		return true;
	}

	return false;
}

/**
 * @brief Helper function to validate the module is running, does not check if a valid state.
 *
 * @param state  [in]  The module's state.
 *
 * @return true if in running state, false otherwise.
 */
static bool state_running(enum audio_module_state state)
{
	if (state == AUDIO_MODULE_STATE_RUNNING) {
		return true;
	}

	return false;
}

/**
 * @brief Helper function to validate the module type.
 *
 * @param type  [in]  The module's type.
 *
 * @return true if not type undefined, false otherwise.
 */
static bool type_not_undefined(enum audio_module_type type)
{
	if (type == AUDIO_MODULE_TYPE_INPUT || type == AUDIO_MODULE_TYPE_OUTPUT ||
	    type == AUDIO_MODULE_TYPE_IN_OUT) {
		return true;
	}

	return false;
}

/**
 * @brief Helper function to validate that the module is one of the possible input types.
 *
 * @param type  [in]  The module's type.
 *
 * @return true if possible input type, false otherwise.
 */
static bool has_input_type(enum audio_module_type type)
{
	if (type == AUDIO_MODULE_TYPE_INPUT || type == AUDIO_MODULE_TYPE_IN_OUT) {
		return true;
	}

	return false;
}

/**
 * @brief Helper function to validate that the module is one of the possible output types.
 *
 * @param type  [in]  The module's type.
 *
 * @return true if possible output type, false otherwise.
 */
static bool has_output_type(enum audio_module_type type)
{
	if (type == AUDIO_MODULE_TYPE_OUTPUT || type == AUDIO_MODULE_TYPE_IN_OUT) {
		return true;
	}

	return false;
}

/**
 * @brief Helper function to validate the module parameters.
 *
 * @param parameters  [in]  The module parameters.
 *
 * @return true if valid parameters, false otherwise.
 */
static bool validate_parameters(struct audio_module_parameters const *const parameters)
{
	if (parameters == NULL) {
		LOG_ERR("No parameters for module");
		return false;
	}

	if (parameters->description == NULL || !type_not_undefined(parameters->description->type) ||
	    parameters->description->name == NULL || parameters->description->functions == NULL) {
		return false;
	}

	if (parameters->description->functions->configuration_set == NULL ||
	    parameters->description->functions->configuration_get == NULL ||
	    parameters->description->functions->data_process == NULL) {
		return false;
	}

	if (parameters->thread.stack == NULL || parameters->thread.stack_size == 0) {
		return false;
	}

	return true;
}

/**
 * @brief General callback for releasing the data when inter-module data
 *        passing.
 *
 * @param handle      [in/out]  The handle of the sending modules instance.
 * @param audio_data  [in]      Pointer to the audio data to release.
 */
static void audio_data_release_cb(struct audio_module_handle_private *handle,
				  struct audio_data const *const audio_data)
{
	int ret;
	struct audio_module_handle *hdl = (struct audio_module_handle *)handle;

	ret = k_sem_take(&hdl->sem, K_NO_WAIT);
	if (ret) {
		LOG_ERR("Failed to take semaphore for data release callback function");
		return;
	}

	if (k_sem_count_get(&hdl->sem) == 0) {
		/* Audio data has been consumed by all modules so now can free the data memory. */
		k_mem_slab_free(hdl->thread.data_slab, (void **)&audio_data->data);
	}
}

/**
 * @brief Send an audio data item to a module, all data is consumed by the module.
 *
 * @param tx_handle            [in/out]  The handle for the sending module instance.
 * @param rx_handle            [in/out]  The handle for the receiving module instance.
 * @param audio_data           [in]      Pointer to the audio data to send to the module.
 * @param data_in_response_cb  [in]      A pointer to a callback to run when the buffer is
 *                                       fully consumed.
 *
 * @return 0 if successful, error otherwise.
 */
static int data_tx(struct audio_module_handle *tx_handle, struct audio_module_handle *rx_handle,
		   struct audio_data const *const audio_data,
		   audio_module_response_cb data_in_response_cb)
{
	int ret;
	struct audio_module_message *data_msg_rx;

	if (rx_handle->state == AUDIO_MODULE_STATE_RUNNING) {
		ret = data_fifo_pointer_first_vacant_get(rx_handle->thread.msg_rx,
							 (void **)&data_msg_rx, K_NO_WAIT);
		if (ret) {
			LOG_ERR("Module %s no free data buffer, ret %d", rx_handle->name, ret);
			return ret;
		}

		/* Copy. The audio data itself will remain in its original location. */
		memcpy(&data_msg_rx->audio_data, audio_data, sizeof(struct audio_data));
		data_msg_rx->tx_handle = tx_handle;
		data_msg_rx->response_cb = data_in_response_cb;

		ret = data_fifo_block_lock(rx_handle->thread.msg_rx, (void **)&data_msg_rx,
					   sizeof(struct audio_module_message));
		if (ret) {
			data_fifo_block_free(rx_handle->thread.msg_rx, (void **)&data_msg_rx);

			LOG_WRN("Module %s failed to queue audio data, ret %d", rx_handle->name,
				ret);
			return ret;
		}

		LOG_DBG("Audio data sent to module %s", rx_handle->name);

	} else {
		LOG_WRN("Receiving module %s is in an invalid state %d", rx_handle->name,
			rx_handle->state);
		return -ECANCELED;
	}

	return 0;
}

/**
 * @brief Send audio data item to the module's TX FIFO.
 *
 * @param handle      [in/out]  The handle for this modules instance.
 * @param audio_data  [in]      A pointer to the audio data.
 *
 * @return 0 if successful, error otherwise.
 */
static int tx_fifo_put(struct audio_module_handle *handle,
		       struct audio_data const *const audio_data)
{
	int ret;
	struct audio_module_message *data_msg_tx;

	/* Take a TX message. */
	ret = data_fifo_pointer_first_vacant_get(handle->thread.msg_tx, (void **)&data_msg_tx,
						 K_NO_WAIT);
	if (ret) {
		LOG_WRN("No free space in TX FIFO for module %s, ret %d", handle->name, ret);
		return ret;
	}

	/* Configure audio data. */
	memcpy(&data_msg_tx->audio_data, audio_data, sizeof(struct audio_data));
	data_msg_tx->tx_handle = handle;
	data_msg_tx->response_cb = audio_data_release_cb;

	/* Send audio data to modules output message queue. */
	ret = data_fifo_block_lock(handle->thread.msg_tx, (void **)&data_msg_tx,
				   sizeof(struct audio_module_message));
	if (ret) {
		LOG_ERR("Failed to send audio data to output of module %s, ret %d", handle->name,
			ret);

		data_fifo_block_free(handle->thread.msg_tx, (void **)&data_msg_tx);

		ret = k_sem_take(&handle->sem, K_NO_WAIT);
		if (ret) {
			LOG_ERR("Failed to take semaphore for TX FIFO put");
		}

		return ret;
	}

	return 0;
}

/**
 * @brief Send the audio data item to all connected modules.
 *
 * @param handle      [in/out]  The handle for this modules instance.
 * @param audio_data  [in]      A pointer to the audio data.
 *
 * @return 0 if successful, error otherwise.
 */
static int send_to_connected_modules(struct audio_module_handle *handle,
				     struct audio_data const *const audio_data)
{
	int ret;
	struct audio_module_handle *handle_to;

	if (handle->dest_count == 0) {
		LOG_WRN("Nowhere to send the audio data from module %s so releasing it",
			handle->name);

		k_mem_slab_free(handle->thread.data_slab, (void **)audio_data->data);

		return 0;
	}

	/* We need to ensure that all receiving modules have got the audio data.
	 * This is so the first receiver cannot free the audio data before all receivers
	 * have all gotten the audio data.
	 */
	ret = k_mutex_lock(&handle->dest_mutex, LOCK_TIMEOUT_US);
	if (ret) {
		LOG_ERR("Failed to take MUTEX lock in time");
		return ret;
	}

	/* Here the semaphore is used as a count of the number of audio data items out in the
	 * connected modules.
	 */
	ret = k_sem_init(&handle->sem, handle->dest_count, handle->dest_count);
	if (ret) {
		LOG_ERR("Failed to initiate semaphore");
		return ret;
	}

	/* Send to all internally connected modules. */
	SYS_SLIST_FOR_EACH_CONTAINER(&handle->handle_dest_list, handle_to, node) {
		ret = data_tx(handle, handle_to, audio_data, &audio_data_release_cb);
		if (ret) {
			LOG_ERR("Failed to send audio data to module %s from %s, ret %d",
				handle_to->name, handle->name, ret);

			return ret;
		}
	}

	ret = k_mutex_unlock(&handle->dest_mutex);
	if (ret) {
		LOG_ERR("Failed to release MUTEX");
		return ret;
	}

	/* Send to this module's TX FIFO for extraction by an external
	 * process with audio_module_rx().
	 */
	if (handle->use_tx_queue && handle->thread.msg_tx) {
		ret = tx_fifo_put(handle, audio_data);
		if (ret) {
			LOG_ERR("Failed to send audio data on module %s TX message queue",
				handle->name);

			ret = k_sem_take(&handle->sem, K_NO_WAIT);
			if (ret) {
				LOG_ERR("Failed to take semaphore");
			}

			return ret;
		}
	}

	return 0;
}

/**
 * @brief The thread that receives data from outside (e.g. the system and passes it into the audio
 *        system.
 *
 * @note An input module obtains data internally within the module (e.g. I2S in) and hence has no RX
 *       FIFO.
 *
 * @param handle  [in/out]  The handle for this modules instance.
 *
 * @return 0 if successful, error otherwise.
 */
static void module_thread_input(struct audio_module_handle *handle, void *p2, void *p3)
{
	int ret;
	struct audio_data audio_data;
	void *data;

	__ASSERT(handle != NULL, "Module task has NULL handle");
	__ASSERT(handle->description->functions->data_process != NULL,
		 "Module task has NULL process function pointer");

	/* Execute thread */
	while (1) {
		data = NULL;

		/* Get a new output buffer.
		 * Since this input module generates data within itself, the module itself
		 * will control the data flow.
		 */
		ret = k_mem_slab_alloc(handle->thread.data_slab, (void **)&data, K_NO_WAIT);
		__ASSERT(ret, "No free data for module %s, ret %d", handle->name, ret);

		/* Configure new audio data. */
		audio_data.data = data;
		audio_data.data_size = handle->thread.data_size;

		/* Process the input audio data */
		ret = handle->description->functions->data_process(
			(struct audio_module_handle_private *)handle, NULL, &audio_data);
		if (ret) {
			k_mem_slab_free(handle->thread.data_slab, (void **)(&data));

			LOG_ERR("Data process error in module %s, ret %d", handle->name, ret);
			continue;
		}

		LOG_DBG("Module %s received new audio data ", handle->name);

		/* Send input audio data to next module(s). */
		send_to_connected_modules(handle, &audio_data);
	}

	CODE_UNREACHABLE;
}

/**
 * @brief The thread that processes inputs and outputs them out of the audio system.
 *
 * @note An output module takes audio data from an input or in/out module.
 *       It then outputs data internally within the module (e.g. I2S out) and hence has no
 *       TX FIFO.
 *
 * @param handle  [in/out]  The handle for this modules instance.
 *
 * @return 0 if successful, error otherwise.
 */
static void module_thread_output(struct audio_module_handle *handle, void *p2, void *p3)
{
	int ret;

	struct audio_module_message *msg_rx;
	size_t size;

	__ASSERT(handle != NULL, "Module task has NULL handle");
	__ASSERT(handle->description->functions->data_process != NULL,
		 "Module task has NULL process function pointer");

	/* Execute thread. */
	while (1) {
		msg_rx = NULL;

		LOG_DBG("Module %s waiting for audio data", handle->name);

		/* Get a new input message.
		 * Since this input message is queued outside the module, this will then control the
		 * data flow.
		 */
		ret = data_fifo_pointer_last_filled_get(handle->thread.msg_rx, (void **)&msg_rx,
							&size, K_FOREVER);
		__ASSERT(ret, "Module %s error in getting last filled", handle->name);

		LOG_DBG("Module %s new audio data received", handle->name);

		/* Process the input audio data and output from the audio system. */
		ret = handle->description->functions->data_process(
			(struct audio_module_handle_private *)handle, &msg_rx->audio_data, NULL);
		if (ret) {
			if (msg_rx->response_cb != NULL) {
				msg_rx->response_cb(
					(struct audio_module_handle_private *)msg_rx->tx_handle,
					&msg_rx->audio_data);
			}

			LOG_ERR("Data process error in module %s, ret %d", handle->name, ret);
			continue;
		}

		if (msg_rx->response_cb != NULL) {
			msg_rx->response_cb((struct audio_module_handle_private *)msg_rx->tx_handle,
					    &msg_rx->audio_data);
		}

		data_fifo_block_free(handle->thread.msg_rx, (void **)&msg_rx);
	}

	CODE_UNREACHABLE;
}

/**
 * @brief The thread that processes inputs and outputs the data from the module.
 *
 * @note An processing module takes input and outputs from/to another
 *       module, thus having RX and TX FIFOs.
 *
 * @param handle  [in/out]  The handle for this modules instance.
 *
 * @return 0 if successful, error otherwise.
 */
static void module_thread_in_out(struct audio_module_handle *handle, void *p2, void *p3)
{
	int ret;
	struct audio_module_message *msg_rx;
	struct audio_data audio_data;
	void *data;
	size_t size;

	__ASSERT(handle != NULL, "Module task has NULL handle");
	__ASSERT(handle->description->functions->data_process != NULL,
		 "Module task has NULL process function pointer");

	/* Execute thread. */
	while (1) {
		data = NULL;

		LOG_DBG("Module %s is waiting for audio data", handle->name);

		/* Get a new input message.
		 * Since this input message is queued outside the module, this will then control the
		 * data flow.
		 */
		ret = data_fifo_pointer_last_filled_get(handle->thread.msg_rx, (void **)&msg_rx,
							&size, K_FOREVER);
		__ASSERT(ret, "Module %s error in getting last filled", handle->name);

		LOG_DBG("Module %s new audio data received", handle->name);

		/* Get a new output buffer. */
		ret = k_mem_slab_alloc(handle->thread.data_slab, (void **)&data, K_NO_WAIT);
		__ASSERT(ret, "No free data buffer for module %s, dropping input, ret %d",
			 handle->name, ret);

		/* Configure new audio audio_data. */
		audio_data.data = data;
		audio_data.data_size = handle->thread.data_size;

		/* Process the input audio data into the output audio data. */
		ret = handle->description->functions->data_process(
			(struct audio_module_handle_private *)handle, &msg_rx->audio_data,
			&audio_data);
		if (ret) {
			if (msg_rx->response_cb != NULL) {
				msg_rx->response_cb(
					(struct audio_module_handle_private *)(msg_rx->tx_handle),
					&msg_rx->audio_data);
			}

			data_fifo_block_free(handle->thread.msg_rx, (void **)(&msg_rx));

			k_mem_slab_free(handle->thread.data_slab, (void **)(&data));

			LOG_ERR("Data process error in module %s, ret %d", handle->name, ret);
			continue;
		}

		/* Send processed audio data to next module(s). */
		send_to_connected_modules(handle, &audio_data);

		if (msg_rx->response_cb != NULL) {
			msg_rx->response_cb((struct audio_module_handle_private *)msg_rx->tx_handle,
					    &msg_rx->audio_data);
		}

		data_fifo_block_free(handle->thread.msg_rx, (void **)&msg_rx);
	}

	CODE_UNREACHABLE;
}

int audio_module_open(struct audio_module_parameters const *const parameters,
		      struct audio_module_configuration const *const configuration,
		      char const *const name, struct audio_module_context *context,
		      struct audio_module_handle *handle)
{
	int ret;
	k_thread_entry_t thread_entry;

	if (parameters == NULL || configuration == NULL || name == NULL || handle == NULL ||
	    context == NULL) {
		LOG_ERR("Parameter is NULL for module open function");
		return -EINVAL;
	}

	if (state_not_undefined(handle->state)) {
		LOG_ERR("The module is already open");
		return -ECANCELED;
	}

	if (!validate_parameters(parameters)) {
		LOG_ERR("Invalid parameters for module");
		return -ECANCELED;
	}

	/* Clear handle to known state. */
	memset(handle, 0, sizeof(struct audio_module_handle));

	/* Allocate the context memory. */
	handle->context = context;

	memcpy(handle->name, name, CONFIG_AUDIO_MODULE_NAME_SIZE);
	if (strlen(name) > CONFIG_AUDIO_MODULE_NAME_SIZE) {
		handle->name[CONFIG_AUDIO_MODULE_NAME_SIZE] = '\0';
		LOG_WRN("Module's instance name truncated to %s", handle->name);
	}

	handle->description = parameters->description;
	memcpy(&handle->thread, &parameters->thread,
	       sizeof(struct audio_module_thread_configuration));

	if (handle->description->functions->open != NULL) {
		ret = handle->description->functions->open(
			(struct audio_module_handle_private *)handle, configuration);
		if (ret) {
			LOG_ERR("Failed open call to module %s, ret %d", name, ret);
			return ret;
		}
	}

	ret = handle->description->functions->configuration_set(
		(struct audio_module_handle_private *)handle, configuration);
	if (ret) {
		LOG_ERR("Set configuration for module %s failed, ret %d", handle->name, ret);
		return ret;
	}

	switch (handle->description->type) {
	case AUDIO_MODULE_TYPE_INPUT:
		thread_entry = (k_thread_entry_t)module_thread_input;
		break;

	case AUDIO_MODULE_TYPE_OUTPUT:
		thread_entry = (k_thread_entry_t)module_thread_output;
		break;

	case AUDIO_MODULE_TYPE_IN_OUT:
		thread_entry = (k_thread_entry_t)module_thread_in_out;
		break;

	default:
		LOG_ERR("Invalid module type %d for module %s", handle->description->type,
			handle->name);
		return -EINVAL;
	}

	sys_slist_init(&handle->handle_dest_list);
	k_mutex_init(&handle->dest_mutex);

	handle->thread_id = k_thread_create(
		&handle->thread_data, handle->thread.stack, handle->thread.stack_size, thread_entry,
		(void *)handle, NULL, NULL, K_PRIO_PREEMPT(handle->thread.priority), 0, K_FOREVER);

	ret = k_thread_name_set(handle->thread_id, &handle->name[0]);
	if (ret) {
		LOG_ERR("Failed to start thread for module %s thread, ret %d", handle->name, ret);

		/* Clean up the handle. */
		memset(handle, 0, sizeof(struct audio_module_handle));
		return ret;
	}

	handle->state = AUDIO_MODULE_STATE_CONFIGURED;

	k_thread_start(handle->thread_id);

	LOG_DBG("Thread started");

	return 0;
}

int audio_module_close(struct audio_module_handle *handle)
{
	int ret;

	if (handle == NULL) {
		LOG_ERR("Module handle is NULL");
		return -EINVAL;
	}

	if (!state_not_undefined(handle->state) || state_running(handle->state)) {
		LOG_ERR("Module %s in an invalid state, %d, for close", handle->name,
			handle->state);
		return -ECANCELED;
	}

	if (handle->description->functions->close != NULL) {
		ret = handle->description->functions->close(
			(struct audio_module_handle_private *)handle);
		if (ret) {
			LOG_ERR("Failed close call to module %s, returned %d", handle->name, ret);
			return ret;
		}
	}

	if (handle->thread.msg_rx != NULL) {
		data_fifo_empty(handle->thread.msg_rx);
	}

	if (handle->thread.msg_tx != NULL) {
		data_fifo_empty(handle->thread.msg_tx);
	}

	/*
	 * TODO: How to return all the data to the slab items?
	 *       Test the semaphore and wait for it to be zero.
	 */

	k_thread_abort(handle->thread_id);

	LOG_DBG("Closed module %s", handle->name);

	return 0;
};

int audio_module_reconfigure(struct audio_module_handle *handle,
			     struct audio_module_configuration const *const configuration)
{
	int ret;

	if (handle == NULL || configuration == NULL) {
		LOG_ERR("Module input parameter error");
		return -EINVAL;
	}

	if (handle->description->functions->configuration_set == NULL) {
		LOG_ERR("No mandatory reconfiguration function for module %s", handle->name);
		return -ECANCELED;
	}

	if (!state_not_undefined(handle->state) || state_running(handle->state)) {
		LOG_ERR("Module %s in an invalid state, %d, for setting the configuration",
			handle->name, handle->state);
		return -ECANCELED;
	}

	ret = handle->description->functions->configuration_set(
		(struct audio_module_handle_private *)handle, configuration);
	if (ret) {
		LOG_ERR("Reconfiguration for module %s failed, ret %d", handle->name, ret);
		return ret;
	}

	handle->state = AUDIO_MODULE_STATE_CONFIGURED;

	return 0;
};

int audio_module_configuration_get(struct audio_module_handle const *const handle,
				   struct audio_module_configuration *configuration)
{
	int ret;

	if (handle == NULL || configuration == NULL) {
		LOG_ERR("Module input parameter error");
		return -EINVAL;
	}

	if (handle->description->functions->configuration_get == NULL) {
		LOG_ERR("No mandatory get configuration function for module %s", handle->name);
		return -ECANCELED;
	}

	if (!state_not_undefined(handle->state)) {
		LOG_ERR("Module %s in an invalid state, %d, for getting the configuration",
			handle->name, handle->state);
		return -ECANCELED;
	}

	ret = handle->description->functions->configuration_get(
		(struct audio_module_handle_private *)handle, configuration);
	if (ret) {
		LOG_WRN("Get configuration for module %s failed, ret %d", handle->name, ret);
		return ret;
	}

	return 0;
};

int audio_module_connect(struct audio_module_handle *handle_from,
			 struct audio_module_handle *handle_to, bool connect_external)
{
	int ret;
	struct audio_module_handle *handle;

	if (handle_from == handle_to) {
		LOG_ERR("Module handles identical");
		return -EINVAL;
	}

	if (handle_from == NULL) {
		LOG_ERR("Invalid parameter for the connection function");
		return -EINVAL;
	}

	if (!has_input_type(handle_from->description->type)) {
		LOG_ERR("Connections between these modules is not supported");
		return -ECANCELED;
	}

	if (!state_not_undefined(handle_from->state)) {
		LOG_WRN("A module is in an invalid state for connecting");
		return -ECANCELED;
	}

	if (connect_external) {
		if (handle_to != NULL || handle_from->thread.msg_tx == NULL) {
			LOG_ERR("Module %s has no TX FIFO or module handle to is not NULL",
				handle_from->name);
			return -EINVAL;
		}
	} else {
		if (handle_to == NULL) {
			LOG_ERR("Invalid parameter for the connection function");
			return -EINVAL;
		}

		if (!has_output_type(handle_to->description->type)) {
			LOG_ERR("Connections between these modules is not supported");
			return -ECANCELED;
		}

		if (!state_not_undefined(handle_to->state)) {
			LOG_WRN("A module is in an invalid state for connecting");
			return -ECANCELED;
		}
	}

	ret = k_mutex_lock(&handle_from->dest_mutex, LOCK_TIMEOUT_US);
	if (ret) {
		LOG_ERR("Failed to take MUTEX lock in time");
		return ret;
	}

	/* If the connect_external is true the handle_from module will queue it's output
	 * data to it's own TX FIFO. Thus allowing an external system to receive the data
	 * with a call to audio_module_data_rx() with the same handle.
	 */
	if (connect_external) {
		handle_from->use_tx_queue = true;

		LOG_DBG("Return the output of %s on it's TX FIFO", handle_from->name);
	} else {
		SYS_SLIST_FOR_EACH_CONTAINER(&handle_from->handle_dest_list, handle, node) {
			if (handle_to == handle) {
				LOG_WRN("Already attached %s to %s", handle_to->name,
					handle_from->name);
				return -EALREADY;
			}
		}

		sys_slist_append(&handle_from->handle_dest_list, &handle_to->node);

		LOG_DBG("Connected the output of %s to the input of %s", handle_from->name,
			handle_to->name);
	}

	handle_from->dest_count++;

	ret = k_mutex_unlock(&handle_from->dest_mutex);
	if (ret) {
		LOG_ERR("Failed to release MUTEX lock");
		return ret;
	}

	return 0;
}

int audio_module_disconnect(struct audio_module_handle *handle,
			    struct audio_module_handle *handle_disconnect, bool disconnect_external)
{
	int ret;

	if (handle == handle_disconnect) {
		LOG_ERR("Module handles identical");
		return -EINVAL;
	}

	if (handle == NULL) {
		LOG_WRN("Module handle is NULL");
		return -EINVAL;
	}

	if (!has_input_type(handle->description->type)) {
		LOG_WRN("Disconnection of modules %s from %s, is not supported",
			handle_disconnect->name, handle->name);
		return -ECANCELED;
	}

	if (!state_not_undefined(handle->state)) {
		LOG_WRN("A module is in an invalid state for connecting");
		return -ECANCELED;
	}

	if (disconnect_external) {
		if (handle_disconnect != NULL) {
			LOG_ERR("Module disconnect handle is not NULL");
			return -EINVAL;
		}
	} else {
		if (handle_disconnect == NULL) {
			LOG_ERR("Invalid parameter for the connection function");
			return -EINVAL;
		}

		if (!has_output_type(handle_disconnect->description->type)) {
			LOG_WRN("Disconnection of modules %s from %s, is not supported",
				handle_disconnect->name, handle->name);
			return -ECANCELED;
		}

		if (!state_not_undefined(handle_disconnect->state)) {
			LOG_WRN("A module is in an invalid state for connecting");
			return -ECANCELED;
		}
	}

	ret = k_mutex_lock(&handle->dest_mutex, LOCK_TIMEOUT_US);
	if (ret) {
		LOG_ERR("Failed to take MUTEX lock in time");
		return ret;
	}

	/* If the handle module is queuing it's output audio data to it's own TX FIFO and
	 * the disconnect_external flag is true, then stop sending the audio data items to
	 * it.
	 */
	if (disconnect_external) {
		handle->use_tx_queue = false;

		LOG_DBG("Stop returning the output of %s on it's TX FIFO", handle->name);
	} else {
		if (!sys_slist_find_and_remove(&handle->handle_dest_list,
					       &handle_disconnect->node)) {
			LOG_ERR("Connection to module %s has not been found for module %s",
				handle_disconnect->name, handle->name);
			return -EALREADY;
		}

		LOG_DBG("Disconnect module %s from module %s", handle_disconnect->name,
			handle->name);
	}

	handle->dest_count--;

	ret = k_mutex_unlock(&handle->dest_mutex);
	if (ret) {
		LOG_ERR("Failed to release MUTEX lock");
		return ret;
	}

	return 0;
}

int audio_module_start(struct audio_module_handle *handle)
{
	int ret;

	if (handle == NULL) {
		LOG_ERR("Module handle is NULL");
		return -EINVAL;
	}

	if (!state_not_undefined(handle->state) || !type_not_undefined(handle->description->type)) {
		LOG_ERR("Module %s in an invalid state (%d) or type (%d) to start", handle->name,
			handle->state, handle->description->type);
		return -ECANCELED;
	}

	if (state_running(handle->state)) {
		LOG_WRN("Module %s already running", handle->name);
		return -EALREADY;
	}

	if (handle->description->functions->start != NULL) {
		ret = handle->description->functions->start(
			(struct audio_module_handle_private *)handle);
		if (ret) {
			LOG_ERR("Failed user start for module %s, ret %d", handle->name, ret);
			return ret;
		}
	}

	handle->state = AUDIO_MODULE_STATE_RUNNING;

	return 0;
}

int audio_module_stop(struct audio_module_handle *handle)
{
	int ret;

	if (handle == NULL) {
		LOG_ERR("Module handle is NULL");
		return -EINVAL;
	}

	if (!state_not_undefined(handle->state) || !type_not_undefined(handle->description->type)) {
		LOG_ERR("Module %s in an invalid state (%d) or type (%d) to stop", handle->name,
			handle->state, handle->description->type);
		return -ECANCELED;
	}

	if (!state_running(handle->state)) {
		LOG_WRN("Module %s is not running already stopped", handle->name);
		return -EALREADY;
	}

	if (handle->description->functions->stop != NULL) {
		ret = handle->description->functions->stop(
			(struct audio_module_handle_private *)handle);
		if (ret) {
			LOG_ERR("Failed user pause for module %s, ret %d", handle->name, ret);
			return ret;
		}
	}

	handle->state = AUDIO_MODULE_STATE_STOPPED;

	return 0;
}

int audio_module_data_tx(struct audio_module_handle *handle,
			 struct audio_data const *const audio_data,
			 audio_module_response_cb response_cb)
{
	if (handle == NULL) {
		LOG_ERR("Module handle is NULL");
		return -EINVAL;
	}

	if (!state_not_undefined(handle->state) || !has_output_type(handle->description->type)) {
		LOG_ERR("Module %s in an invalid state (%d) or type (%d) to transmit data",
			handle->name, handle->state, handle->description->type);
		return -ECANCELED;
	}

	if (!state_running(handle->state)) {
		LOG_WRN("Module %s is not running", handle->name);
		return -ECANCELED;
	}

	if (handle->thread.msg_rx == NULL) {
		LOG_ERR("Module %s has message queue set to NULL", handle->name);
		return -ECANCELED;
	}

	if (audio_data == NULL || audio_data->data == NULL || audio_data->data_size == 0) {
		LOG_ERR("Data parameter error for module %s", handle->name);
		return -EINVAL;
	}

	return data_tx((void *)NULL, handle, audio_data, response_cb);
}

int audio_module_data_rx(struct audio_module_handle *handle, struct audio_data *audio_data,
			 k_timeout_t timeout)
{
	int ret;

	struct audio_module_message *msg_tx;
	size_t msg_tx_size;

	if (handle == NULL) {
		LOG_ERR("Module handle is NULL");
		return -EINVAL;
	}

	if (!state_not_undefined(handle->state) || !has_input_type(handle->description->type)) {
		LOG_ERR("Module %s in an invalid state (%d) or type (%d) to receive data",
			handle->name, handle->state, handle->description->type);
		return -ECANCELED;
	}

	if (!state_running(handle->state)) {
		LOG_WRN("Module %s is not running", handle->name);
		return -ECANCELED;
	}

	if (handle->thread.msg_tx == NULL) {
		LOG_ERR("Module has message queue set to NULL");
		return -ECANCELED;
	}

	if (audio_data == NULL || audio_data->data == NULL || audio_data->data_size == 0) {
		LOG_ERR("Input audio data for module %s has NULL pointer or a zero size buffer",
			handle->name);
		return -EINVAL;
	}

	ret = data_fifo_pointer_last_filled_get(handle->thread.msg_tx, (void **)&msg_tx,
						&msg_tx_size, timeout);
	if (ret) {
		LOG_ERR("Failed to retrieve data from module %s, ret %d", handle->name, ret);
		return ret;
	}

	if (msg_tx->audio_data.data == NULL ||
	    msg_tx->audio_data.data_size > audio_data->data_size) {
		LOG_ERR("Data output pointer NULL or not enough room for buffer from module %s",
			handle->name);
		ret = -EINVAL;
	} else {
		memcpy(&audio_data->meta, &msg_tx->audio_data.meta, sizeof(struct audio_metadata));
		memcpy((uint8_t *)audio_data->data, (uint8_t *)msg_tx->audio_data.data,
		       msg_tx->audio_data.data_size);
	}

	data_fifo_block_free(handle->thread.msg_tx, (void **)&msg_tx);

	return ret;
}

int audio_module_data_tx_rx(struct audio_module_handle *handle_tx,
			    struct audio_module_handle *handle_rx,
			    struct audio_data const *const audio_data_tx,
			    struct audio_data *audio_data_rx, k_timeout_t timeout)
{
	int ret;
	struct audio_module_message *msg_rx;
	size_t msg_rx_size;

	if (handle_tx == NULL || handle_rx == NULL) {
		LOG_ERR("Module handle is NULL");
		return -EINVAL;
	}

	if (!state_not_undefined(handle_tx->state) || !state_not_undefined(handle_rx->state)) {
		LOG_WRN("One or both of the modules are in an invalid state");
		return -ECANCELED;
	}

	if (!state_running(handle_tx->state) || !state_running(handle_rx->state)) {
		LOG_WRN("One or both of the modules are not running");
		return -ECANCELED;
	}

	if (!has_output_type(handle_tx->description->type) ||
	    !has_input_type(handle_rx->description->type)) {
		LOG_ERR("Modules are not of the right type for operation");
		return -ECANCELED;
	}

	if (audio_data_tx == NULL || audio_data_rx == NULL) {
		LOG_ERR("One or both of the audio data pointers are NULL");
		return -EINVAL;
	}

	if (handle_tx->thread.msg_rx == NULL || handle_rx->thread.msg_tx == NULL) {
		LOG_ERR("Modules have message queue set to NULL");
		return -EINVAL;
	}

	if (audio_data_tx->data == NULL || audio_data_tx->data_size == 0 ||
	    audio_data_rx->data == NULL || audio_data_rx->data_size == 0) {
		LOG_ERR("One or both of the have invalid output audio data");
		return -EINVAL;
	}

	ret = data_tx(NULL, handle_tx, audio_data_tx, NULL);
	if (ret) {
		LOG_ERR("Failed to send audio data to module %s, ret %d", handle_tx->name, ret);
		return ret;
	}

	LOG_DBG("Wait for message on module %s TX queue", handle_rx->name);

	ret = data_fifo_pointer_last_filled_get(handle_rx->thread.msg_rx, (void **)&msg_rx,
						&msg_rx_size, timeout);
	if (ret) {
		LOG_ERR("Failed to retrieve audio data from module %s, ret %d", handle_rx->name,
			ret);
		return ret;
	}

	if (msg_rx->audio_data.data == NULL || msg_rx->audio_data.data_size == 0) {
		LOG_ERR("Data output buffer too small for received buffer from module %s "
			"(%d)",
			handle_rx->name, msg_rx->audio_data.data_size);
		ret = -EINVAL;
	} else {
		memcpy(&audio_data_rx->meta, &msg_rx->audio_data.meta,
		       sizeof(struct audio_metadata));
		memcpy((uint8_t *)audio_data_rx->data, (uint8_t *)msg_rx->audio_data.data,
		       msg_rx->audio_data.data_size);
	}

	data_fifo_block_free(handle_rx->thread.msg_rx, (void **)&msg_rx);

	return ret;
};

int audio_module_names_get(struct audio_module_handle const *const handle, char **base_name,
			   char *instance_name)
{
	if (handle == NULL || base_name == NULL || instance_name == NULL) {
		LOG_ERR("Input parameter is NULL");
		return -EINVAL;
	}

	if (!state_not_undefined(handle->state)) {
		LOG_WRN("Module %s is in an invalid state, %d, for get names", handle->name,
			handle->state);
		return -ECANCELED;
	}

	*base_name = handle->description->name;
	strcpy(instance_name, &handle->name[0]);

	return 0;
}

int audio_module_state_get(struct audio_module_handle const *const handle,
			   enum audio_module_state *state)
{
	if (handle == NULL || state == NULL) {
		LOG_ERR("Input parameter is NULL");
		return -EINVAL;
	}

	if (!state_not_undefined(handle->state)) {
		LOG_WRN("Module state is invalid");
		return -ECANCELED;
	}

	*state = handle->state;

	return 0;
};

int audio_module_number_channels_calculate(uint32_t locations, int8_t *number_channels)
{
	if (number_channels == NULL) {
		LOG_ERR("Invalid parameters");
		return -EINVAL;
	}

	*number_channels = 0;
	for (int i = 0; i < AUDIO_MODULE_LOCATIONS_NUM; i++) {
		*number_channels += locations & 1;
		locations >>= 1;
	}

	LOG_DBG("Found %d channel(s)", *number_channels);

	return 0;
}
