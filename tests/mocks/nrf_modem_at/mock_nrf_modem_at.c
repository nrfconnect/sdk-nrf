/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <unity.h>
#include <string.h>
#include <zephyr/kernel.h>

#if defined(CONFIG_MOCK_NRF_MODEM_AT_PRINTF)

/** Size of the buffer used for vnsprintf. */
#define MOCK_NRF_MODEM_AT_PRINTF_BUFFER_SIZE 4096

/**
 * Mock information for a single nrf_modem_at_printf() call.
 */
struct printf_mock {
	const char *fmt;
	int return_value;
};

/* Currently processed nrf_modem_at_printf() call */
static int printf_mock_index;
/* Number of expected nrf_modem_at_printf() calls */
static int printf_mock_count;
/* Mock information for multiple nrf_modem_at_printf() calls */
static struct printf_mock printf_mocks[CONFIG_MOCK_NRF_MODEM_AT_PRINTF_CALL_COUNT];

#endif

#if defined(CONFIG_MOCK_NRF_MODEM_AT_SCANF)

/**
 * Variable argument type.
 */
enum scanf_varg_type {
	SCANF_VARG_TYPE_NONE,
	SCANF_VARG_TYPE_INT,
	SCANF_VARG_TYPE_INT32,
	SCANF_VARG_TYPE_INT16,
	SCANF_VARG_TYPE_UINT32,
	SCANF_VARG_TYPE_UINT16,
	SCANF_VARG_TYPE_STRING
};

/**
 * Variable argument value.
 */
struct scanf_varg_value {
	union {
		int i;
		uint32_t u32;
		uint16_t u16;
		int32_t s32;
		int16_t s16;
		char str[CONFIG_MOCK_NRF_MODEM_AT_SCANF_VARGS_STR_SIZE];
	};
};

/**
 * A single variable argument type and value.
 */
struct scanf_mock_varg {
	enum scanf_varg_type type;
	struct scanf_varg_value value;
};

/**
 * Mock information for a single nrf_modem_at_scanf() call.
 */
struct scanf_mock {
	const char *cmd;
	const char *fmt;
	int return_value;
	int varg_index;
	struct scanf_mock_varg vargs[CONFIG_MOCK_NRF_MODEM_AT_SCANF_VARGS_COUNT];
};

/* Currently processed nrf_modem_at_scanf() call */
static int scanf_mock_index;
/* Number of expected nrf_modem_at_scanf() calls */
static int scanf_mock_count;
/* Mock information for multiple nrf_modem_at_scanf() calls */
static struct scanf_mock scanf_mocks[CONFIG_MOCK_NRF_MODEM_AT_SCANF_CALL_COUNT];

#endif

void mock_nrf_modem_at_Init(void)
{
#if defined(CONFIG_MOCK_NRF_MODEM_AT_PRINTF)
	memset(printf_mocks, 0, sizeof(printf_mocks));
	printf_mock_index = 0;
	printf_mock_count = -1;
#endif
#if defined(CONFIG_MOCK_NRF_MODEM_AT_SCANF)
	memset(scanf_mocks, 0, sizeof(scanf_mocks));
	scanf_mock_index = 0;
	scanf_mock_count = -1;
#endif
}

void mock_nrf_modem_at_Verify(void)
{
#if defined(CONFIG_MOCK_NRF_MODEM_AT_PRINTF)
	TEST_ASSERT_MESSAGE(printf_mock_index > printf_mock_count,
		"nrf_modem_at_printf called fewer times than expected");
#endif
#if defined(CONFIG_MOCK_NRF_MODEM_AT_SCANF)
	TEST_ASSERT_MESSAGE(scanf_mock_index > scanf_mock_count,
		"nrf_modem_at_scanf called fewer times than expected");
#endif
}

#if defined(CONFIG_MOCK_NRF_MODEM_AT_PRINTF)

void __mock_nrf_modem_at_printf_ExpectAndReturn(const char *fmt, int return_value)
{
	printf_mock_count++;

	TEST_ASSERT_MESSAGE(
		printf_mock_count < CONFIG_MOCK_NRF_MODEM_AT_PRINTF_CALL_COUNT,
		"Too many nrf_modem_at_printf calls set to be expected. "
		"Please increase CONFIG_MOCK_NRF_MODEM_AT_PRINTF_CALL_COUNT.");

	printf_mocks[printf_mock_count].fmt = fmt;
	printf_mocks[printf_mock_count].return_value = return_value;
}

int __cmock_nrf_modem_at_printf(const char *fmt, ...)
{
	static char string[MOCK_NRF_MODEM_AT_PRINTF_BUFFER_SIZE];
	va_list args;
	struct printf_mock *mock_data = &printf_mocks[printf_mock_index];
	int len;

	memset(string, 0, sizeof(string));

	TEST_ASSERT_MESSAGE(printf_mock_index <= printf_mock_count,
		"nrf_modem_at_printf called more times than expected");

	va_start(args, fmt);
	len = vsnprintf(string, MOCK_NRF_MODEM_AT_PRINTF_BUFFER_SIZE, fmt, args);
	va_end(args);

	TEST_ASSERT_MESSAGE(
		len < MOCK_NRF_MODEM_AT_PRINTF_BUFFER_SIZE,
		"Unit under test calls nrf_modem_at_printf with too big final formatted string. "
		"Please increase MOCK_NRF_MODEM_AT_PRINTF_BUFFER_SIZE in mock_nrf_modem_at.c");

	TEST_ASSERT_EQUAL_STRING(mock_data->fmt, string);

	printf_mock_index++;

	return mock_data->return_value;
}

#endif /* CONFIG_MOCK_NRF_MODEM_AT_PRINTF */

#if defined(CONFIG_MOCK_NRF_MODEM_AT_SCANF)

void __mock_nrf_modem_at_scanf_ExpectAndReturn(
	const char *cmd, const char *fmt, int return_value)
{
	scanf_mock_count++;

	TEST_ASSERT_MESSAGE(
		scanf_mock_count < CONFIG_MOCK_NRF_MODEM_AT_SCANF_CALL_COUNT,
		"Too many nrf_modem_at_scanf calls set to be expected. "
		"Please increase CONFIG_MOCK_NRF_MODEM_AT_SCANF_CALL_COUNT.");

	scanf_mocks[scanf_mock_count].cmd = cmd;
	scanf_mocks[scanf_mock_count].fmt = fmt;
	scanf_mocks[scanf_mock_count].return_value = return_value;
	scanf_mocks[scanf_mock_count].varg_index = 0;
}

/**
 * @brief Get next variable argument to be expected for nrf_modem_at_scanf() call.
 *
 * @return Next variable argument.
 */
static struct scanf_mock_varg *__mock_nrf_modem_at_scanf_ReturnVarg_get_next(void)
{
	TEST_ASSERT_MESSAGE(
		scanf_mock_count != -1,
		"__mock_nrf_modem_at_scanf_ExpectAndReturn must be called before "
		"setting vargs");

	/* Check that there are not too many variable arguments */
	TEST_ASSERT_MESSAGE(
		scanf_mocks[scanf_mock_count].varg_index <
		CONFIG_MOCK_NRF_MODEM_AT_SCANF_VARGS_COUNT,
		"Too many vargs defined with __mock_nrf_modem_at_scanf_ReturnVarg_*() "
		"functions. Please increase CONFIG_MOCK_NRF_MODEM_AT_SCANF_VARGS_COUNT.");

	struct scanf_mock_varg *varg =
		&scanf_mocks[scanf_mock_count].vargs[scanf_mocks[scanf_mock_count].varg_index];
	scanf_mocks[scanf_mock_count].varg_index++;
	return varg;
}

void __mock_nrf_modem_at_scanf_ReturnVarg_int(int value)
{
	struct scanf_mock_varg *varg = __mock_nrf_modem_at_scanf_ReturnVarg_get_next();

	varg->type = SCANF_VARG_TYPE_INT;
	varg->value.i = value;
}

void __mock_nrf_modem_at_scanf_ReturnVarg_uint32(uint32_t value)
{
	struct scanf_mock_varg *varg = __mock_nrf_modem_at_scanf_ReturnVarg_get_next();

	varg->type = SCANF_VARG_TYPE_UINT32;
	varg->value.u32 = value;
}

void __mock_nrf_modem_at_scanf_ReturnVarg_uint16(uint16_t value)
{
	struct scanf_mock_varg *varg = __mock_nrf_modem_at_scanf_ReturnVarg_get_next();

	varg->type = SCANF_VARG_TYPE_UINT16;
	varg->value.u16 = value;
}

void __mock_nrf_modem_at_scanf_ReturnVarg_int32(int32_t value)
{
	struct scanf_mock_varg *varg = __mock_nrf_modem_at_scanf_ReturnVarg_get_next();

	varg->type = SCANF_VARG_TYPE_INT32;
	varg->value.s32 = value;
}

void __mock_nrf_modem_at_scanf_ReturnVarg_int16(int16_t value)
{
	struct scanf_mock_varg *varg = __mock_nrf_modem_at_scanf_ReturnVarg_get_next();

	varg->type = SCANF_VARG_TYPE_INT16;
	varg->value.s16 = value;
}

void __mock_nrf_modem_at_scanf_ReturnVarg_string(char *value)
{
	/* Check that given string fits into its buffer.
	 * Smaller than comparison so that NUL character fits also.
	 */
	TEST_ASSERT_MESSAGE(
		strlen(value) < CONFIG_MOCK_NRF_MODEM_AT_SCANF_VARGS_STR_SIZE,
		"Too long string given to __mock_nrf_modem_at_scanf_ReturnVarg_string. "
		"Please increase CONFIG_MOCK_NRF_MODEM_AT_SCANF_VARGS_STR_SIZE.");

	struct scanf_mock_varg *varg = __mock_nrf_modem_at_scanf_ReturnVarg_get_next();

	varg->type = SCANF_VARG_TYPE_STRING;
	strcpy(varg->value.str, value);
}

int __cmock_nrf_modem_at_scanf(const char *cmd, const char *fmt, ...)
{
	int i = 0;
	va_list args;
	struct scanf_mock *mock_data = &scanf_mocks[scanf_mock_index];

	TEST_ASSERT_MESSAGE(scanf_mock_index <= scanf_mock_count,
		"nrf_modem_at_scanf called more times than expected");

	va_start(args, fmt);

	TEST_ASSERT(mock_data->cmd != NULL);

	TEST_ASSERT_EQUAL_STRING(mock_data->cmd, cmd);
	TEST_ASSERT_EQUAL_STRING(mock_data->fmt, fmt);

	while (i < mock_data->varg_index) {

		switch (mock_data->vargs[i].type) {
		case SCANF_VARG_TYPE_INT: {
			int *tmp_param = va_arg(args, int *);

			*tmp_param = mock_data->vargs[i].value.i;
			}
			break;
		case SCANF_VARG_TYPE_INT32: {
			int32_t *tmp_param = va_arg(args, int32_t *);

			*tmp_param = mock_data->vargs[i].value.s32;
			}
			break;
		case SCANF_VARG_TYPE_INT16: {
			int16_t *tmp_param = va_arg(args, int16_t *);

			*tmp_param = mock_data->vargs[i].value.s16;
			}
			break;
		case SCANF_VARG_TYPE_UINT32: {
			uint32_t *tmp_param = va_arg(args, uint32_t *);

			*tmp_param = mock_data->vargs[i].value.u32;
			}
			break;
		case SCANF_VARG_TYPE_UINT16: {
			uint16_t *tmp_param = va_arg(args, uint16_t *);

			*tmp_param = mock_data->vargs[i].value.u16;
			}
			break;
		case SCANF_VARG_TYPE_STRING: {
			char *tmp_param = va_arg(args, char *);

			strcpy(tmp_param, mock_data->vargs[i].value.str);
			}
			break;
		case SCANF_VARG_TYPE_NONE:
			TEST_FAIL_MESSAGE("Argument with SCANF_VARG_TYPE_NONE means a test bug");
			break;
		default:
			TEST_FAIL_MESSAGE("Argument with unknown type means a test bug");
			break;
		}
		i++;
	}

	va_end(args);

	scanf_mock_index++;

	return mock_data->return_value;
}

#endif /* CONFIG_MOCK_NRF_MODEM_AT_SCANF */
