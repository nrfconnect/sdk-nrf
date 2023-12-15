/*
 * Copyright (c) 2019 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef AT_PARAMS_H__
#define AT_PARAMS_H__

#include <zephyr/types.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @file at_params.h
 *
 * @brief Store a list of AT command/response parameters.
 * @defgroup at_params AT command/response parameters
 * @{
 *
 * A parameter list contains an array of parameters defined by a type,
 * a length and a value. Those parameters could be arguments of an AT
 * command, AT response or event, for example.
 * Several parameter types can be stored. They can be arrays or a single
 * numeric or string values. Optional or empty parameters are supported.
 * The same list of parameters can be reused. Each parameter can be
 * updated or cleared. A parameter type or value can be changed at any
 * time. Once the parameter list is created, its size cannot be changed.
 * All parameters values are copied in the list. Parameters should be
 * cleared to free that memory. Getter and setter methods are available
 * to read and write parameter values.
 */

/** @brief Parameter types that can be stored. */
enum at_param_type {
	/** Invalid parameter, typically a parameter that does not exist. */
	AT_PARAM_TYPE_INVALID,
	/** Parameter of type integer. */
	AT_PARAM_TYPE_NUM_INT,
	/** Parameter of type string. */
	AT_PARAM_TYPE_STRING,
	/** Parameter of type array. */
	AT_PARAM_TYPE_ARRAY,
	/** Empty or optional parameter that should be skipped. */
	AT_PARAM_TYPE_EMPTY,
};

/** @brief Parameter value. */
union at_param_value {
	/** Integer value. */
	int64_t int_val;
	/** String value. */
	char *str_val;
	/** Array of uint32_t */
	uint32_t *array_val;
};

/** @brief A parameter is defined with a type, length and value. */
struct at_param {
	enum at_param_type type;
	size_t size;
	union at_param_value value;
};

/**
 * @brief List of AT parameters that compose an AT command or response.
 *
 * Contains an array of opaque data. Setter and getter methods should be used
 * to get access to the parameters in the array.
 */
struct at_param_list {
	size_t param_count;
	struct at_param *params;
};

/**
 * @brief Create a list of parameters.
 *
 * An array of @p max_params_count is allocated. Each parameter is
 * initialized to its default value. This function should not be called
 * again before freeing the list.
 *
 * @param[in] list Parameter list to initialize.
 * @param[in] max_params_count Maximum number of element that the list can
 * store.
 *
 * @retval 0 If the operation was successful.
 *           Otherwise, a (negative) error code is returned.
 */
int at_params_list_init(struct at_param_list *list, size_t max_params_count);

/**
 * @brief Clear/reset all parameter types and values.
 *
 * All parameter types and values are reset to default values.
 *
 * @param[in] list Parameter list to clear.
 */
void at_params_list_clear(struct at_param_list *list);

/**
 * @brief Free a list of parameters.
 *
 * First the list is cleared. Then the list and its elements are deleted.
 *
 * @param[in] list Parameter list to free.
 */
void at_params_list_free(struct at_param_list *list);

/**
 * @brief Add a parameter in the list at the specified index and assign it an
 * integer value.
 *
 * If a parameter exists at this index, it is replaced.
 *
 * @param[in] list      Parameter list.
 * @param[in] index     Index in the list where to put the parameter.
 * @param[in] value     Parameter value.
 *
 * @retval 0 If the operation was successful.
 *           Otherwise, a (negative) error code is returned.
 */
int at_params_int_put(const struct at_param_list *list, size_t index, int64_t value);

/**
 * @brief Add a parameter in the list at the specified index and assign it a
 * string value.
 *
 * The parameter string value is copied and added to the list as a
 * null-terminated string. If a parameter exists at this index, it is replaced.
 *
 * @param[in] list    Parameter list.
 * @param[in] index   Index in the list where to put the parameter.
 * @param[in] str     Pointer to the string value.
 * @param[in] str_len Number of characters of the string value @p str.
 *
 * @retval 0 If the operation was successful.
 *           Otherwise, a (negative) error code is returned.
 */
int at_params_string_put(const struct at_param_list *list, size_t index,
			 const char *str, size_t str_len);

/**
 * @brief Add a parameter in the list at the specified index and assign it an
 * array type value.
 *
 * The parameter array value is copied and added to the list.
 * If a parameter exists at this index, it is replaced. Only numbers (uint32_t)
 * are currently supported. If the list contain compound values the parser
 * will try to convert the value. Either 0 will be stored or if the value start
 * with a numeric value that value will be converted, the rest of the value
 * will be ignored. Ie. 5-23 will result in 5.
 *
 * @param[in] list      Parameter list.
 * @param[in] index     Index in the list where to put the parameter.
 * @param[in] array     Pointer to the array of number values.
 * @param[in] array_len In bytes (must currently be divisible by 4)
 *
 * @retval 0 If the operation was successful.
 *           Otherwise, a (negative) error code is returned.
 */
int at_params_array_put(const struct at_param_list *list, size_t index,
			const uint32_t *array, size_t array_len);

/**
 * @brief Add a parameter in the list at the specified index and assign it an
 * empty status.
 *
 * This will indicate that an empty parameter was found when parsing the
 * AT string.
 *
 * @param[in] list    Parameter list.
 * @param[in] index   Index in the list where to put the parameter.
 *
 * @retval 0 If the operation was successful.
 *           Otherwise, a (negative) error code is returned.
 */
int at_params_empty_put(const struct at_param_list *list, size_t index);

/**
 * @brief Get the size of a given parameter (in bytes).
 *
 * A size of '0' is returned for invalid and empty parameters.
 *
 * @param[in] list    Parameter list.
 * @param[in] index   Parameter index in the list.
 * @param[out] len    Length of the parameter in bytes.
 *
 * @retval 0 If the operation was successful.
 *           Otherwise, a (negative) error code is returned.
 */
int at_params_size_get(const struct at_param_list *list, size_t index,
		       size_t *len);

/**
 * @brief Get a parameter value as a short number.
 *
 * @param[in] list    Parameter list.
 * @param[in] index   Parameter index in the list.
 * @param[out] value  Parameter value.
 *
 * @retval 0 If the operation was successful.
 *           Otherwise, a (negative) error code is returned.
 */
int at_params_short_get(const struct at_param_list *list, size_t index,
			int16_t *value);

/**
 * @brief Get a parameter value as an unsigned short number.
 *
 * @param[in] list    Parameter list.
 * @param[in] index   Parameter index in the list.
 * @param[out] value  Parameter value.
 *
 * @retval 0 If the operation was successful.
 *           Otherwise, a (negative) error code is returned.
 */
int at_params_unsigned_short_get(const struct at_param_list *list, size_t index,
			uint16_t *value);

/**
 * @brief Get a parameter value as an integer number.
 *
 * @param[in] list    Parameter list.
 * @param[in] index   Parameter index in the list.
 * @param[out] value  Parameter value.
 *
 * @retval 0 If the operation was successful.
 *           Otherwise, a (negative) error code is returned.
 */
int at_params_int_get(const struct at_param_list *list, size_t index,
		      int32_t *value);

/**
 * @brief Get a parameter value as an unsigned integer number.
 *
 * @param[in] list    Parameter list.
 * @param[in] index   Parameter index in the list.
 * @param[out] value  Parameter value.
 *
 * @retval 0 If the operation was successful.
 *           Otherwise, a (negative) error code is returned.
 */
int at_params_unsigned_int_get(const struct at_param_list *list, size_t index, uint32_t *value);

/**
 * @brief Get a parameter value as a signed 64-bit integer number.
 *
 * @param[in] list    Parameter list.
 * @param[in] index   Parameter index in the list.
 * @param[out] value  Parameter value.
 *
 * @retval 0 If the operation was successful.
 *           Otherwise, a (negative) error code is returned.
 */
int at_params_int64_get(const struct at_param_list *list, size_t index, int64_t *value);

/**
 * @brief Get a parameter value as a string.
 *
 * The parameter type must be a string, or an error is returned.
 * The string parameter value is copied to the buffer.
 * @p len must be bigger than the string length, or an error is returned.
 * The copied string is not null-terminated.
 *
 * @param[in]     list    Parameter list.
 * @param[in]     index   Parameter index in the list.
 * @param[in]     value   Pointer to the buffer where to copy the value.
 * @param[in,out] len     Available space in @p value, returns actual length
 *                        copied into string buffer in bytes.
 *
 * @retval 0 If the operation was successful.
 *           Otherwise, a (negative) error code is returned.
 */
int at_params_string_get(const struct at_param_list *list, size_t index,
			 char *value, size_t *len);

/**
 * @brief Get a pointer to the string parameter value.
 *
 * The parameter type must be a string, or an error is returned.
 * Allows for custom copying of the value. at_params_list_clear will invalidate the pointer.
 * String parameter is not null-terminated.
 *
 * @param[in]     list       Parameter list.
 * @param[in]     index      Parameter index in the list.
 * @param[out]    at_param   Pointer to the address of the string.
 * @param[out]    len        Length of the parameter value.
 *
 * @retval 0 If the operation was successful.
 *           Otherwise, a (negative) error code is returned.
 */
int at_params_string_ptr_get(const struct at_param_list *list, size_t index, const char **at_param,
			     size_t *len);

/**
 * @brief Get a parameter value as an array.
 *
 * The parameter type must be an array, or an error is returned.
 * The string parameter value is copied to the buffer.
 * @p len must be equal or bigger than the array length,
 * or an error is returned. The copied string is not null-terminated.
 *
 * @param[in]     list    Parameter list.
 * @param[in]     index   Parameter index in the list.
 * @param[out]    array   Pointer to the buffer where to copy the array.
 * @param[in,out] len     Available space in @p value, returns actual length
 *                        copied into array buffer in bytes.
 *
 * @retval 0 If the operation was successful.
 *           Otherwise, a (negative) error code is returned.
 */
int at_params_array_get(const struct at_param_list *list, size_t index,
			uint32_t *array, size_t *len);

/**
 * @brief Get the number of valid parameters in the list.
 *
 * @param[in] list    Parameter list.
 *
 * @return The number of valid parameters until an empty parameter is found.
 */
uint32_t at_params_valid_count_get(const struct at_param_list *list);

/**
 * @brief Get parameter type for parameter at index
 *
 * @param[in] list    Parameter list.
 * @param[in] index   Parameter index in the list.
 *
 * @return Return parameter type of @ref at_param_type.
 */
enum at_param_type at_params_type_get(const struct at_param_list *list,
				      size_t index);

/** @} */

#ifdef __cplusplus
}
#endif

#endif /* AT_PARAMS_H__ */
