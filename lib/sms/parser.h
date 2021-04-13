/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <stdint.h>
#include <stdbool.h>

#ifndef _PARSER_H_
#define _PARSER_H_

/*
 * Maximum length of a valid SMS message is 175 bytes so we'll reserve buffer
 * of 180 bytes to be safe and we are able to handle properly few extra bytes.
 * 175 is result of the following calculation of maximum field lengths:
 *   12: SMSC address
 *    4: Misc bytes in SMS-DELIVER msg:
 *       First byte with several bit fields, TP-PID, TP-DCS, TP-UDL.
 *   12: Originating address (phone number)
 *    7: Timestamp
 *  140: User data
 */
#define PARSER_BUF_SIZE 180

/* Forward declaration of the parser struct */
struct parser;

typedef int (*parser_module)(struct parser *parser, uint8_t *buf);

/**
 * @brief Set of parser functions needed for parser framework to operate.
 */
struct parser_api {
	/** @brief Function type for getting data size. */
	uint32_t (*data_size)(void);
	/** @brief Function type for getting list of sub parsers that are processed sequentially. */
	void*    (*get_parsers)(void);
	/** @brief Function type for getting payload/data decoder. */
	void*    (*get_decoder)(void);
	/** @brief Function type for getting number of sub parsers. */
	int      (*get_parser_count)(void);
	/** @brief Function type for getting header information. */
	int      (*get_header)(struct parser *parser, void *header);
};

/**
 * @brief Set of parser variables needed for parser framework to operate.
 *
 * @details Holds state information about the parsing as it proceeds by passing this structure
 * to functions storing relevant information out of the parser buffers.
 *
 * Parser contains sub parsers that are available through parser_api and its get_parsers function.
 */
struct parser {
	/** @brief Iterator type of state information on where the parsing is proceeding. */
	uint8_t buf_pos;
	/** @brief Parser data buffer that is processed. */
	uint8_t buf[PARSER_BUF_SIZE];
	/** @brief Size of the actual data in buf. */
	uint16_t buf_size;

	/** @brief Payload output buffer that is processed. */
	uint8_t *payload;
	/** @brief Payload output buffer size. */
	uint8_t payload_buf_size;
	/** @brief Index to the start of the payload within buf. */
	uint8_t payload_pos;

	/** @brief Data structure to hold the parsed information. */
	void *data;

	/** @brief Functions defining the functionality of this specific parser. */
	struct parser_api *api;
};

/**
 * @brief Parser instance creation.
 *
 * @param[in] parser Parser instance.
 * @param[in] api Parser API functions.
 *
 * @return Zero if successful, negative value in error cases.
 */
int parser_create(struct parser *parser, struct parser_api *api);

/**
 * @brief Parser instance deletion.
 *
 * @details This function will destroy a parser instance and free up any memory used.
 *
 * @param[in] parser Parser instance.
 *
 * @return Zero if successful, negative value in error cases.
 */
int parser_delete(struct parser *parser);

/**
 * @brief Parse ASCII formatted hexadecimal string.
 *
 * @details The format of the hexadecimal string is defined in 3GPP TS 27.005 Section 4.0 and 3.1.
 * \<pdu\> definition says the following:
 *   "ME/TA converts each octet of TP data unit into two IRA character long hexadecimal number
 *    (e.g. octet with integer value 42 is presented to TE as two characters 2A (IRA 50 and 65))"
 *
 * @param[in] parser Parser instance.
 * @param[in] data ASCII formatted hex string containing the data to be parsed.
 *
 * @return Zero if successful, negative value in error cases.
 */
int parser_process_str(struct parser *parser, const char *data);

/**
 * @brief Get the payload.
 *
 * @details This function will fill a user supplied buffer with payload data.
 *
 * @param[in] parser Parser instance.
 * @param[in,out] buf A user supplied buffer to put payload data into.
 * @param[in] buf_size How much can be stored in the buffer
 *
 * @return Zero if successful, negative value in error cases.
 */
int parser_get_payload(struct parser *parser, char *buf, uint8_t buf_size);

/**
 * @brief Get the header.
 *
 * @details This function will fill given header buffer with message header information.
 * The header structure is defined in the parser implementation.
 *
 * @param[in] parser Parser instance.
 * @param[in,out] header Parser specific header structure to be filled.
 *
 * @return Zero if successful, negative value in error cases.
 */
int parser_get_header(struct parser *parser, void *header);

#endif /* _PARSER_H_ */
