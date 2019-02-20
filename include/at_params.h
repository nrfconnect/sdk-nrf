/**@file at_params.h
 *
 * @brief Store a list of AT commands/responses parameters.
 *
 * A parameter list contains an array of parameters defined by a type
 * and a value. Those parameters could be arguments of an AT command for
 * instance. They can be numeric or string values.
 * The same list of parameters can be reused. Each parameter can be
 * updated or cleared. Once the parameter list is created, its size
 * cannot be changed. All parameters values are copied in the list.
 * Parameters should be cleared to free that memory. Getter and setter
 * methods are available to read parameter values.
 *
 */

/*
 * Copyright (c) 2019 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
 */

#ifndef AT_PARAMS_H_
#define AT_PARAMS_H_

#include <zephyr/types.h>

/**
 * @brief List of AT parameters that compose an AT command or response.
 *
 * Contains an array of opaque data. Setter and getter methods should be used
 * to get access to the parameters in the array.
 */

enum at_param_type {
	AT_PARAM_TYPE_EMPTY,
	AT_PARAM_TYPE_NUM_SHORT,
	AT_PARAM_TYPE_NUM_INT,
	AT_PARAM_TYPE_STRING
};

union at_param_value {
	u16_t short_val;
	u32_t int_val;
	char *str_val;
};

struct at_param {
	enum at_param_type type;
	union at_param_value value;
};

struct at_param_list {
	size_t param_count;
	struct at_param *params;
};

/**
 * @brief Create a list of parameters.
 *
 * An array of @param max_params_count is allocated. Each parameter is
 * initialized to its default value. Should not be called again before freeing
 * the list.
 *
 * @param[in] list List of parameters to initialize.
 * @param[in] max_params_count Maximum number of element that the list can
 * store.
 *
 * @return Zero on success or (negative) error code otherwise.
 */
int at_params_list_init(struct at_param_list *list,
			     size_t max_params_count);


/**
 * @brief Clear/reset all parameter types and values.
 *
 * All parameter types and values are reset to defaults values.
 *
 * @param[in] list List of parameters to clear.
 */
void at_params_list_clear(struct at_param_list *list);


/**
 * @brief Free a list of parameters.
 *
 * First the list is cleared and then the list and its elements are deleted.
 *
 * @param[in] list List of parameters to free.
 */
void at_params_list_free(struct at_param_list *list);


/**
 * @brief Clear/reset a parameter type and value.
 *
 * @param[in] list List of parameters to clear.
 * @param[in] index  Parameter index to clear.
 *
 * @return Zero on success or (negative) error code otherwise.
 */
int at_params_clear(struct at_param_list *list, size_t index);


/**
 * @brief Add a parameter in the list at the specified index and assign it a
 * short value. Any previous parameter will be replaced.
 *
 * @param[in] list    List of parameters.
 * @param[in] index     Index in the list were to put the parameter.
 * @param[in] value     Parameter value.
 *
 * @return Zero on success or (negative) error code otherwise.
 */
int at_params_short_put(const struct at_param_list *list, size_t index,
			u16_t value);


/**
 * @brief Add a parameter in the list at the specified index and assign it an
 * integer value. Any previous parameter will be replaced.
 *
 * @param[in] list    List of parameters.
 * @param[in] index     Index in the list were to put the parameter.
 * @param[in] value     Parameter value.
 *
 * @return Zero on success or (negative) error code otherwise.
 */
int at_params_int_put(const struct at_param_list *list, size_t index,
			u32_t value);


/**
 * @brief Add a parameter in the list at the specified index and assign it a
 * string value. The parameter string value is copied and added to the list. A
 * null terminated string is stored. Any previous parameter will be replaced.
 *
 * @param[in] list    List of parameters.
 * @param[in] index     Index in the list were to put the parameter.
 * @param[in] str     Pointer to the string value.
 * @param[in] str_len   Number of characters of the string value @ref str.
 *
 * @return Zero on success or (negative) error code otherwise.
 */
int at_params_string_put(const struct at_param_list *list,
			size_t index, const char *str,
			size_t str_len);


/**
 * @brief Get the size of a given parameter in byte.
 * A missing parameter has a size of '0'.
 *
 * @param[in] list    List of parameters.
 * @param[in] index     Parameter index in the list.
 * @param[out] len    Length of the parameter in byte or the string length.
 *
 * @return Zero on success or (negative) error code otherwise.
 */
int at_params_size_get(const struct at_param_list *list, size_t index,
			size_t *len);


/**
 * @brief Get a parameter value as a short number.
 * Numeric values are stored as unsigned number. The parameter type has to be a
 * short or an error is returned.
 *
 * @param[in] list    List of parameters.
 * @param[in] index     Parameter index in the list.
 * @param[out] value  Parameter value.
 *
 * @return Zero on success or (negative) error code otherwise.
 */
int at_params_short_get(const struct at_param_list *list, size_t index,
			u16_t *value);


/**
 * @brief Get a parameter value as an integer number.
 * Numeric values are stored as unsigned number. The parameter type has to be an
 * integer or an error is returned.
 *
 * @param[in] list    List of parameters.
 * @param[in] index     Parameter index in the list.
 * @param[out] value  Parameter value.
 *
 * @return Zero on success or (negative) error code otherwise.
 */
int at_params_int_get(const struct at_param_list *list, size_t index,
			u32_t *value);


/**
 * @brief Get a parameter value as a string.
 * The parameter type has to be a string or an error is returned.
 * The string parameter value is copied to the buffer.
 * @ref len should be bigger than the string length or an error is returned.
 * The copied string is not NULL terminated.
 *
 * @param[in] list    List of parameters.
 * @param[in] index     Parameter index in the list.
 * @param[in] value   Pointer to the buffer to copy the value to.
 * @param[in] len       Available space in @param value.
 *
 * @return Zero on success or (negative) error code otherwise.
 */
int at_params_string_get(const struct at_param_list *list,
			size_t index, char *value, size_t len);


/**
 * @brief Get the number of valid parameters in the list.
 *
 * @param[in] list    List of parameters.
 *
 * @return The number of valid parameters, until an empty parameter is found.
 */
u32_t at_params_valid_count_get(const struct at_param_list *list);

#endif /* AT_PARAMS_H_ */
