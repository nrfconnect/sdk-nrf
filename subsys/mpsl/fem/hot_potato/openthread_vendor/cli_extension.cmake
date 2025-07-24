target_compile_definitions(ot-config INTERFACE "OPENTHREAD_CONFIG_CLI_MAX_USER_CMD_ENTRIES=2")

add_library(cli-extension ${CMAKE_CURRENT_LIST_DIR}/cli_extension.cpp)

if(CONFIG_OPENTHREAD_CLI_VENDOR_RADIO_TEST)
    set(radio_test_dir ${CMAKE_CURRENT_LIST_DIR}/../radio_test_nrf54l/src)
    target_sources(cli-extension PRIVATE ${radio_test_dir}/radio_cmd.c ${radio_test_dir}/radio_test.c)
endif()

if(CONFIG_OPENTHREAD_CLI_VENDOR_CPU_USAGE)
    target_sources(cli-extension PRIVATE ${CMAKE_CURRENT_LIST_DIR}/cpu_usage.cpp)
endif()

target_link_libraries(cli-extension PRIVATE ot-config)
add_dependencies(cli-extension zephyr_interface)

target_include_directories(cli-extension PUBLIC ${OT_PUBLIC_INCLUDES} PRIVATE ${COMMON_INCLUDES})

set(OT_CLI_VENDOR_TARGET cli-extension)
