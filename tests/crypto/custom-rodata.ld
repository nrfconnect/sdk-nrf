
. = ALIGN(4);
__start_test_vector_hash_data = .;
KEEP(*(test_vector_hash_data))
__stop_test_vector_hash_data = .;

. = ALIGN(4);
__start_test_vector_hash_long_data = .;
KEEP(*(test_vector_hash_long_data))
__stop_test_vector_hash_long_data = .;

. = ALIGN(4);
__start_test_case_data = .;
KEEP(*(test_case_data))
__stop_test_case_data = .;
