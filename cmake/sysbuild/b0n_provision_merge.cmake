if(NOT CONFIG_PARTITION_MANAGER_ENABLED AND DEFINED SYSBUILD_NAME)
  function(zephyr_runner_file type path)
    # Property magic which makes west flash choose the signed build output of a given type.
    set_target_properties(runners_yaml_props_target PROPERTIES "${type}_file" "${path}")
  endfunction()

  zephyr_runner_file(hex ${CMAKE_BINARY_DIR}/../b0n_provision_merged.hex)
endif()
