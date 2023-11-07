/*
 * Copyright (c) 2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef ZIGBEE_APP_UTILS_H__
#define ZIGBEE_APP_UTILS_H__

#include <stdint.h>
#include <stdbool.h>

#include <zboss_api.h>

/** @file zigbee_app_utils.h
 *
 * @defgroup zigbee_app_utils Zigbee application utilities library.
 * @{
 * @brief  Library with helper functions and routines.
 *
 * @details Provides Zigbee default handler, helper functions for parsing
 * and converting Zigbee data, indicating status of the device at a network
 * using onboard LEDs.
 */

#ifdef __cplusplus
extern "C" {
#endif

/**@brief Function for setting the Erase persistent storage,
 *        depending on the erase pin.
 *
 * If the erase pin (1.39 by default, defined in zigbee_app_utils.c)
 * is shortened to the ground, then the persistent storage is erased.
 * Otherwise, whether the storage is erased is decided upon the input
 * parameter 'erase'. This behaviour is only valid if PCA10056 is used.
 *
 * @param[in] erase Whether to erase the persistent storage in case
 *                  the erase pin is not shortened to the ground.
 */
void zigbee_erase_persistent_storage(zb_bool_t erase);

/**@brief Function for converting an input buffer to a hex string.
 *
 * @param[out] out       Pointer to the output buffer.
 * @param[in]  out_size  Size of the output buffer.
 * @param[in]  in        Pointer to the input buffer.
 * @param[in]  in_size   Size of the input buffer.
 * @param[in]  reverse   If true, data output happens in the reverse order.
 *
 * @return     snprintf-compatible value. Less than zero means encoding error.
 *             Non-negative value is the number of characters that
 *             would have been written if the supplied buffer had been
 *             large enough. Value greater than or equal to buf_len means
 *             that the supplied buffer was too small.
 *
 * @note    Null terminator is written if buf_len is large enough,
 *          but does not count for the return value.
 */
int to_hex_str(char *out, uint16_t out_size, const uint8_t *in,
	       uint8_t in_size, bool reverse);

/**@brief Read array of uint8_t from hex string.
 *
 * @param in_str        Pointer to the input hex string.
 * @param in_str_len    Length, in characters, of the input string.
 * @param out_buff      Pointer to the output uint8_t array.
 * @param out_buff_size Size, in bytes, of the output uint8_t array.
 * @param reverse       If true then parse from end to start.
 *
 * @retval true   if the conversion succeed
 * @retval false  if the conversion failed
 */
bool parse_hex_str(char const *in_str, uint8_t in_str_len,
		   uint8_t *out_buff, uint8_t out_buff_size, bool reverse);

/**@brief Parse a hex string to uint8_t.
 *
 * The function verifies if input is valid, i.e., if all input characters
 * are valid hex digits. If an invalid character is found then function fails.
 *
 * @param s        Pointer to input string.
 * @param value    Pointer to output value.
 *
 * @retval true   if the conversion succeed
 * @retval false  if the conversion failed
 */
static inline bool parse_hex_u8(char const *s, uint8_t *value)
{
	return parse_hex_str(s, strlen(s), value, sizeof(*value), true);
}

/**@brief Parse a hex string to uint16_t.
 *
 * The function verifies if input is valid, i.e., if all input characters
 * are valid hex digits. If an invalid character is found then function fails.
 *
 * @param s        Pointer to input string.
 * @param value    Pointer to output value.
 *
 * @retval true   if the conversion succeed
 * @retval false  if the conversion failed
 */
static inline bool parse_hex_u16(char const *s, uint16_t *value)
{
	return parse_hex_str(s, strlen(s), (uint8_t *)value, sizeof(*value), true);
}

/**@brief Function for converting 64-bit address to hex string.
 *
 * @param[out] str_buf      Pointer to output buffer.
 * @param[in]  buf_len      Length of the buffer pointed by str_buf.
 * @param[in]  in           Zigbee IEEE address to be converted to string.
 *
 * @return     snprintf-compatible value. Less than zero means encoding error.
 *             Non-negative value is the number of characters that would
 *             have been written if the supplied buffer had been large enough.
 *             Value greater than or equal to buf_len means that the supplied
 *             buffer was too small.
 *
 * @note    Null terminator is written if buf_len is large enough,
 *          but does not count for the return value.
 */
int ieee_addr_to_str(char *str_buf, uint16_t buf_len,
		     const zb_ieee_addr_t in);

/**@brief Address type.
 *
 * @ref ADDR_SHORT and @ref ADDR_LONG correspond to APS addressing
 * mode constants and must not be changed.
 */
typedef enum {
	ADDR_INVALID    = 0,
	ADDR_ANY        = 1,
	ADDR_SHORT      = 2,    /* ZB_APS_ADDR_MODE_16_ENDP_PRESENT */
	ADDR_LONG       = 3,    /* ZB_APS_ADDR_MODE_64_ENDP_PRESENT */
} addr_type_t;

/**@brief Function for parsing a null-terminated string of hex characters
 *        into 64-bit or 16-bit address.
 *
 * The function will skip 0x suffix from input if present.
 *
 * @param input     Pointer to the input string string representing
 *                  the address in big endian.
 * @param output    Pointer to the resulting zb_addr_u variable.
 * @param addr_type Expected address type.
 *
 * @return Conversion result.
 */
addr_type_t parse_address(const char *input, zb_addr_u *output,
			  addr_type_t addr_type);

/**@brief Function for parsing a null-terminated string of hex characters
 *        into a 64-bit address.
 *
 * The function will skip 0x suffix from input if present.
 *
 * @param input   Pointer to the input string representing the address
 *                in big endian.
 * @param addr    Variable where the address will be placed.
 *
 * @retval true   if the conversion succeed
 * @retval false  if the conversion failed
 */
static inline bool parse_long_address(const char *input, zb_ieee_addr_t addr)
{
	return (parse_address(input, (zb_addr_u *)addr,
			      ADDR_LONG) != ADDR_INVALID);
}

/**@brief Function for parsing a null-terminated string of hex characters
 *        into 16-bit address.
 *
 * The function will skip 0x suffix from input if present.
 *
 * @param input   Pointer to the input string representing
 *                the address in big endian.
 * @param addr    Pointer to the variable where address will be placed.
 *
 * @retval true   if the conversion succeed
 * @retval false  if the conversion failed
 */
static inline bool parse_short_address(const char *input, zb_uint16_t *addr)
{
	return (parse_address(input, (zb_addr_u *)addr,
			      ADDR_SHORT) != ADDR_INVALID);
}

/**@brief Function for passing signals to the default
 *        Zigbee stack event handler.
 *
 * @note This function does not free the Zigbee buffer.
 *
 * @param[in] bufid Reference to the Zigbee stack buffer used to pass signal.
 *
 * @return RET_OK on success or error code on failure.
 */
zb_ret_t zigbee_default_signal_handler(zb_bufid_t bufid);

/**@brief Function for indicating the Zigbee network connection
 *        status on LED.
 *
 * If the device is successfully commissioned, the LED is turned on.
 * If the device is not commissioned or has left the network,
 * the LED is turned off.
 *
 * @note This function does not free the Zigbee buffer.
 *
 * @param[in] bufid   Reference to the Zigbee stack buffer
 *                    used to pass signal.
 * @param[in] led_idx LED index, as defined in the board-specific
 *                    BSP header. The index starts from 0.
 */
void zigbee_led_status_update(zb_bufid_t bufid, uint32_t led_idx);

/**@brief Function for indicating the default signal handler
 *        about user input on the device.
 *
 * If the device is not commissioned, the rejoin procedure is started.
 *
 * @note This function is to be used with End Devices only.
 *
 */
#if defined CONFIG_ZIGBEE_ROLE_END_DEVICE
void user_input_indicate(void);

/**@brief Function for enabling sleepy behavior of End Device. Must be called
 *        before zigbee_enable() is called. Also must be called after ZB_INIT(),
 *        which is called when the system is initializing.
 *
 * @note This function is to be used with End Devices only.
 *
 * @param[in] enable   Boolean value indicating if sleepy behavior
 *                     should be enabled or disabled.
 */
void zigbee_configure_sleepy_behavior(bool enable);
#endif /* CONFIG_ZIGBEE_ROLE_END_DEVICE */

#if defined CONFIG_ZIGBEE_FACTORY_RESET

/**@brief Registers which button and for how long has to be pressed in order to do Factory Reset.
 *
 * @note Must be called once before check_factory_reset_button function can be used.
 *
 * @param[in] button Development Kit button to be used as Factory Reset button.
 */
void register_factory_reset_button(uint32_t button);

/**@brief Checks if Factory Reset button was pressed.
 *        If so, it initiates the procedure of checking if it was pressed for specified time.
 *
 * @note register_factory_reset_button function has to be called before (once)
 *
 * @param[in] button_state state of Development Kit buttons; passed from button handler
 * @param[in] has_changed determines which buttons changed; passed from button handler
 */
void check_factory_reset_button(uint32_t button_state, uint32_t has_changed);

/**@brief Indicates whether Factory Reset was started as a result of a button press or not.
 *
 * @return true if Factory Reset was started; false if Factory Reset was not started
 */
bool was_factory_reset_done(void);

#endif /* CONFIG_ZIGBEE_FACTORY_RESET */

#ifdef __cplusplus
}
#endif

/**@} */

#endif /* ZIGBEE_APP_UTILS_H__ */
