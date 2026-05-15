if(NOT CONFIG_PARTITION_MANAGER_ENABLED)
  function(zephyr_runner_file type path)
    # Property magic which makes west flash choose the signed build output of a given type.
    set_target_properties(runners_yaml_props_target PROPERTIES "${type}_file" "${path}")
  endfunction()

  if(CONFIG_SECURE_BOOT)
    if(CONFIG_BOOTLOADER_MCUBOOT)
      zephyr_runner_file(hex ${CMAKE_BINARY_DIR}/../signed_by_mcuboot_and_b0_${SYSBUILD_NAME}.hex)
    else()
      zephyr_runner_file(hex ${CMAKE_BINARY_DIR}/../signed_by_b0_${SYSBUILD_NAME}.hex)
    endif()
  endif()
endif()
