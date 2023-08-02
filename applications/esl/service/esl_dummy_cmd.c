/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/logging/log.h>

#include "esl_client.h"
#include "esl_internal.h"
#include "esl_client_tag_storage.h"
LOG_MODULE_DECLARE(esl_c);

extern uint16_t esl_addr_start;

void esl_dummy_ap_ad_data(uint8_t sync_pkt_type, uint8_t group_id)
{
	uint8_t buf[ESL_ENCRTYPTED_DATA_MAX_LEN], len;
	struct bt_esl_client *esl_c_obj = esl_c_get_esl_c_obj();

	buf[0] = group_id;
	switch (sync_pkt_type) {
	/* type 0 NO OP pkt ,send no operation to broadcast 0xFF */
	case 0:
		LOG_DBG("type 0 NO OP pkt ,send no operation to broadcast 0xFF");
		buf[1] = OP_PING;
		buf[2] = ESL_ADDR_BROADCAST;
		len = 2 + sizeof(uint8_t);
		break;
	/* type 1 led 0 broadcast flashing */
	case 1:
		LOG_INF("type 1 broadcast led 0 flashing");
		buf[1] = OP_LED_CONTROL;
		buf[2] = ESL_ADDR_BROADCAST;
		buf[3] = 0;
		buf[4] = 0xf0;
		buf[5] = 0x41;
		buf[6] = 0x10;
		buf[7] = 0x00;
		buf[8] = 0x00;
		buf[9] = 0x00;
		buf[10] = 0x32;
		buf[11] = 0xfa;
		buf[12] = 0x1f;
		buf[13] = 0x00;
		len = 13 + sizeof(uint8_t);
		break;
	/* type 2 led 1 broadcast flashing */
	case 2:
		LOG_INF("type 2 broadcast led 1 flashing");
		buf[1] = OP_LED_CONTROL;
		buf[2] = ESL_ADDR_BROADCAST;
		buf[3] = 1;
		buf[4] = 0xf0;
		buf[5] = 0x41;
		buf[6] = 0x10;
		buf[7] = 0x00;
		buf[8] = 0x00;
		buf[9] = 0x00;
		buf[10] = 0x32;
		buf[11] = 0xfa;
		buf[12] = 0x1f;
		buf[13] = 0x00;
		len = 13 + sizeof(uint8_t);
		break;
	/* type 3 led 0 broadcast on */
	case 3:
		LOG_INF("type 3 broadcast led 0 on");
		buf[1] = OP_LED_CONTROL;
		buf[2] = ESL_ADDR_BROADCAST;
		buf[3] = 0;
		buf[4] = 0xc0;
		buf[5] = 0x00;
		buf[6] = 0x00;
		buf[7] = 0x00;
		buf[8] = 0x00;
		buf[9] = 0x00;
		buf[10] = 0x00;
		buf[11] = 0x00;
		buf[12] = 0x01;
		buf[13] = 0x00;
		len = 13 + sizeof(uint8_t);
		break;
	/* type 4 led 0 broadcast off */
	case 4:
		LOG_INF("type 4 broadcast led 0 off");
		buf[1] = OP_LED_CONTROL;
		buf[2] = ESL_ADDR_BROADCAST;
		buf[3] = 0;
		buf[4] = 0xc0;
		buf[5] = 0x00;
		buf[6] = 0x00;
		buf[7] = 0x00;
		buf[8] = 0x00;
		buf[9] = 0x00;
		buf[10] = 0x00;
		buf[11] = 0x00;
		buf[12] = 0x00;
		buf[13] = 0x00;
		len = 13 + sizeof(uint8_t);
		break;
	/* type 5 led 1 broadcast on */
	case 5:
		LOG_INF("type 5 broadcast led 1 on");
		buf[1] = OP_LED_CONTROL;
		buf[2] = ESL_ADDR_BROADCAST;
		buf[3] = 0;
		buf[4] = 0xc0;
		buf[5] = 0x00;
		buf[6] = 0x00;
		buf[7] = 0x00;
		buf[8] = 0x00;
		buf[9] = 0x00;
		buf[10] = 0x00;
		buf[11] = 0x00;
		buf[12] = 0x01;
		buf[13] = 0x00;
		len = 13 + sizeof(uint8_t);
		break;
	/* type 6 led 1 broadcast off */
	case 6:
		LOG_INF("type 6 broadcast led 1 off");
		buf[1] = OP_LED_CONTROL;
		buf[2] = ESL_ADDR_BROADCAST;
		buf[3] = 1;
		buf[4] = 0xc0;
		buf[5] = 0x00;
		buf[6] = 0x00;
		buf[7] = 0x00;
		buf[8] = 0x00;
		buf[9] = 0x00;
		buf[10] = 0x00;
		buf[11] = 0x00;
		buf[12] = 0x00;
		buf[13] = 0x00;
		len = 13 + sizeof(uint8_t);
		break;
	/* type 7 display 0 img 0 */
	case 7:
		LOG_INF("type 7 broadcast display 0 img 0");
		buf[1] = OP_DISPLAY_IMAGE;
		buf[2] = ESL_ADDR_BROADCAST;
		buf[3] = 0x00;
		buf[4] = 0x00;
		len = 4 + sizeof(uint8_t);
		break;
	/* type 8 display 0 img 1 */
	case 8:
		LOG_INF("type 8 broadcast display 0 img 1");
		buf[1] = OP_DISPLAY_IMAGE;
		buf[2] = ESL_ADDR_BROADCAST;
		buf[3] = 0x00;
		buf[4] = 0x01;
		len = 4 + sizeof(uint8_t);
		break;
	/** type 9 read sensor 0 broadcast
	 *  Ask tag to prepare sensor data, need to issue addressed command again
	 **/
	case 9:
		LOG_INF("type 9 broadcast read sensor 0");
		buf[1] = OP_READ_SENSOR_DATA;
		buf[2] = ESL_ADDR_BROADCAST;
		buf[3] = 0; /* sensor index 0*/
		len = 3 + sizeof(uint8_t);
		break;
	/* type A ping esl_id 0x38 0x39 0x3a 0x3b 0x3c */
	case 0xA:
		LOG_INF("type A ping esl_id 0x38 0x39 0x3a 0x3b 0x3c");
		buf[1] = OP_PING;
		buf[2] = CONFIG_ESL_CLIENT_DEFAULT_ESL_ID;
		buf[3] = OP_PING;
		buf[4] = CONFIG_ESL_CLIENT_DEFAULT_ESL_ID + 1;
		buf[5] = OP_PING;
		buf[6] = CONFIG_ESL_CLIENT_DEFAULT_ESL_ID + 2;
		buf[7] = OP_PING;
		buf[8] = CONFIG_ESL_CLIENT_DEFAULT_ESL_ID + 3;
		buf[9] = OP_PING;
		buf[10] = CONFIG_ESL_CLIENT_DEFAULT_ESL_ID + 4;
		len = 10 + sizeof(uint8_t);
		break;
	/* type B led 0 flashing 0x38,0x39,0x3a */
	case 0xB:
		LOG_INF("type B led 0 flashing 0x38,0x39,0x3a");
		buf[1] = OP_LED_CONTROL;
		buf[2] = CONFIG_ESL_CLIENT_DEFAULT_ESL_ID;
		buf[3] = 0; /* LED 0*/
		buf[4] = 0xf0;
		buf[5] = 0x41;
		buf[6] = 0x10;
		buf[7] = 0x00;
		buf[8] = 0x00;
		buf[9] = 0x00;
		buf[10] = 0x32;
		buf[11] = 0xfa;
		buf[12] = 0x1f;
		buf[13] = 0x00;
		buf[14] = OP_LED_CONTROL;
		buf[15] = CONFIG_ESL_CLIENT_DEFAULT_ESL_ID + 1;
		buf[16] = 0; /* LED 0 */
		buf[17] = 0xf0;
		buf[18] = 0x41;
		buf[19] = 0x10;
		buf[20] = 0x00;
		buf[21] = 0x00;
		buf[22] = 0x00;
		buf[23] = 0x32;
		buf[24] = 0xfa;
		buf[25] = 0x1f;
		buf[26] = 0x00;
		buf[27] = OP_LED_CONTROL;
		buf[28] = CONFIG_ESL_CLIENT_DEFAULT_ESL_ID + 2;
		buf[29] = 0; /* LED 0*/
		buf[30] = 0xf0;
		buf[31] = 0x41;
		buf[32] = 0x10;
		buf[33] = 0x00;
		buf[34] = 0x00;
		buf[35] = 0x00;
		buf[36] = 0x32;
		buf[37] = 0xfa;
		buf[38] = 0x1f;
		buf[39] = 0x00;
		len = 39 + sizeof(uint8_t);
		break;
	/** type C led 0 flashing 0x3b,0x3c
	 *  Fill first 2 slots with broadcast ping to make response slot begin with 2
	 */
	case 0xC:
		LOG_INF("type C led 0 flashing 0x3b,0x3c");
		buf[1] = OP_PING;
		buf[2] = ESL_ADDR_BROADCAST;
		buf[3] = OP_PING;
		buf[4] = ESL_ADDR_BROADCAST;
		buf[5] = OP_LED_CONTROL;
		buf[6] = CONFIG_ESL_CLIENT_DEFAULT_ESL_ID + 3;
		buf[7] = 0; /* LED 0*/
		buf[8] = 0xf0;
		buf[9] = 0x41;
		buf[10] = 0x10;
		buf[11] = 0x00;
		buf[12] = 0x00;
		buf[13] = 0x00;
		buf[14] = 0x32;
		buf[15] = 0xfa;
		buf[16] = 0x1f;
		buf[17] = 0x00;
		buf[18] = OP_LED_CONTROL;
		buf[19] = CONFIG_ESL_CLIENT_DEFAULT_ESL_ID + 4;
		buf[20] = 0; /* LED 0 */
		buf[21] = 0xf0;
		buf[22] = 0x41;
		buf[23] = 0x10;
		buf[24] = 0x00;
		buf[25] = 0x00;
		buf[26] = 0x00;
		buf[27] = 0x32;
		buf[28] = 0xfa;
		buf[29] = 0x1f;
		buf[30] = 0x00;
		len = 30 + sizeof(uint8_t);
		break;
	/* type D led 1 flashing 0x38,0x39,0x3a */
	case 0xD:
		LOG_INF("type D led 1 flashing 0x38,0x39,0x3a");
		buf[1] = OP_LED_CONTROL;
		buf[2] = CONFIG_ESL_CLIENT_DEFAULT_ESL_ID;
		buf[3] = 1; /* LED 0*/
		buf[4] = 0xf0;
		buf[5] = 0x41;
		buf[6] = 0x10;
		buf[7] = 0x00;
		buf[8] = 0x00;
		buf[9] = 0x00;
		buf[10] = 0x32;
		buf[11] = 0xfa;
		buf[12] = 0x1f;
		buf[13] = 0x00;
		buf[14] = OP_LED_CONTROL;
		buf[15] = CONFIG_ESL_CLIENT_DEFAULT_ESL_ID + 1;
		buf[16] = 1; /* LED 0 */
		buf[17] = 0xf0;
		buf[18] = 0x41;
		buf[19] = 0x10;
		buf[20] = 0x00;
		buf[21] = 0x00;
		buf[22] = 0x00;
		buf[23] = 0x32;
		buf[24] = 0xfa;
		buf[25] = 0x1f;
		buf[26] = 0x00;
		buf[27] = OP_LED_CONTROL;
		buf[28] = CONFIG_ESL_CLIENT_DEFAULT_ESL_ID + 2;
		buf[29] = 1; /* LED 0*/
		buf[30] = 0xf0;
		buf[31] = 0x41;
		buf[32] = 0x10;
		buf[33] = 0x00;
		buf[34] = 0x00;
		buf[35] = 0x00;
		buf[36] = 0x32;
		buf[37] = 0xfa;
		buf[38] = 0x1f;
		buf[39] = 0x00;
		len = 39 + sizeof(uint8_t);
		break;
	/** type E led 1 flashing 0x3b,0x3c
	 *  Fill first 2 slots with broadcast ping to make response slot begin with 2
	 */
	case 0xE:
		LOG_INF("type E led 1 flashing 0x3b,0x3c");
		buf[1] = OP_PING;
		buf[2] = ESL_ADDR_BROADCAST;
		buf[3] = OP_PING;
		buf[4] = ESL_ADDR_BROADCAST;
		buf[5] = OP_LED_CONTROL;
		buf[6] = CONFIG_ESL_CLIENT_DEFAULT_ESL_ID + 3;
		buf[7] = 1; /* LED 0*/
		buf[8] = 0xf0;
		buf[9] = 0x41;
		buf[10] = 0x10;
		buf[11] = 0x00;
		buf[12] = 0x00;
		buf[13] = 0x00;
		buf[14] = 0x32;
		buf[15] = 0xfa;
		buf[16] = 0x1f;
		buf[17] = 0x00;
		buf[18] = OP_LED_CONTROL;
		buf[19] = CONFIG_ESL_CLIENT_DEFAULT_ESL_ID + 4;
		buf[20] = 1; /* LED 0 */
		buf[21] = 0xf0;
		buf[22] = 0x41;
		buf[23] = 0x10;
		buf[24] = 0x00;
		buf[25] = 0x00;
		buf[26] = 0x00;
		buf[27] = 0x32;
		buf[28] = 0xfa;
		buf[29] = 0x1f;
		buf[30] = 0x00;
		len = 30 + sizeof(uint8_t);
		break;
	/** type f read sensor 0 0x38,0x39,0x3a,0x3b,0x3c
	 *
	 **/
	case 0xf:
		LOG_INF("type f read sensor 0 0x38,0x39,0x3a,0x3b,0x3c");
		buf[1] = OP_READ_SENSOR_DATA;
		buf[2] = CONFIG_ESL_CLIENT_DEFAULT_ESL_ID;
		buf[3] = 0; /* sensor index 0*/
		buf[4] = OP_READ_SENSOR_DATA;
		buf[5] = CONFIG_ESL_CLIENT_DEFAULT_ESL_ID + 1;
		buf[6] = 0; /* sensor index 0*/
		buf[7] = OP_READ_SENSOR_DATA;
		buf[8] = CONFIG_ESL_CLIENT_DEFAULT_ESL_ID + 2;
		buf[9] = 0; /* sensor index 0*/
		buf[10] = OP_READ_SENSOR_DATA;
		buf[11] = CONFIG_ESL_CLIENT_DEFAULT_ESL_ID + 3;
		buf[12] = 0; /* sensor index 0*/
		buf[13] = OP_READ_SENSOR_DATA;
		buf[14] = CONFIG_ESL_CLIENT_DEFAULT_ESL_ID + 4;
		buf[15] = 0; /* sensor index 0*/
		len = 15 + sizeof(uint8_t);
		break;

	/* type 0x10 ESLP/ESL/SYNC/BI-03-I [Response TLV Too Long] */
	case 0x10:
		LOG_INF("type 0x10 ESLP/ESL/SYNC/BI-03-I [Response TLV Too Long]");
		buf[1] = OP_SERVICE_RESET;
		buf[2] = CONFIG_ESL_CLIENT_DEFAULT_ESL_ID;
		buf[3] = OP_SERVICE_RESET;
		buf[4] = CONFIG_ESL_CLIENT_DEFAULT_ESL_ID;
		buf[5] = OP_SERVICE_RESET;
		buf[6] = CONFIG_ESL_CLIENT_DEFAULT_ESL_ID;
		buf[7] = OP_SERVICE_RESET;
		buf[8] = CONFIG_ESL_CLIENT_DEFAULT_ESL_ID;
		buf[9] = OP_SERVICE_RESET;
		buf[10] = CONFIG_ESL_CLIENT_DEFAULT_ESL_ID;
		buf[11] = OP_SERVICE_RESET;
		buf[12] = CONFIG_ESL_CLIENT_DEFAULT_ESL_ID;
		buf[13] = OP_SERVICE_RESET;
		buf[14] = CONFIG_ESL_CLIENT_DEFAULT_ESL_ID;
		buf[15] = OP_SERVICE_RESET;
		buf[16] = CONFIG_ESL_CLIENT_DEFAULT_ESL_ID;
		buf[17] = OP_SERVICE_RESET;
		buf[18] = CONFIG_ESL_CLIENT_DEFAULT_ESL_ID;
		buf[19] = OP_SERVICE_RESET;
		buf[20] = CONFIG_ESL_CLIENT_DEFAULT_ESL_ID;
		buf[21] = OP_SERVICE_RESET;
		buf[22] = CONFIG_ESL_CLIENT_DEFAULT_ESL_ID;
		buf[23] = OP_SERVICE_RESET;
		buf[24] = CONFIG_ESL_CLIENT_DEFAULT_ESL_ID;
		buf[25] = OP_SERVICE_RESET;
		buf[26] = CONFIG_ESL_CLIENT_DEFAULT_ESL_ID;
		buf[27] = OP_SERVICE_RESET;
		buf[28] = CONFIG_ESL_CLIENT_DEFAULT_ESL_ID;
		buf[29] = OP_SERVICE_RESET;
		buf[30] = CONFIG_ESL_CLIENT_DEFAULT_ESL_ID;
		buf[31] = OP_SERVICE_RESET;
		buf[32] = CONFIG_ESL_CLIENT_DEFAULT_ESL_ID;
		buf[33] = OP_SERVICE_RESET;
		buf[34] = CONFIG_ESL_CLIENT_DEFAULT_ESL_ID;
		len = 34 + sizeof(uint8_t);
		break;
	/** type 0x11 middle of 24 pings
	 *  This command generally won't send a response, since we don't have enough response slot.
	 */
	case 0x11:
		LOG_INF("type 0x11 middle of 24 pings");
		buf[1] = OP_PING;
		buf[2] = CONFIG_ESL_CLIENT_DEFAULT_ESL_ID + 1;
		buf[3] = OP_PING;
		buf[4] = CONFIG_ESL_CLIENT_DEFAULT_ESL_ID + 1;
		buf[5] = OP_PING;
		buf[6] = CONFIG_ESL_CLIENT_DEFAULT_ESL_ID + 1;
		buf[7] = OP_PING;
		buf[8] = CONFIG_ESL_CLIENT_DEFAULT_ESL_ID + 1;
		buf[9] = OP_PING;
		buf[10] = CONFIG_ESL_CLIENT_DEFAULT_ESL_ID + 1;
		buf[11] = OP_PING;
		buf[12] = CONFIG_ESL_CLIENT_DEFAULT_ESL_ID + 1;
		buf[13] = OP_PING;
		buf[14] = CONFIG_ESL_CLIENT_DEFAULT_ESL_ID + 1;
		buf[15] = OP_PING;
		buf[16] = CONFIG_ESL_CLIENT_DEFAULT_ESL_ID + 1;
		buf[17] = OP_PING;
		buf[18] = CONFIG_ESL_CLIENT_DEFAULT_ESL_ID + 1;
		buf[19] = OP_PING;
		buf[20] = CONFIG_ESL_CLIENT_DEFAULT_ESL_ID + 1;
		buf[21] = OP_PING;
		buf[22] = CONFIG_ESL_CLIENT_DEFAULT_ESL_ID + 1;
		buf[23] = OP_PING;
		buf[24] = CONFIG_ESL_CLIENT_DEFAULT_ESL_ID;
		buf[25] = OP_PING;
		buf[26] = CONFIG_ESL_CLIENT_DEFAULT_ESL_ID + 1;
		buf[27] = OP_PING;
		buf[28] = CONFIG_ESL_CLIENT_DEFAULT_ESL_ID + 1;
		buf[29] = OP_PING;
		buf[30] = CONFIG_ESL_CLIENT_DEFAULT_ESL_ID + 1;
		buf[31] = OP_PING;
		buf[32] = CONFIG_ESL_CLIENT_DEFAULT_ESL_ID + 1;
		buf[33] = OP_PING;
		buf[34] = CONFIG_ESL_CLIENT_DEFAULT_ESL_ID + 1;
		buf[35] = OP_PING;
		buf[36] = CONFIG_ESL_CLIENT_DEFAULT_ESL_ID + 1;
		buf[37] = OP_PING;
		buf[38] = CONFIG_ESL_CLIENT_DEFAULT_ESL_ID + 1;
		buf[39] = OP_PING;
		buf[40] = CONFIG_ESL_CLIENT_DEFAULT_ESL_ID + 1;
		buf[41] = OP_PING;
		buf[42] = CONFIG_ESL_CLIENT_DEFAULT_ESL_ID + 1;
		buf[43] = OP_PING;
		buf[44] = CONFIG_ESL_CLIENT_DEFAULT_ESL_ID + 1;
		buf[45] = OP_PING;
		buf[46] = CONFIG_ESL_CLIENT_DEFAULT_ESL_ID + 1;
		len = 46 + sizeof(uint8_t);
		break;
	/* type 0X12 Tag 0~10 Display Img 0 */
	case 0x12:
		LOG_INF("type 0X12 Tag 0~10 Display Img 0");
		buf[1] = OP_DISPLAY_IMAGE;
		buf[2] = CONFIG_ESL_CLIENT_DEFAULT_ESL_ID;
		buf[3] = 0x00;
		buf[4] = 0x00;
		buf[5] = OP_DISPLAY_IMAGE;
		buf[6] = CONFIG_ESL_CLIENT_DEFAULT_ESL_ID + 1;
		buf[7] = 0x00;
		buf[8] = 0x00;
		buf[9] = OP_DISPLAY_IMAGE;
		buf[10] = CONFIG_ESL_CLIENT_DEFAULT_ESL_ID + 2;
		buf[11] = 0x00;
		buf[12] = 0x00;
		buf[13] = OP_DISPLAY_IMAGE;
		buf[14] = CONFIG_ESL_CLIENT_DEFAULT_ESL_ID + 3;
		buf[15] = 0x00;
		buf[16] = 0x00;
		buf[17] = OP_DISPLAY_IMAGE;
		buf[18] = CONFIG_ESL_CLIENT_DEFAULT_ESL_ID + 4;
		buf[19] = 0x00;
		buf[20] = 0x00;
		buf[21] = OP_DISPLAY_IMAGE;
		buf[22] = CONFIG_ESL_CLIENT_DEFAULT_ESL_ID + 5;
		buf[23] = 0x00;
		buf[24] = 0x00;
		buf[25] = OP_DISPLAY_IMAGE;
		buf[26] = CONFIG_ESL_CLIENT_DEFAULT_ESL_ID + 6;
		buf[27] = 0x00;
		buf[28] = 0x00;
		buf[29] = OP_DISPLAY_IMAGE;
		buf[30] = CONFIG_ESL_CLIENT_DEFAULT_ESL_ID + 7;
		buf[31] = 0x00;
		buf[32] = 0x00;
		buf[33] = OP_DISPLAY_IMAGE;
		buf[34] = CONFIG_ESL_CLIENT_DEFAULT_ESL_ID + 8;
		buf[35] = 0x00;
		buf[36] = 0x00;
		buf[37] = OP_DISPLAY_IMAGE;
		buf[38] = CONFIG_ESL_CLIENT_DEFAULT_ESL_ID + 9;
		buf[39] = 0x00;
		buf[40] = 0x00;
		buf[41] = OP_DISPLAY_IMAGE;
		buf[42] = CONFIG_ESL_CLIENT_DEFAULT_ESL_ID + 10;
		buf[43] = 0x00;
		buf[44] = 0x00;
		len = 44 + sizeof(uint8_t);
		break;
	/* type 0X13 Tag 0~10 Display img 1 */
	case 0x13:
		LOG_INF("type 0X13 Tag 0~10 Display img 1");
		buf[1] = OP_DISPLAY_IMAGE;
		buf[2] = CONFIG_ESL_CLIENT_DEFAULT_ESL_ID;
		buf[3] = 0x00;
		buf[4] = 0x01;
		buf[5] = OP_DISPLAY_IMAGE;
		buf[6] = CONFIG_ESL_CLIENT_DEFAULT_ESL_ID + 1;
		buf[7] = 0x00;
		buf[8] = 0x01;
		buf[9] = OP_DISPLAY_IMAGE;
		buf[10] = CONFIG_ESL_CLIENT_DEFAULT_ESL_ID + 2;
		buf[11] = 0x00;
		buf[12] = 0x01;
		buf[13] = OP_DISPLAY_IMAGE;
		buf[14] = CONFIG_ESL_CLIENT_DEFAULT_ESL_ID + 3;
		buf[15] = 0x00;
		buf[16] = 0x01;
		buf[17] = OP_DISPLAY_IMAGE;
		buf[18] = CONFIG_ESL_CLIENT_DEFAULT_ESL_ID + 4;
		buf[19] = 0x00;
		buf[20] = 0x01;
		buf[21] = OP_DISPLAY_IMAGE;
		buf[22] = CONFIG_ESL_CLIENT_DEFAULT_ESL_ID + 5;
		buf[23] = 0x00;
		buf[24] = 0x01;
		buf[25] = OP_DISPLAY_IMAGE;
		buf[26] = CONFIG_ESL_CLIENT_DEFAULT_ESL_ID + 6;
		buf[27] = 0x00;
		buf[28] = 0x01;
		buf[29] = OP_DISPLAY_IMAGE;
		buf[30] = CONFIG_ESL_CLIENT_DEFAULT_ESL_ID + 7;
		buf[31] = 0x00;
		buf[32] = 0x01;
		buf[33] = OP_DISPLAY_IMAGE;
		buf[34] = CONFIG_ESL_CLIENT_DEFAULT_ESL_ID + 8;
		buf[35] = 0x00;
		buf[36] = 0x01;
		buf[37] = OP_DISPLAY_IMAGE;
		buf[38] = CONFIG_ESL_CLIENT_DEFAULT_ESL_ID + 9;
		buf[39] = 0x00;
		buf[40] = 0x01;
		buf[41] = OP_DISPLAY_IMAGE;
		buf[42] = CONFIG_ESL_CLIENT_DEFAULT_ESL_ID + 10;
		buf[43] = 0x00;
		buf[44] = 0x01;
		len = 44 + sizeof(uint8_t);
		break;
	/* type 0X14 Tag 0~10 Display img 2 */
	case 0x14:
		LOG_INF("type 0X14 Tag 0~10 Display img 2");
		buf[1] = OP_DISPLAY_IMAGE;
		buf[2] = CONFIG_ESL_CLIENT_DEFAULT_ESL_ID;
		buf[3] = 0x00;
		buf[4] = 0x02;
		buf[5] = OP_DISPLAY_IMAGE;
		buf[6] = CONFIG_ESL_CLIENT_DEFAULT_ESL_ID + 1;
		buf[7] = 0x00;
		buf[8] = 0x02;
		buf[9] = OP_DISPLAY_IMAGE;
		buf[10] = CONFIG_ESL_CLIENT_DEFAULT_ESL_ID + 2;
		buf[11] = 0x00;
		buf[12] = 0x02;
		buf[13] = OP_DISPLAY_IMAGE;
		buf[14] = CONFIG_ESL_CLIENT_DEFAULT_ESL_ID + 3;
		buf[15] = 0x00;
		buf[16] = 0x02;
		buf[17] = OP_DISPLAY_IMAGE;
		buf[18] = CONFIG_ESL_CLIENT_DEFAULT_ESL_ID + 4;
		buf[19] = 0x00;
		buf[20] = 0x02;
		buf[21] = OP_DISPLAY_IMAGE;
		buf[22] = CONFIG_ESL_CLIENT_DEFAULT_ESL_ID + 5;
		buf[23] = 0x00;
		buf[24] = 0x02;
		buf[25] = OP_DISPLAY_IMAGE;
		buf[26] = CONFIG_ESL_CLIENT_DEFAULT_ESL_ID + 6;
		buf[27] = 0x00;
		buf[28] = 0x02;
		buf[29] = OP_DISPLAY_IMAGE;
		buf[30] = CONFIG_ESL_CLIENT_DEFAULT_ESL_ID + 7;
		buf[31] = 0x00;
		buf[32] = 0x02;
		buf[33] = OP_DISPLAY_IMAGE;
		buf[34] = CONFIG_ESL_CLIENT_DEFAULT_ESL_ID + 8;
		buf[35] = 0x00;
		buf[36] = 0x02;
		buf[37] = OP_DISPLAY_IMAGE;
		buf[38] = CONFIG_ESL_CLIENT_DEFAULT_ESL_ID + 9;
		buf[39] = 0x00;
		buf[40] = 0x02;
		buf[41] = OP_DISPLAY_IMAGE;
		buf[42] = CONFIG_ESL_CLIENT_DEFAULT_ESL_ID + 10;
		buf[43] = 0x00;
		buf[44] = 0x02;
		len = 44 + sizeof(uint8_t);
		break;

	default:
		LOG_ERR("invalid sync_pkt_type");
		return;
	}

	esl_c_obj->sync_buf[group_id].data_len =
		esl_compose_ad_data(esl_c_obj->sync_buf[group_id].data, buf, len,
				    esl_c_obj->esl_randomizer, esl_c_obj->esl_ap_key);
	esl_c_obj->sync_buf[group_id].status = SYNC_READY_TO_PUSH;
	LOG_HEXDUMP_DBG(esl_c_obj->sync_buf[group_id].data, esl_c_obj->sync_buf[group_id].data_len,
			"final data");
}
