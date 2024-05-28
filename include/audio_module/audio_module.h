/*
 * Copyright(c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef _AUDIO_MODULE_H_
#define _AUDIO_MODULE_H_

/**
 * @file
 * @defgroup audio_module Audio module
 * @{
 * @brief Audio module for LE Audio applications.
 */

#ifdef __cplusplus
extern "C" {
#endif

#include <zephyr/kernel.h>
#include <data_fifo.h>

#include "audio_defines.h"

/**
 * @brief Number of valid location bits.
 */
#define AUDIO_MODULE_LOCATIONS_NUM (32)

/**
 * @brief Module type.
 */
enum audio_module_type {
	/* The module type is undefined. */
	AUDIO_MODULE_TYPE_UNDEFINED = 0,

	/* This is an input processing module.
	 *
	 * @note An input module obtains data internally within the
	 *       module (e.g. I2S in) and hence has no RX FIFO.
	 */
	AUDIO_MODULE_TYPE_INPUT,

	/* This is an output processing module.
	 *
	 * @note An output module takes audio data from an input or an in/out module.
	 *       It then outputs data internally within the module (e.g. I2S out) and hence has no
	 *       TX FIFO.
	 */
	AUDIO_MODULE_TYPE_OUTPUT,

	/* This is a processing module.
	 *
	 * @note A processing module takes input from and outputs to another
	 *       module, thus having RX and TX FIFOs.
	 */
	AUDIO_MODULE_TYPE_IN_OUT
};

/**
 * @brief Module state.
 */
enum audio_module_state {
	/* Module state undefined. */
	AUDIO_MODULE_STATE_UNDEFINED = 0,

	/* Module is in the configured state. */
	AUDIO_MODULE_STATE_CONFIGURED,

	/* Module is in the running state. */
	AUDIO_MODULE_STATE_RUNNING,

	/* Module is in the stopped state. */
	AUDIO_MODULE_STATE_STOPPED
};

/**
 * @brief Module's private handle.
 */
struct audio_module_handle_private;

/**
 * @brief Private context for the module.
 */
struct audio_module_context;

/**
 * @brief Module's private configuration.
 */
struct audio_module_configuration;

/**
 * @brief Callback function for a response to a data_send as
 *        supplied by the module user.
 *
 * @param handle      [in/out]  The handle of the module that sent the audio data.
 * @param audio_data  [in]      The audio data to operate on.
 */
typedef void (*audio_module_response_cb)(struct audio_module_handle_private *handle,
					 struct audio_data const *const audio_data);

/**
 * @brief Private pointer to a module's functions.
 */
struct audio_module_functions {
	/**
	 * @brief Open an audio module with the specified initial configuration.
	 *
	 * @note This is an optional function for an audio module.
	 *
	 * @param handle         [in/out]  The handle to the module instance.
	 * @param configuration  [in]      Pointer to the desired initial configuration to set.
	 *
	 * @return 0 if successful, error otherwise.
	 */
	int (*open)(struct audio_module_handle_private *handle,
		    struct audio_module_configuration const *const configuration);

	/**
	 * @brief Close an open audio module.
	 *
	 * @note This is an optional function for an audio module.
	 *
	 * @param handle  [in/out]  The handle to the module instance.
	 *
	 * @return 0 if successful, error otherwise.
	 */
	int (*close)(struct audio_module_handle_private *handle);

	/**
	 * @brief Reconfigure an audio module after it has been opened with its initial
	 *        configuration.
	 *
	 * @note This is a mandatory function for an audio module.
	 *
	 * @param handle         [in/out]  The handle to the module instance.
	 * @param configuration  [in]      Pointer to the desired configuration to set.
	 *
	 * @return 0 if successful, error otherwise.
	 */
	int (*configuration_set)(struct audio_module_handle_private *handle,
				 struct audio_module_configuration const *const configuration);

	/**
	 * @brief Get the configuration of an audio module.
	 *
	 * @note This is a mandatory function for an audio module.
	 *
	 * @param handle         [in]   The handle to the module instance.
	 * @param configuration  [out]  Pointer to the module's current configuration.
	 *
	 * @return 0 if successful, error otherwise.
	 */
	int (*configuration_get)(struct audio_module_handle_private const *const handle,
				 struct audio_module_configuration *configuration);

	/**
	 * @brief Start an audio module.
	 *
	 * @note This is an optional function for an audio module.
	 *
	 * @param handle  [in/out]  The handle for the module to start.
	 *
	 * @return 0 if successful, error otherwise.
	 */
	int (*start)(struct audio_module_handle_private *handle);

	/**
	 * @brief Stop an audio module.
	 *
	 * @note This is an optional function for an audio module.
	 *
	 * @param handle  [in/out]  The handle for the module to stop.
	 *
	 * @return 0 if successful, error otherwise.
	 */
	int (*stop)(struct audio_module_handle_private *handle);

	/**
	 * @brief The core data processing for an audio module. Can be either an
	 *        input, output or input/output module type.
	 *
	 * @note This is a mandatory function for an audio module.
	 *
	 * @param handle         [in/out]  The handle to the module instance.
	 * @param audio_data_rx  [in]      Pointer to the input audio data or NULL for an input
	 *                                 module.
	 * @param audio_data_tx  [out]     Pointer to the output audio data or NULL for an output
	 *                                 module.
	 *
	 * @return 0 if successful, error otherwise.
	 */
	int (*data_process)(struct audio_module_handle_private *handle,
			    struct audio_data const *const audio_data_rx,
			    struct audio_data *audio_data_tx);
};

/**
 * @brief A module's minimum description.
 */
struct audio_module_description {
	/* The module base name. */
	char *name;

	/* The module type. */
	enum audio_module_type type;

	/* A pointer to the functions in the module. */
	const struct audio_module_functions *functions;
};

/**
 * @brief Module's thread configuration structure.
 */
struct audio_module_thread_configuration {
	/* Thread stack. */
	k_thread_stack_t *stack;

	/* Thread stack size. */
	size_t stack_size;

	/* Thread priority. */
	int priority;

	/* A pointer to a module's audio data receiver FIFO, can be NULL. */
	struct data_fifo *msg_rx;

	/* A pointer to a module's audio data transmitter FIFO, can be NULL. */
	struct data_fifo *msg_tx;

	/* A pointer to the audio data buffer slab, can be NULL. */
	struct k_mem_slab *data_slab;

	/* Size of each memory data buffer in bytes that will be
	 * taken from the audio data buffer slab. The size can be 0.
	 */
	size_t data_size;
};

/**
 * @brief Module's generic set-up structure.
 */
struct audio_module_parameters {
	/* The module's description. */
	struct audio_module_description *description;

	/* The module's thread setting. */
	struct audio_module_thread_configuration thread;
};

/**
 * @brief Private module handle.
 */
struct audio_module_handle {
	/* A NULL terminated string giving a unique name of this module's instance. */
	char name[CONFIG_AUDIO_MODULE_NAME_SIZE + 1];

	/* The module's description. */
	const struct audio_module_description *description;

	/* Current state of the module. */
	enum audio_module_state state;

	/* Thread ID. */
	k_tid_t thread_id;

	/* Thread data. */
	struct k_thread thread_data;

	/* Flag to indicate if the module should put its output audio data onto its TX FIFO. */
	bool use_tx_queue;

	/* List node (pointer to next audio module). */
	sys_snode_t node;

	/* A single linked-list of the destination handles the module is connected to. */
	sys_slist_t handle_dest_list;

	/* Number of destination modules. */
	uint8_t dest_count;

	/* Semaphore to count messages between modules and on a module's TX FIFO. */
	struct k_sem sem;

	/* Mutex to make the above destinations list thread safe. */
	struct k_mutex dest_mutex;

	/* Module's thread configuration. */
	struct audio_module_thread_configuration thread;

	/* Private context for the module. */
	struct audio_module_context *context;
};

/**
 * @brief Private structure describing a data_in message into the module thread.
 */
struct audio_module_message {
	/* Audio data to input. */
	struct audio_data audio_data;

	/* Sending module's handle. */
	struct audio_module_handle *tx_handle;

	/* Callback for when the audio data has been consumed. */
	audio_module_response_cb response_cb;
};

/**
 * @brief Open an audio module.
 *
 * @param parameters     [in]      Pointer to the module set-up parameters.
 * @param configuration  [in]      Pointer to the module's configuration.
 * @param name           [in]      A NULL terminated string giving a unique name for this module
 *                                 instance.
 * @param context        [in/out]  Pointer to the private context for the module.
 * @param handle         [out]     Pointer to the module's private handle.
 *
 * @return 0 if successful, error otherwise.
 */
int audio_module_open(struct audio_module_parameters const *const parameters,
		      struct audio_module_configuration const *const configuration,
		      char const *const name, struct audio_module_context *context,
		      struct audio_module_handle *handle);

/**
 * @brief Close an opened audio module.
 *
 * @param handle  [in/out]  The handle to the module instance.
 *
 * @return 0 if successful, error otherwise.
 */
int audio_module_close(struct audio_module_handle *handle);

/**
 * @brief Reconfigure an opened audio module.
 *
 * @param handle         [in/out]  The handle to the module instance.
 * @param configuration  [in]      Pointer to the module's configuration to set.
 *
 * @return 0 if successful, error otherwise.
 */
int audio_module_reconfigure(struct audio_module_handle *handle,
			     struct audio_module_configuration const *const configuration);

/**
 * @brief Get the configuration of an audio module.
 *
 * @param handle         [in]   The handle to the module instance.
 * @param configuration  [out]  Pointer to the module's current configuration.
 *
 * @return 0 if successful, error otherwise.
 */
int audio_module_configuration_get(struct audio_module_handle const *const handle,
				   struct audio_module_configuration *configuration);

/**
 * @brief Connect two audio modules together or connect to the module's TX FIFO.
 *
 * @note The function should be called once for every individual connection.
 *
 * @param handle_from       [in/out]  The handle for the module for output.
 * @param handle_to         [in/out]  The handle of the module for input, should be NULL if
 *                                    connect_external flag is true.
 * @param connect_external  [in]      Flag to indicate if the from module should put its output
 *                                    audio data items onto its TX FIFO for access via an external
 *                                    system.
 *
 * @return 0 if successful, error otherwise.
 */
int audio_module_connect(struct audio_module_handle *handle_from,
			 struct audio_module_handle *handle_to, bool connect_external);

/**
 * @brief Disconnect audio modules from each other or disconnect the module's TX FIFO. The function
 * should be called for all individual disconnections.
 *
 * @param handle               [in/out]  The handle for the module.
 * @param handle_disconnect    [in/out]  The handle of the module to disconnect, should be NULL if
 *                                       disconnect_external flag is true.
 * @param disconnect_external  [in]      Flag to indicate that the output audio data items should
 *                                       stop being put on handle TX FIFO.
 *
 * @return 0 if successful, error otherwise.
 */
int audio_module_disconnect(struct audio_module_handle *handle,
			    struct audio_module_handle *handle_disconnect,
			    bool disconnect_external);

/**
 * @brief Start processing audio data in the audio module given by handle.
 *
 * @param handle  [in/out]  The handle for the module to start.
 *
 * @return 0 if successful, error otherwise.
 */
int audio_module_start(struct audio_module_handle *handle);

/**
 * @brief Stop processing audio data in the audio module given by handle.
 *
 * @param handle  [in/out]  The handle for the module to be stopped.
 *
 * @return 0 if successful, error otherwise.
 */
int audio_module_stop(struct audio_module_handle *handle);

/**
 * @brief Send an audio data item to an audio module, all data is consumed by the module.
 *
 * @param handle       [in/out]  The handle for the receiving module instance.
 * @param audio_data   [in]      Pointer to the audio data to send to the module.
 * @param response_cb  [in]      Pointer to a callback to run when the buffer is
 *                               fully consumed in a module.
 *
 * @return 0 if successful, error otherwise.
 */
int audio_module_data_tx(struct audio_module_handle *handle,
			 struct audio_data const *const audio_data,
			 audio_module_response_cb response_cb);

/**
 * @brief Retrieve an audio data item from an audio module.
 *
 * @param handle      [in/out]  The handle to the module instance.
 * @param audio_data  [out]     Pointer to the audio data from the module.
 * @param timeout     [in]      Non-negative waiting period to wait for operation to complete
 *                              (in milliseconds). Use K_NO_WAIT to return without waiting,
 *                              or K_FOREVER to wait as long as necessary.
 *
 * @return 0 if successful, error otherwise.
 */
int audio_module_data_rx(struct audio_module_handle *handle, struct audio_data *audio_data,
			 k_timeout_t timeout);

/**
 * @brief Send an audio data to an audio module and retrieve an audio data from an audio module.
 *
 * @note The audio data is processed within the module or sequence of modules. The result is
 *       returned via the module or final module's output FIFO. All the input data is consumed
 *       within the call and thus the input data buffer maybe released once the function returns.
 *
 * @param handle_tx      [in/out]  The handle to the module to send the input audio data to.
 * @param handle_rx      [in/out]  The handle to the module to receive audio data from.
 * @param audio_data_tx  [in]      Pointer to the audio data to send.
 * @param audio_data_rx  [out]     Pointer to the audio data received.
 * @param timeout        [in]      Non-negative waiting period to wait for operation to complete
 *                                 (in milliseconds). Use K_NO_WAIT to return without waiting,
 *                                 or K_FOREVER to wait as long as necessary.
 *
 * @return 0 if successful, error otherwise.
 */
int audio_module_data_tx_rx(struct audio_module_handle *handle_tx,
			    struct audio_module_handle *handle_rx,
			    struct audio_data const *const audio_data_tx,
			    struct audio_data *audio_data_rx, k_timeout_t timeout);

/**
 * @brief Helper to get the base and instance names for a given audio
 *        module handle.
 *
 * @param handle         [in]   The handle to the module instance.
 * @param base_name      [out]  Pointer to the name of the module.
 * @param instance_name  [out]  Pointer to the name of the current module instance.
 *
 * @return 0 if successful, error otherwise.
 */
int audio_module_names_get(struct audio_module_handle const *const handle, char **base_name,
			   char *instance_name);

/**
 * @brief Helper to get the state of a given audio module handle.
 *
 * @param handle  [in]   The handle to the module instance.
 * @param state   [out]  Pointer to the current module's state.
 *
 * @return 0 if successful, error otherwise.
 */
int audio_module_state_get(struct audio_module_handle const *const handle,
			   enum audio_module_state *state);

/**
 * @brief Helper to calculate the number of channels from the channel map for the given
 *        audio data.
 *
 * @param locations        [in]   Channel locations to calculate the number of channels from.
 * @param number_channels  [out]  Pointer to the calculated number of channels in the audio data.
 *
 * @return 0 if successful, error otherwise.
 */
int audio_module_number_channels_calculate(uint32_t locations, int8_t *number_channels);

#ifdef __cplusplus
}
#endif

/**
 * @}
 */

#endif /*_AUDIO_MODULE_H_ */
