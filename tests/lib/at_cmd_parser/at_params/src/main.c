#include <ztest.h>
#include <stdio.h>
#include <string.h>
#include <kernel.h>

#include <at_cmd_parser/at_cmd_parser.h>
#include <modem/at_params.h>

#define TEST_PARAMS 4

static struct at_param_list test_list;

static void test_init_free_params_list(void)
{
	zassert_equal(-EINVAL, at_params_list_init(NULL, TEST_PARAMS),
		      "Init function initializes with NULL parameter");
	zassert_equal(0, at_params_list_init(&test_list, TEST_PARAMS),
		      "Not able to initialize params list");

	zassert_equal(TEST_PARAMS, test_list.param_count,
		      "Params count should be the same as TEST_PARAMS");

	at_params_list_free(&test_list);

	zassert_equal(0, test_list.param_count,
		      "Params list count is not 0 after free");
	zassert_equal_ptr(NULL, test_list.params,
			  "Params is not NULL after free");
}

static void test_params_put_get_int_setup(void)
{
	at_params_list_init(&test_list, TEST_PARAMS);
}

static void test_params_put_get_int(void)
{
	u16_t tmp_short = 0;
	u32_t tmp_int   = 0;

	zassert_equal(-EINVAL, at_params_short_put(NULL, 0, 1),
		      "at_params_short_put should return -EINVAL");

	zassert_equal(-EINVAL, at_params_int_put(NULL, 0, 1),
		      "at_params_int_put should return -EINVAL");

	zassert_equal(-EINVAL, at_params_short_put(NULL, TEST_PARAMS, 1),
		      "at_params_short_put should return -EINVAL");

	zassert_equal(-EINVAL, at_params_int_put(NULL, TEST_PARAMS, 1),
		      "at_params_int_put should return -EINVAL");

	zassert_equal(0, at_params_short_put(&test_list, 0, 65535),
		      "at_params_short_put should return 0");
	zassert_equal(0, at_params_int_put(&test_list, 1, 65536),
		      "at_params_int_put should return 0");

	zassert_equal(-EINVAL, at_params_short_get(&test_list, 1, &tmp_short),
		      "at_params_short_get should return -EINVAL");

	zassert_equal(0, at_params_int_get(&test_list, 0, &tmp_int),
		      "at_params_int_get should return 0");

	zassert_equal(0, at_params_short_get(&test_list, 0, &tmp_short),
		      "at_params_short_get should return 0");
	zassert_equal(65535, tmp_short, "Short should be 65535");

	zassert_equal(0, at_params_int_get(&test_list, 1, &tmp_int),
		      "at_params_int_get should return 0");
	zassert_equal(65536, tmp_int, "Integer should be 65536");
}

static void test_params_put_get_int_teardown(void)
{
	at_params_list_free(&test_list);
}

static void test_params_put_get_string_setup(void)
{
	at_params_list_init(&test_list, TEST_PARAMS);
	at_params_int_put(&test_list, 0, 1);
}

static void test_params_put_get_string(void)
{
	const char test_str[] = "Test, 1, 2, 3";
	char test_buf[32];
	size_t test_buf_len = sizeof(test_buf);

	zassert_equal(-EINVAL, at_params_string_put(NULL, 1,
					test_str, sizeof(test_str)),
		      "String put should return -EINVAL");

	zassert_equal(-EINVAL, at_params_string_put(&test_list, 1,
					NULL, sizeof(test_str)),
		      "String put should return -EINVAL");

	zassert_equal(0, at_params_string_put(&test_list, 1,
					      test_str, sizeof(test_str)),
		      "String put should return 0");

	zassert_equal(-EINVAL, at_params_string_get(NULL, 1,
						    test_buf, &test_buf_len),
		      "String get should return -EINVAL");

	zassert_equal(-EINVAL, at_params_string_get(&test_list, 1,
						    NULL, &test_buf_len),
		      "String get should return -EINVAL");

	zassert_equal(-EINVAL, at_params_string_get(&test_list, 1,
						    test_buf, NULL),
		      "String get should return -EINVAL");

	zassert_equal(-EINVAL, at_params_string_get(&test_list, 0,
						    test_buf, &test_buf_len),
		      "String get should return -EINVAL");

	test_buf_len = 0;
	zassert_equal(-ENOMEM, at_params_string_get(&test_list, 1,
						    test_buf, &test_buf_len),
		      "String get should return -ENOMEM");

	test_buf_len = sizeof(test_buf);
	zassert_equal(0, at_params_string_get(&test_list, 1,
					      test_buf, &test_buf_len),
		      "String get should return 0");

	zassert_equal(sizeof(test_str), test_buf_len,
		      "test_buf_len should be equal to sizeof(test_str)");

	zassert_equal(0, memcmp(test_str, test_buf, sizeof(test_str)),
		      "test_str and test_buf should be equal");
}

static void test_params_put_get_string_teardown(void)
{
	at_params_list_free(&test_list);
}

static void test_params_put_get_array_setup(void)
{
	at_params_list_init(&test_list, TEST_PARAMS);
	at_params_int_put(&test_list, 0, 1);
}

static void test_params_put_get_array(void)
{
	const u32_t test_array[] = {1, 2, 3, 4, 5, 6, 7};
	u32_t       test_buf[32];
	size_t      test_buf_len = sizeof(test_buf);

	zassert_equal(-EINVAL, at_params_array_put(NULL, 1,
					test_array, sizeof(test_array)),
		      "Array put should return -EINVAL");

	zassert_equal(-EINVAL, at_params_array_put(&test_list, 1,
					NULL, sizeof(test_array)),
		      "Array put should return -EINVAL");

	zassert_equal(0, at_params_array_put(&test_list, 1,
					      test_array, sizeof(test_array)),
		      "Array put should return 0");

	zassert_equal(-EINVAL, at_params_array_get(NULL, 1,
						    test_buf, &test_buf_len),
		      "Array get should return -EINVAL");

	zassert_equal(-EINVAL, at_params_array_get(&test_list, 1,
						    NULL, &test_buf_len),
		      "Array get should return -EINVAL");

	zassert_equal(-EINVAL, at_params_array_get(&test_list, 1,
						    test_buf, NULL),
		      "Array get should return -EINVAL");

	zassert_equal(-EINVAL, at_params_array_get(&test_list, 0,
						    test_buf, &test_buf_len),
		      "Array get should return -EINVAL");

	test_buf_len = 0;
	zassert_equal(-ENOMEM, at_params_array_get(&test_list, 1,
						    test_buf, &test_buf_len),
		      "Array get should return -ENOMEM");

	test_buf_len = sizeof(test_buf);
	zassert_equal(0, at_params_array_get(&test_list, 1,
					      test_buf, &test_buf_len),
		      "Array get should return 0");

	zassert_equal(sizeof(test_array), test_buf_len,
		      "test_buf_len should be equal to sizeof(test_array)");

	zassert_equal(0, memcmp(test_array, test_buf, sizeof(test_array)),
		      "test_array and test_buf should be equal");
}

static void test_params_put_get_array_teardown(void)
{
	at_params_list_free(&test_list);
}

static void test_params_get_type_setup(void)
{
	const char    test_str[] = "Test, 1, 2, 3";
	const u32_t test_array[] = {1, 2, 3, 4, 5, 6, 7};

	at_params_list_init(&test_list, TEST_PARAMS);
	at_params_short_put(&test_list, 0, 2);
	at_params_int_put(&test_list, 1, 1);
	at_params_string_put(&test_list, 2, test_str, sizeof(test_str));
	at_params_array_put(&test_list, 3, test_array, sizeof(test_array));
}

static void test_params_get_type(void)
{
	zassert_equal(AT_PARAM_TYPE_INVALID, at_params_type_get(NULL, 0),
		      "Get type should return AT_PARAM_TYPE_INVALID");

	zassert_equal(AT_PARAM_TYPE_INVALID, at_params_type_get(&test_list, 4),
		      "Get type should return AT_PARAM_TYPE_INVALID");

	zassert_equal(AT_PARAM_TYPE_NUM_SHORT, at_params_type_get(&test_list,
								  0),
		      "Get type should return AT_PARAM_TYPE_NUM_SHORT");

	zassert_equal(AT_PARAM_TYPE_NUM_INT, at_params_type_get(&test_list, 1),
		      "Get type should return AT_PARAM_TYPE_NUM_INT");

	zassert_equal(AT_PARAM_TYPE_STRING, at_params_type_get(&test_list, 2),
		      "Get type should return AT_PARAM_TYPE_STRING");

	zassert_equal(AT_PARAM_TYPE_ARRAY, at_params_type_get(&test_list, 3),
		      "Get type should return AT_PARAM_TYPE_ARRAY");
}

static void test_params_get_type_teardown(void)
{
	at_params_list_free(&test_list);
}

static void test_params_put_empty_setup(void)
{
	at_params_list_init(&test_list, TEST_PARAMS);
}

static void test_params_put_empty(void)
{
	enum at_param_type test_type;

	zassert_equal(-EINVAL, at_params_empty_put(NULL, 0),
		      "Put empty should return -EINVAL");

	zassert_equal(-EINVAL, at_params_empty_put(&test_list, TEST_PARAMS),
		      "Put empty should return -EINVAL");

	zassert_equal(0, at_params_empty_put(&test_list, 0),
		      "Put empty should return 0");

	test_type = at_params_type_get(&test_list, 0);
	zassert_equal(AT_PARAM_TYPE_EMPTY, test_type,
		      "test_type should equal to AT_PARAM_TYPE_EMPTY");
}

static void test_params_put_empty_teardown(void)
{
	at_params_list_free(&test_list);
}

static void test_params_get_count_setup(void)
{
	at_params_list_init(&test_list, TEST_PARAMS);
}

static void test_params_get_count(void)
{
	const char test_str[]    = "Hello World!";
	const u32_t test_array[] = {1, 2, 3, 4, 5, 6, 7};

	zassert_equal(-EINVAL, at_params_valid_count_get(NULL),
		      "Get count should return -EINVAL");

	zassert_equal(0, at_params_valid_count_get(&test_list),
		      "Get count should return 0");

	at_params_empty_put(&test_list, 0);

	zassert_equal(1, at_params_valid_count_get(&test_list),
		      "Get count should return 1");

	at_params_short_put(&test_list, 0, 1);
	zassert_equal(1, at_params_valid_count_get(&test_list),
		      "Get count should return 1");

	at_params_int_put(&test_list, 1, 1);
	zassert_equal(2, at_params_valid_count_get(&test_list),
		      "Get count should return 2");

	at_params_string_put(&test_list, 2, test_str, sizeof(test_str));
	zassert_equal(3, at_params_valid_count_get(&test_list),
		      "Get count should return 3");

	at_params_array_put(&test_list, 3, test_array, sizeof(test_array));
	zassert_equal(4, at_params_valid_count_get(&test_list),
		      "Get count should return 4");
}

static void test_params_get_count_teardown(void)
{
	at_params_list_free(&test_list);
}

static void test_params_get_size_setup(void)
{
	at_params_list_init(&test_list, TEST_PARAMS);
}

static void test_params_get_size(void)
{
	size_t len;
	const char test_str[]    = "Hello World!";
	const u32_t test_array[] = {1, 2, 3, 4, 5, 6, 7};

	zassert_equal(-EINVAL, at_params_size_get(NULL, 0, &len),
		      "Get size should return -EINVAL");

	zassert_equal(-EINVAL, at_params_size_get(&test_list, 0, NULL),
		      "Get size should return -EINVAL");

	zassert_equal(0, at_params_size_get(&test_list, 0, &len),
		      "Get size should return 0");

	at_params_short_put(&test_list, 0, 1);

	at_params_size_get(&test_list, 0, &len);
	zassert_equal(sizeof(u16_t), len,
		      "Get size should return sizeof(u16_t)");

	at_params_int_put(&test_list, 0, 1);

	at_params_size_get(&test_list, 0, &len);
	zassert_equal(sizeof(u32_t), len,
		      "Get size should return sizeof(u32_t)");

	at_params_string_put(&test_list, 0, test_str, sizeof(test_str));

	at_params_size_get(&test_list, 0, &len);
	zassert_equal(sizeof(test_str), len,
		      "Get size should return sizeof(test_str)");

	at_params_array_put(&test_list, 0, test_array, sizeof(test_array));

	at_params_size_get(&test_list, 0, &len);
	zassert_equal(sizeof(test_array), len,
		      "Get size should return sizeof(test_array)");

	at_params_empty_put(&test_list, 0);

	at_params_size_get(&test_list, 0, &len);
	zassert_equal(0, len,
		      "Get size should return 0");
}

static void test_params_get_size_teardown(void)
{
	at_params_list_free(&test_list);
}

static void test_params_list_management_setup(void)
{
	at_params_list_init(&test_list, TEST_PARAMS);
}

static void test_params_list_management(void)
{
	int   ret;
	u32_t tmp_int;

	const char test_str[]    = "Hello World!";
	const u32_t test_array[] = {1, 2, 3, 4, 5, 6, 7};

	for (int i = 0; i <= TEST_PARAMS; ++i) {
		ret = at_params_int_put(&test_list, i, 1);
	}

	zassert_equal(-EINVAL, ret, "Last put int should return -EINVAL");
	zassert_equal(TEST_PARAMS, at_params_valid_count_get(&test_list),
		      "Get valid count should return TEST_PARAMS");

	zassert_equal(0, at_params_string_put(&test_list, 1,
					      test_str, sizeof(test_str)),
		      "Put string should return 0");
	zassert_equal(0, at_params_array_put(&test_list, 3,
					      test_array, sizeof(test_array)),
		      "Put string should return 0");

	zassert_equal(TEST_PARAMS, at_params_valid_count_get(&test_list),
		      "Get valid count should return TEST_PARAMS");

	zassert_equal(AT_PARAM_TYPE_STRING, at_params_type_get(&test_list, 1),
		      "Get type should return AT_PARAM_TYPE_STRING");

	zassert_equal(AT_PARAM_TYPE_ARRAY, at_params_type_get(&test_list, 3),
		      "Get type should return AT_PARAM_TYPE_ARRAY");

	at_params_list_clear(&test_list);
	zassert_equal(0, at_params_valid_count_get(&test_list),
		      "Params valid count should return 0");
	zassert_equal(-EINVAL, at_params_int_get(&test_list, 0, &tmp_int),
		      "Params int get should return -EINVAL");
}

static void test_params_list_management_teardown(void)
{
	at_params_list_free(&test_list);
}

void test_main(void)
{
	ztest_test_suite(at_cmd_parser,
			 ztest_unit_test(test_init_free_params_list),
			 ztest_unit_test_setup_teardown(
					test_params_put_get_int,
					test_params_put_get_int_setup,
					test_params_put_get_int_teardown),
			 ztest_unit_test_setup_teardown(
					test_params_put_get_string,
					test_params_put_get_string_setup,
					test_params_put_get_string_teardown),
			 ztest_unit_test_setup_teardown(
					test_params_put_get_array,
					test_params_put_get_array_setup,
					test_params_put_get_array_teardown),
			 ztest_unit_test_setup_teardown(
					test_params_get_type,
					test_params_get_type_setup,
					test_params_get_type_teardown),
			 ztest_unit_test_setup_teardown(
					test_params_put_empty,
					test_params_put_empty_setup,
					test_params_put_empty_teardown),
			 ztest_unit_test_setup_teardown(
					test_params_get_count,
					test_params_get_count_setup,
					test_params_get_count_teardown),
			 ztest_unit_test_setup_teardown(
					test_params_get_size,
					test_params_get_size_setup,
					test_params_get_size_teardown),
			 ztest_unit_test_setup_teardown(
					test_params_list_management,
					test_params_list_management_setup,
					test_params_list_management_teardown)
			);

	ztest_run_test_suite(at_cmd_parser);
}
