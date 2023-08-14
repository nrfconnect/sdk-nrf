/*
 * Copyright (c) 2019 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef SMS_H_
#define SMS_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <zephyr/types.h>
#include <sys/types.h>

/**
 * @file sms.h
 *
 * @defgroup sms SMS subscriber manager
 *
 * @{
 *
 * @brief Public APIs of the SMS subscriber manager module.
 */

/**
 * @brief SMS message type.
 */
enum sms_type {
	/** @brief SMS-DELIVER message type. */
	SMS_TYPE_DELIVER,
	/** @brief SMS-STATUS-REPORT message type. */
	SMS_TYPE_STATUS_REPORT
};

/**
 * @brief SMS data type.
 */
enum sms_data_type {
	/** @brief ASCII string with ISO-8859-15 extension. */
	SMS_DATA_TYPE_ASCII,
	/** @brief GSM 7 bit Default Alphabet as specified in 3GPP TS 23.038 Section 6.2. */
	SMS_DATA_TYPE_GSM7BIT,
};

/**
 * @brief Maximum length of SMS in number of characters
 * as specified in 3GPP TS 23.038 Section 4 and Section 6.1.2.1.1.
 */
#define SMS_MAX_PAYLOAD_LEN_CHARS 160

/**
 * @brief Maximum length of SMS address, i.e., phone number, in characters
 * as specified in 3GPP TS 23.040 Section 9.1.2.3.
 */
#define SMS_MAX_ADDRESS_LEN_CHARS 20

/**
 * @brief SMS time information specified in 3GPP TS 23.040 Section 9.2.3.11.
 */
struct sms_time {
	uint8_t year;    /**< @brief Year. Last two digits of the year.*/
	uint8_t month;   /**< @brief Month. */
	uint8_t day;     /**< @brief Day. */
	uint8_t hour;    /**< @brief Hour. */
	uint8_t minute;  /**< @brief Minute. */
	uint8_t second;  /**< @brief Second. */
	int8_t timezone; /**< @brief Timezone in quarters of an hour. */
};

/**
 * @brief SMS address, i.e., phone number.
 *
 * @details This may represent either originating or destination address and is
 * specified in 3GPP TS 23.040 Section 9.1.2.5.
 */
struct sms_address {
	/** @brief Address in NUL-terminated string format. */
	char address_str[SMS_MAX_ADDRESS_LEN_CHARS + 1];
	/** @brief Address length in number of characters. */
	uint8_t length;
	/** @brief Address type as specified in 3GPP TS 23.040 Section 9.1.2.5. */
	uint8_t type;
};

/**
 * @brief SMS concatenated short message information.
 *
 * @details This is specified in 3GPP TS 23.040 Section 9.2.3.24.1 and 9.2.3.24.8.
 */
struct sms_udh_concat {
	/** @brief Indicates whether this field is present in the SMS message. */
	bool present;
	/** @brief Concatenated short message reference number. */
	uint16_t ref_number;
	/** @brief Maximum number of short messages in the concatenated short message. */
	uint8_t total_msgs;
	/** @brief Sequence number of the current short message. */
	uint8_t seq_number;
};

/**
 * @brief SMS application port addressing information.
 *
 * @details This is specified in 3GPP TS 23.040 Section 9.2.3.24.3 and 9.2.3.24.4.
 */
struct sms_udh_app_port {
	/** @brief Indicates whether this field is present in the SMS message. */
	bool present;
	/** @brief Destination port. */
	uint16_t dest_port;
	/** @brief Source port. */
	uint16_t src_port;
};

/**
 * @brief SMS-DELIVER message header.
 *
 * @details This is for incoming SMS message and more specifically SMS-DELIVER
 * message specified in 3GPP TS 23.040.
 */
struct sms_deliver_header {
	/** @brief Timestamp. */
	struct sms_time time;
	/** @brief Originating address, i.e., phone number. */
	struct sms_address originating_address;
	/** @brief Application port addressing information. */
	struct sms_udh_app_port app_port;
	/** @brief Concatenated short message information. */
	struct sms_udh_concat concatenated;
};

/**
 * @brief SMS header.
 *
 * @details This can easily be extended to support additional message types.
 */
union sms_header {
	struct sms_deliver_header deliver;
};

/** @brief SMS PDU data. */
struct sms_data {
	/** @brief Received message type. */
	enum sms_type type;
	/** @brief SMS header. */
	union sms_header header;

	/** @brief Length of the payload buffer. */
	int payload_len;
	/**
	 * @brief SMS message payload.
	 *
	 * @details Reserving enough bytes for maximum number of characters
	 * but the length of the received payload is in payload_len variable.
	 *
	 * Generally the message is of text type in which case you can treat it as string.
	 * However, header may contain information that determines it for specific purpose,
	 * e.g., via application port information, in which case it should be treated as
	 * specified for that purpose.
	 */
	uint8_t payload[SMS_MAX_PAYLOAD_LEN_CHARS + 1];
};

/** @brief SMS listener callback function. */
typedef void (*sms_callback_t)(struct sms_data *const data, void *context);

/**
 * @brief Register a new listener to SMS library.
 *
 * @details Also registers to modem's SMS service if it's not already subscribed.
 *
 * A listener is identified by a unique handle value. This handle should be used
 * to unregister the listener. A listener can be registered multiple times with
 * the same or a different context.
 *
 * @param[in] listener Callback function. Cannot be null.
 * @param[in] context User context. Can be null if not used.
 *
 * @retval -EINVAL Invalid parameter.
 * @retval -ENOSPC List of observers is full.
 * @retval -EBUSY Indicates that one SMS client has already been registered towards the modem
 *                and SMS subscriber module is not able to do it.
 * @retval -ENOMEM Out of memory.
 * @return Handle identifying the listener,
 *         or a negative value if an error occurred.
 */
int sms_register_listener(sms_callback_t listener, void *context);

/**
 * @brief Unregister a listener.
 *
 * @details Also unregisters from modem's SMS service if there are
 * no listeners registered.
 *
 * @param[in] handle Handle identifying the listener to unregister.
 */
void sms_unregister_listener(int handle);

/**
 * @brief Send SMS message as ASCII string with ISO-8859-15 extension.
 *
 * @details Sending is done with GSM 7bit encoding used to encode textual SMS messages.
 * SMS-SUBMIT message is specified in 3GPP TS 23.040 Section 9.2.2.2 and data encoding
 * in 3GPP TS 23.038 Section 4 and Section 6.2.
 *
 * This function does the same as if @c sms_send would be called with SMS_DATA_TYPE_ASCII type.
 *
 * This function doesn't support sending of 8 bit binary data messages or UCS2 encoded text.
 *
 * @param[in] number Recipient number in international format.
 * @param[in] text Text to be sent as ASCII string with ISO-8859-15 extension.
 *
 * @retval -EINVAL Invalid parameter.
 * @return 0 on success, otherwise error code.
 *         A positive value on AT error with "ERROR", "+CME ERROR", and "+CMS ERROR" responses.
 *         The type can be resolved with @c nrf_modem_at_err_type and
 *         the error value with @c nrf_modem_at_err.
 */
int sms_send_text(const char *number, const char *text);

/**
 * @brief Send SMS message in given message type.
 *
 * @details The message is sent with GSM 7bit encoding used to encode textual SMS messages.
 * SMS-SUBMIT message is specified in 3GPP TS 23.040 Section 9.2.2.2 and data encoding
 * in 3GPP TS 23.038 Section 4 and Section 6.2.
 *
 * If @c type is set to SMS_DATA_TYPE_GSM7BIT, input data is treated as string of hexadecimal
 * characters where each pair of two characters form a single byte. Those bytes are treated as
 * GSM 7 bit Default Alphabet as specified in 3GPP TS 23.038 Section 6.2.
 *
 * This function does not support sending of 8 bit binary data messages or UCS2 encoded text.
 *
 * Concatenated messages are not supported in this function.
 *
 * @param[in] number Recipient number in international format.
 * @param[in] data Data to be sent.
 * @param[in] data_len Data length.
 * @param[in] type Input data type.
 *
 * @retval -EINVAL Invalid parameter.
 * @retval -E2BIG Too much data.
 * @return 0 on success, otherwise error code.
 *         A positive value on AT error with "ERROR", "+CME ERROR", and "+CMS ERROR" responses.
 *         The type can be resolved with @c nrf_modem_at_err_type and
 *         the error value with @c nrf_modem_at_err.
 */
int sms_send(const char *number, const uint8_t *data, uint16_t data_len, enum sms_data_type type);

/** @} */

#ifdef __cplusplus
}
#endif

#endif /* SMS_H_ */
