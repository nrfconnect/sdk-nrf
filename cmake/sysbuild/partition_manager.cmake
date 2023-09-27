#
# Copyright (c) 2023 Nordic Semiconductor
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

# Load static configuration if found.
# Try user defined file first, then file found in configuration directory,
# finally file from board directory.

macro(add_region)
  set(oneValueArgs NAME SIZE BASE PLACEMENT DEVICE DEFAULT_DRIVER_KCONFIG DYNAMIC_PARTITION DOMAIN)
  cmake_parse_arguments(REGION "" "${oneValueArgs}" "" ${ARGN})
  if(DEFINED REGION_DOMAIN)
    set(underscore_domain ${REGION_DOMAIN}_)
  else()
    set(underscore_domain)
  endif()

  list(APPEND ${underscore_domain}regions ${REGION_NAME})
  list(APPEND ${underscore_domain}region_arguments "--${REGION_NAME}-size;${REGION_SIZE}")
  list(APPEND ${underscore_domain}region_arguments "--${REGION_NAME}-base-address;${REGION_BASE}")
  list(APPEND ${underscore_domain}region_arguments
    "--${REGION_NAME}-placement-strategy;${REGION_PLACEMENT}")
  if (REGION_DEVICE)
    list(APPEND ${underscore_domain}region_arguments "--${REGION_NAME}-device;${REGION_DEVICE}")
  list(APPEND ${underscore_domain}region_arguments
       "--${REGION_NAME}-default-driver-kconfig;${REGION_DEFAULT_DRIVER_KCONFIG}")
  endif()
  if (REGION_DYNAMIC_PARTITION)
    list(APPEND ${underscore_domain}region_arguments
      "--${REGION_NAME}-dynamic-partition;${REGION_DYNAMIC_PARTITION}")
  endif()
endmacro()

function(partition_manager)
  cmake_parse_arguments(PM "" "DOMAIN" "IN_FILES;REGIONS" ${ARGN})

  if(DEFINED PM_DOMAIN)
    get_property(image_name GLOBAL PROPERTY DOMAIN_APP_${PM_DOMAIN})
    set(user_def_pm_static ${${image_name}_PM_STATIC_YML_FILE})
  else()
    set(user_def_pm_static ${PM_STATIC_YML_FILE})
    get_property(image_name GLOBAL PROPERTY DOMAIN_APP_APP)
  endif()

  sysbuild_get(${image_name}_APPLICATION_CONFIG_DIR IMAGE ${image_name} VAR APPLICATION_CONFIG_DIR CACHE)

  ncs_file(CONF_FILES ${${image_name}_APPLICATION_CONFIG_DIR}
           PM conf_dir_pm_static
           DOMAIN ${DOMAIN}
           BUILD ${CONF_FILE_BUILD_TYPE}
  )

  ncs_file(CONF_FILES ${BOARD_DIR}
           PM board_dir_pm_static
           DOMAIN ${DOMAIN}
           BUILD ${CONF_FILE_BUILD_TYPE}
  )

  if(EXISTS "${user_def_pm_static}" AND NOT IS_DIRECTORY "${user_def_pm_static}")
    set(static_configuration_file ${user_def_pm_static})
  elseif (EXISTS ${conf_dir_pm_static})
    set(static_configuration_file ${conf_dir_pm_static})
  elseif (EXISTS ${board_dir_pm_static})
    set(static_configuration_file ${board_dir_pm_static})
  endif()

  if (EXISTS ${static_configuration_file})
    message(STATUS "Found partition manager static configuration ${PM_DOMAIN}: "
                   "${static_configuration_file}"
    )
    set(static_configuration --static-config ${static_configuration_file})
  endif()

  if (NOT static_configuration AND CONFIG_PM_IMAGE_NOT_BUILT_FROM_SOURCE)
    message(WARNING
      "One or more child image is not configured to be built from source. \
      However, there is no static configuration provided to the \
      partition manager. Please provide a static configuration as described in \
      the 'Scripts -> Partition Manager -> Static configuration' chapter in the \
      documentation. Without this information, the build system is not able to \
      place the image correctly in flash.")
  endif()


# We must set this when running for the domain, so how is the domain name partition handled in settings ?
  if("${PM_DOMAIN}" STREQUAL "CPUNET")
    set(dynamic_partition ${${PM_DOMAIN}_PM_DOMAIN_DYNAMIC_PARTITION})
    set(
      dynamic_partition_argument
      "--flash_primary-dynamic-partition;${dynamic_partition}"
      )
  endif()

  if (DEFINED PM_DOMAIN)
    set(underscore _)
  else()
    set(underscore)
  endif()

  set(pm_out_partition_file ${APPLICATION_BINARY_DIR}/partitions${underscore}${PM_DOMAIN}.yml)
  set(pm_out_region_file ${APPLICATION_BINARY_DIR}/regions${underscore}${PM_DOMAIN}.yml)
  set(pm_out_dotconf_file ${APPLICATION_BINARY_DIR}/pm${underscore}${PM_DOMAIN}.config)

  set(pm_cmd
    ${PYTHON_EXECUTABLE}
    ${ZEPHYR_NRF_MODULE_DIR}/scripts/partition_manager.py
    --input-files ${PM_IN_FILES}
    --regions ${PM_REGIONS}
    --output-partitions ${pm_out_partition_file}
    --output-regions ${pm_out_region_file}
    ${dynamic_partition_argument}
    ${static_configuration}
    ${${PM_DOMAIN}${underscore}region_arguments}     # region args are scoped in and thus available. Should probably be in arg.
    )

  set(pm_output_cmd
    ${PYTHON_EXECUTABLE}
    ${ZEPHYR_NRF_MODULE_DIR}/scripts/partition_manager_output.py
    --input-partitions ${pm_out_partition_file}
    --input-regions ${pm_out_region_file}
    --config-file ${pm_out_dotconf_file}
    )

  # Run the partition manager algorithm.
  execute_process(
    COMMAND
    ${pm_cmd}
    RESULT_VARIABLE ret
    )

  if(NOT ${ret} EQUAL "0")
    message(FATAL_ERROR "Partition Manager failed, aborting. Command: ${pm_cmd}")
  endif()

  # Produce header files and config file.
  execute_process(
    COMMAND
    ${pm_output_cmd}
    RESULT_VARIABLE ret
    )

  if(NOT ${ret} EQUAL "0")
    message(FATAL_ERROR "Partition Manager output generation failed, aborting. Command: ${pm_output_cmd}")
  endif()


  add_custom_target(partition_manager${underscore}${PM_DOMAIN})
  set(pm_var_names)
  import_pm_config(${pm_out_dotconf_file} pm_var_names)

  if(DEFINED PM_MCUBOOT_PAD_SIZE AND
     (NOT "${PM_MCUBOOT_PAD_SIZE}" STREQUAL "${SB_CONFIG_PM_MCUBOOT_PAD}")
  )
    message(WARNING "MCUboot padding partition size is ${PM_MCUBOOT_PAD_SIZE} "
            "but signing uses ${SB_CONFIG_PM_MCUBOOT_PAD}, please adjust "
            "`PM_MCUBOOT_PAD` value in sysbuild Kconfig to ${PM_MCUBOOT_PAD_SIZE}."
    )
  endif()

  foreach(name ${pm_var_names})
    set_property(
      TARGET partition_manager${underscore}${PM_DOMAIN}
      PROPERTY ${name}
      ${${name}}
      )
  endforeach()

  # Turn the space-separated list into a Cmake list.
  string(REPLACE " " ";" PM_ALL_BY_SIZE ${PM_ALL_BY_SIZE})

  # Iterate over every partition, from smallest to largest.
  foreach(part ${PM_ALL_BY_SIZE})
    if(${part} STREQUAL "app")
      if(DEFINED PM_DOMAIN)
        get_property(part GLOBAL PROPERTY DOMAIN_APP_${PM_DOMAIN})
      else()
        set(part "${DEFAULT_IMAGE}")
      endif()
    endif()
    string(TOUPPER ${part} PART)
    get_property(${part}_PM_HEX_FILE GLOBAL PROPERTY ${part}_PM_HEX_FILE)
    get_property(${part}_PM_ELF_FILE GLOBAL PROPERTY ${part}_PM_ELF_FILE)

    # Process container partitions (if it has a SPAN list it is a container partition).
    if(DEFINED PM_${PART}_SPAN)
      string(REPLACE " " ";" PM_${PART}_SPAN ${PM_${PART}_SPAN})
      list(APPEND containers ${part})
    endif()

    # Include the partition in the merge operation if it has a hex file.
    if(${part} IN_LIST IMAGES)
      # Question, what is it we want to know ?
      # We are now in sysbuild, meaning we know everything.
      # So for each domain we should just include the generated hex file.
      # Those are available thorugh sysbuild_get, but not locally as there is no parent image.
      list(APPEND explicitly_assigned ${part})
      sysbuild_get(${part}_PM_HEX_FILE IMAGE ${part} VAR BYPRODUCT_KERNEL_SIGNED_HEX_NAME CACHE)
      if(NOT ${part}_PM_HEX_FILE)
        sysbuild_get(${part}_PM_HEX_FILE IMAGE ${part} VAR BYPRODUCT_KERNEL_HEX_NAME CACHE)
      endif()
      set(${part}_PM_TARGET ${part})
    else()
      if(${part} IN_LIST containers)
        set_ifndef(${part}_PM_HEX_FILE ${PROJECT_BINARY_DIR}/${part}.hex)
        set_ifndef(${part}_PM_TARGET ${part}_hex)
      endif()
      list(APPEND implicitly_assigned ${part})
    endif()
  endforeach()

  if (DEFINED PM_DOMAIN)
    set(merged_suffix _${PM_DOMAIN})
    string(TOUPPER ${merged_suffix} MERGED_SUFFIX)
  endif()
  set(merged merged${merged_suffix})
  set(MERGED MERGED${MERGED_SUFFIX})

  set(PM_${MERGED}_SPAN ${implicitly_assigned} ${explicitly_assigned})
  set(${merged}_overlap TRUE) # Enable overlapping for the merged hex file.

  # Iterate over all container partitions, plus the "fake" merged paritition.
  # The loop will create a hex file for each iteration.
  foreach(container ${containers} ${merged})
    string(TOUPPER ${container} CONTAINER)

    # Prepare the list of hex files and list of dependencies for the merge command.
    foreach(part ${PM_${CONTAINER}_SPAN})
      string(TOUPPER ${part} PART)
      list(APPEND ${container}hex_files ${${part}_PM_HEX_FILE})
      list(APPEND ${container}elf_files ${${part}_PM_ELF_FILE})
      list(APPEND ${container}targets ${${part}_PM_TARGET})
    endforeach()

    # Do not merge hex files for empty partitions
    if(NOT ${container}hex_files)
      list(REMOVE_ITEM PM_${MERGED}_SPAN ${container})
      continue()
    endif()

    # If overlapping is enabled, add the appropriate argument.
    if(${${container}_overlap})
      set(${container}overlap_arg --overlap=replace)
    endif()

    # Should all files still be merged ?
    # And under which circumstances.
    # Add command to merge files.
    add_custom_command(
      OUTPUT ${PROJECT_BINARY_DIR}/${container}.hex
      COMMAND
      ${PYTHON_EXECUTABLE}
      ${ZEPHYR_BASE}/scripts/build/mergehex.py
      -o ${PROJECT_BINARY_DIR}/${container}.hex
      ${${container}overlap_arg}
      ${${container}hex_files}
      DEPENDS
      ${${container}targets}
      ${${container}hex_files}
      )

    # Wrapper target for the merge command.
    add_custom_target(
      ${container}_hex
      ALL DEPENDS
      ${PROJECT_BINARY_DIR}/${container}.hex
      )

    if (DEFINED PM_DOMAIN)
      get_property(image_name GLOBAL PROPERTY DOMAIN_APP_${PM_DOMAIN})
      update_runner(IMAGE ${image_name} HEX ${PROJECT_BINARY_DIR}/${container}.hex)
    endif()

    if ("${container}" STREQUAL "merged")
      update_runner(IMAGE ${DEFAULT_IMAGE} HEX ${PROJECT_BINARY_DIR}/${container}.hex)
    endif()
  endforeach()

endfunction()

# Function to update an already generated runners file for a Zephyr image.
# This function allows the build system to adjust files to flash in the config
# section of the runners.yaml on a given image.
function(update_runner)
  cmake_parse_arguments(RUNNER "" "IMAGE;HEX;BIN;ELF" "" ${ARGN})

  check_arguments_required("update_runner" RUNNER IMAGE)

  get_target_property(bin_dir ${RUNNER_IMAGE} _EP_BINARY_DIR)
  set(runners_file ${bin_dir}/zephyr/runners.yaml)

  set(runners_content_update)
  file(STRINGS ${runners_file} runners_content)
  foreach(line ${runners_content})
    if(DEFINED RUNNER_ELF AND ${line} MATCHES "^.*elf_file: .*$")
      string(REGEX REPLACE "(.*elf_file:) .*" "\\1 ${RUNNER_ELF}" line ${line})
    endif()

    if(DEFINED RUNNER_HEX AND ${line} MATCHES "^.*hex_file: .*$")
      string(REGEX REPLACE "(.*hex_file:) .*" "\\1 ${RUNNER_HEX}" line ${line})
    endif()

    if(DEFINED RUNNER_BIN AND ${line} MATCHES "^.*bin_file: .*$")
      string(REGEX REPLACE "(.*bin_file:) .*" "\\1 ${RUNNER_BIN}" line ${line})
    endif()
    list(APPEND runners_content_update "${line}\n")
  endforeach()
  file(WRITE ${runners_file} ${runners_content_update})
endfunction()


# APP is a special domain which is handled differently.
# Remove it from the list.
get_property(PM_DOMAINS GLOBAL PROPERTY PM_DOMAINS)
list(REMOVE_ITEM PM_DOMAINS APP)

## Check if current image is the dynamic partition in its domain.
## I.E. it is the only partition without a statically configured size in this
## domain. This is equivalent to the 'app' partition in the root domain.
##
## The dynamic partition is specified by the parent domain (i.e. the domain
## which creates the current domain through 'create_domain_image()'.
#if(DEFINED ${DOMAIN}_PM_DOMAIN_DYNAMIC_PARTITION
#   AND "${IMAGE_NAME}" STREQUAL "${${DOMAIN}_PM_DOMAIN_DYNAMIC_PARTITION}"
#)
#  set(is_dynamic_partition_in_domain TRUE)
#endif()

get_property(PM_IMAGES GLOBAL PROPERTY PM_IMAGES)
get_property(PM_SUBSYS_PREPROCESSED GLOBAL PROPERTY PM_SUBSYS_PREPROCESSED)

# This file is executed once per domain.
#
# It will be executed if one of the following criteria is true for the
# current image:
# - It's a child image, and is the dynamic partition in the domain
# - It's the root image, and a static configuration has been provided
# - It's the root image, and PM_IMAGES is populated.
# - It's the root image, and other domains exist.
# - A subsys has defined a partition and CONFIG_PM_SINGLE_IMAGE is set.
# Otherwise, return here
#if (NOT (
#  (IMAGE_NAME AND is_dynamic_partition_in_domain) OR
#  (NOT IMAGE_NAME AND static_configuration) OR
#  (NOT IMAGE_NAME AND PM_IMAGES) OR
#  (NOT IMAGE_NAME AND PM_DOMAINS) OR
#  (PM_SUBSYS_PREPROCESSED AND CONFIG_PM_SINGLE_IMAGE)
#  ))
#  return()
#endif()

# Set the dynamic partition. This is the only partition which does not
# have a statically defined size. There is only one dynamic partition per
# domain. For the "root domain" (ie the domain of the root image) this is
# always "app".
if (NOT is_dynamic_partition_in_domain)
  set(dynamic_partition "app")  # Should this be renamed to main image name, or does it matter at all ?
#  set(dynamic_partition "${DEFAULT_IMAGE}")  # Should this be renamed to main image name, or does it matter at all ?
else()
  set(dynamic_partition ${${DOMAIN}_PM_DOMAIN_DYNAMIC_PARTITION})
  set(
    dynamic_partition_argument
    "--flash_primary-dynamic-partition;${dynamic_partition}"
    )
endif()

# Image files, those should be queried from actual image.
# Add the dynamic partition as an image partition.
#set_property(GLOBAL PROPERTY
#  ${dynamic_partition}_PM_HEX_FILE
#  ${PROJECT_BINARY_DIR}/${KERNEL_HEX_NAME}
#  )
#
#set_property(GLOBAL PROPERTY
#  ${dynamic_partition}_PM_TARGET
#  ${logical_target_for_zephyr_elf}
#  )

# Prepare the input_files, header_files, and images lists
set(generated_path include/generated)
# ToDo: In child image, this happens to each domain parent.
# In parent image, it only happen to direct children, not children of children.
# This must be adjusted into:
# Explicitly add the main dynamic partition image
sysbuild_get(${DEFAULT_IMAGE}_input_files IMAGE ${DEFAULT_IMAGE} VAR PM_YML_FILES CACHE)
sysbuild_get(${DEFAULT_IMAGE}_binary_dir  IMAGE ${DEFAULT_IMAGE} VAR ZEPHYR_BINARY_DIR CACHE)
list(APPEND prefixed_images ":${dynamic_partition}")
list(APPEND input_files  ${${DEFAULT_IMAGE}_input_files})
list(APPEND header_files ${${DEFAULT_IMAGE}_binary_dir}/${generated_path}/pm_config.h)
foreach (image ${IMAGES})
  set(domain)
  # Special handling of `app_image` as this must be added as `:app` for historic reasons.
  # `:app` is handled below.
  foreach (d ${PM_DOMAINS})
    get_property(PM_${d}_IMAGES GLOBAL PROPERTY PM_${d}_IMAGES)
    if(${image} IN_LIST PM_${d}_IMAGES)
      set(domain ${d})
      break()
    endif()
  endforeach()

  if(NOT "${DEFAULT_IMAGE}" STREQUAL "${image}")
    sysbuild_get(${image}_input_files IMAGE ${image} VAR PM_YML_FILES CACHE)
    sysbuild_get(${image}_binary_dir  IMAGE ${image} VAR ZEPHYR_BINARY_DIR CACHE)

    list(APPEND prefixed_images ${domain}:${image})
    list(APPEND images ${image})
    list(APPEND header_files ${${image}_binary_dir}/${generated_path}/pm_config.h)
    get_property(domain_app GLOBAL PROPERTY DOMAIN_APP_${domain})
    if(NOT DEFINED domain OR "${domain_app}" STREQUAL "${image}")
      list(APPEND input_files  ${${image}_input_files})
    endif()
  endif()
endforeach()

#foreach (image ${IMAGES})
#    # Re-configure (Re-execute all CMakeLists.txt code) when original
#    # (not preprocessed) configuration file changes.
#  #  get_shared(dependencies IMAGE ${image} PROPERTY PM_YML_DEP_FILES)
#  #  set_property(
#  #    DIRECTORY APPEND PROPERTY
#  #    CMAKE_CONFIGURE_DEPENDS
#  #    ${dependencies}
#  #    )
#endforeach()

list(APPEND input_files  ${${DEFAULT_IMAGE}_binary_dir}/${generated_path}/pm.yml)

foreach (d ${PM_DOMAINS})
  get_property(PM_${d}_IMAGES GLOBAL PROPERTY PM_${d}_IMAGES)
  foreach (image ${PM_${d}_IMAGES})
    get_property(DOMAIN_APP_${d} GLOBAL PROPERTY DOMAIN_APP_${d})
    if(NOT "${DOMAIN_APP_${d}}" STREQUAL "${image}")
      list(APPEND ${d}_input_files  ${${image}_input_files})
      list(APPEND ${d}_header_files ${${image}_binary_dir}/${generated_path}/pm_config.h)
    endif()
  endforeach()

  # ToDo: Adjust into each image generated folder instead of this.
  list(APPEND ${d}_input_files ${PROJECT_BINARY_DIR}/${generated_path}/pm.yml)
  list(APPEND ${d}_header_files ${PROJECT_BINARY_DIR}/${generated_path}/pm_config.h)
endforeach()

# Add subsys defined pm.yml to the input_files
list(APPEND input_files ${PM_SUBSYS_PREPROCESSED})

foreach(d APP ${PM_DOMAINS})
  # CPUNET
  get_property(image_name GLOBAL PROPERTY DOMAIN_APP_${d})
  if(${d} STREQUAL "APP")
    set(d)
  endif()
  sysbuild_get(${image_name}_CONFIG_PM_SRAM_SIZE IMAGE ${image_name} VAR CONFIG_PM_SRAM_SIZE KCONFIG)
  sysbuild_get(${image_name}_CONFIG_PM_SRAM_BASE IMAGE ${image_name} VAR CONFIG_PM_SRAM_BASE KCONFIG)

  sysbuild_get(${image_name}_CONFIG_SOC_NRF9160 IMAGE ${image_name} VAR CONFIG_SOC_NRF9160 KCONFIG)
  sysbuild_get(${image_name}_CONFIG_SOC_NRF5340_CPUAPP IMAGE ${image_name} VAR CONFIG_SOC_NRF5340_CPUAPP KCONFIG)

  if (${image_name}_CONFIG_SOC_NRF9160)
    # See nRF9160 Product Specification, chapter "UICR"
    set(otp_start_addr "0xff8108")
    set(otp_size 756) # 189 * 4
  elseif (${image_name}_CONFIG_SOC_NRF5340_CPUAPP)
    # See nRF5340 Product Specification, chapter Application Core -> ... "UICR"
    set(otp_start_addr "0xff8100")
    set(otp_size 764)  # 191 * 4
  endif()

  add_region(
    NAME sram_primary
    SIZE ${${image_name}_CONFIG_PM_SRAM_SIZE}
    BASE ${${image_name}_CONFIG_PM_SRAM_BASE}
    PLACEMENT complex
    DYNAMIC_PARTITION sram_primary
    DOMAIN ${d}
    )

  sysbuild_get(${image_name}_CONFIG_FLASH_SIZE IMAGE ${image_name} VAR CONFIG_FLASH_SIZE KCONFIG)
  math(EXPR flash_size "${${image_name}_CONFIG_FLASH_SIZE} * 1024" OUTPUT_FORMAT HEXADECIMAL)

  if (${image_name}_CONFIG_SOC_NRF9160 OR ${image_name}_CONFIG_SOC_NRF5340_CPUAPP)
    add_region(
      NAME otp
      SIZE ${otp_size}
      BASE ${otp_start_addr}
      PLACEMENT start_to_end
      DOMAIN ${d}
      )
  endif()
  sysbuild_get(${image_name}_CONFIG_FLASH_BASE_ADDRESS IMAGE ${image_name} VAR CONFIG_FLASH_BASE_ADDRESS KCONFIG)
  add_region(
    NAME flash_primary
    SIZE ${flash_size}
    BASE ${${image_name}_CONFIG_FLASH_BASE_ADDRESS}
    PLACEMENT complex
    DEVICE flash_controller
    DEFAULT_DRIVER_KCONFIG CONFIG_SOC_FLASH_NRF
    DOMAIN ${d}
    )


endforeach()

sysbuild_get(ext_flash_enabled IMAGE ${DEFAULT_IMAGE} VAR CONFIG_PM_EXTERNAL_FLASH_ENABLED KCONFIG)
sysbuild_get(ext_flash_path IMAGE ${DEFAULT_IMAGE} VAR CONFIG_PM_EXTERNAL_FLASH_PATH KCONFIG)
sysbuild_get(num_bits IMAGE ${DEFAULT_IMAGE} VAR CONFIG_PM_EXTERNAL_FLASH_SIZE_BITS KCONFIG)

if(ext_flash_enabled)
  math(EXPR num_bytes "${num_bits} / 8")

  sysbuild_get(custom_driver IMAGE ${DEFAULT_IMAGE} VAR CONFIG_PM_OVERRIDE_EXTERNAL_DRIVER_CHECK KCONFIG)
  if (custom_driver)
    set(external_flash_driver_kconfig CONFIG_PM_OVERRIDE_EXTERNAL_DRIVER_CHECK)
  else()
    set(external_flash_driver_kconfig CONFIG_NORDIC_QSPI_NOR)
  endif()

  sysbuild_get(external_flash_base IMAGE ${DEFAULT_IMAGE} VAR CONFIG_PM_EXTERNAL_FLASH_BASE KCONFIG)
  add_region(
    NAME external_flash
    SIZE ${num_bytes}
    BASE ${external_flash_base}
    PLACEMENT start_to_end
    DEVICE "DT_CHOSEN(nordic_pm_ext_flash)"
    DEFAULT_DRIVER_KCONFIG ${external_flash_driver_kconfig}
    )
endif()

# Start - Code related to network core update. Multi image updates are part of NCSDK-17807
## If simultaneous updates of the network core and application core is supported
## we add a region which is used to emulate flash. In reality this data is being
## placed in RAM. This is used to bank the network core update in RAM while
## the application core update is banked in flash. This works since the nRF53
## application core has 512kB of RAM and the network core only has 256kB of flash
#get_shared(
#  mcuboot_NRF53_MULTI_IMAGE_UPDATE
#  IMAGE mcuboot
#  PROPERTY NRF53_MULTI_IMAGE_UPDATE
#  )
#
#get_shared(
#  mcuboot_NRF53_RECOVERY_NETWORK_CORE
#  IMAGE mcuboot
#  PROPERTY NRF53_RECOVERY_NETWORK_CORE
#  )
#
#if ((DEFINED mcuboot_NRF53_MULTI_IMAGE_UPDATE) OR (DEFINED mcuboot_NRF53_RECOVERY_NETWORK_CORE))
#  # This region will contain the 'mcuboot_secondary' partition, and the banked
#  # updates for the network core will be stored here.
#  get_shared(ram_flash_label IMAGE mcuboot PROPERTY RAM_FLASH_LABEL)
#  get_shared(ram_flash_addr IMAGE mcuboot PROPERTY RAM_FLASH_ADDR)
#  get_shared(ram_flash_size IMAGE mcuboot PROPERTY RAM_FLASH_SIZE)
#
#  add_region(
#    NAME ram_flash
#    SIZE ${ram_flash_size}
#    BASE ${ram_flash_addr}
#    PLACEMENT start_to_end
#    DEVICE ${ram_flash_label}
#    DEFAULT_DRIVER_KCONFIG CONFIG_FLASH_SIMULATOR
#    )
#endif()
# end - Code related to network core update. Multi image updates are part of NCSDK-17807

# Do per domain, end with main app domain.
partition_manager(IN_FILES ${input_files} REGIONS ${regions})
foreach(d ${PM_DOMAINS})
  get_property(image_name GLOBAL PROPERTY DOMAIN_APP_${d})
  partition_manager(DOMAIN ${d} IN_FILES ${${d}_input_files} REGIONS ${${d}_regions})

#  foreach(d ${PM_DOMAINS})
#    # Should list be adjust to domain image list ?
#    # In files must be adjust according to image being built/
#    partition_manager(DOMAIN ${d} IN_FILES ${${d}_input_files} REGIONS ${domain_regions})
#  endforeach()
endforeach()

# Start - Code related to network core update. Multi image updates are part of NCSDK-17807
#if (CONFIG_SECURE_BOOT AND CONFIG_BOOTLOADER_MCUBOOT)
#  # Create symbols for the offsets required for moving test update hex files
#  # to MCUBoots secondary slot. This is needed because objcopy does not
#  # support arithmetic expressions as argument (e.g. '0x100+0x200'), and all
#  # of the symbols used to generate the offset is only available as a
#  # generator expression when MCUBoots cmake code exectues. This because
#  # partition manager is performed as the last step in the configuration stage.
#  math(EXPR s0_offset "${PM_MCUBOOT_SECONDARY_ADDRESS} - ${PM_S0_ADDRESS}")
#  math(EXPR s1_offset "${PM_MCUBOOT_SECONDARY_ADDRESS} - ${PM_S1_ADDRESS}")
#
#  set_property(
#    TARGET partition_manager
#    PROPERTY s0_TO_SECONDARY
#    ${s0_offset}
#    )
#  set_property(
#    TARGET partition_manager
#    PROPERTY s1_TO_SECONDARY
#    ${s1_offset}
#    )
#endif()
# End - Code related to network core update. Multi image updates are part of NCSDK-17807

# Always add main partition file to list.
list(APPEND pm_out_partition_file ${APPLICATION_BINARY_DIR}/partitions.yml)
list(APPEND pm_out_region_file    ${APPLICATION_BINARY_DIR}/regions.yml)

if (is_dynamic_partition_in_domain)
  # Nothing is built as child.
  # We have all required info available, just need to use them.
  # We are being built as sub image.
  # Expose the generated partition manager configuration files to parent image.
  # This is used by the root image to create the global configuration in
  # pm_config.h.
  set_shared(IMAGE ${DOMAIN} PROPERTY PM_DOMAIN_PARTITIONS ${pm_out_partition_file})
  set_shared(IMAGE ${DOMAIN} PROPERTY PM_DOMAIN_REGIONS ${pm_out_region_file})
  set_shared(IMAGE ${DOMAIN} PROPERTY PM_DOMAIN_HEADER_FILES ${header_files})
  set_shared(IMAGE ${DOMAIN} PROPERTY PM_DOMAIN_IMAGES ${prefixed_images})
  set_shared(IMAGE ${DOMAIN} PROPERTY PM_HEX_FILE ${PROJECT_BINARY_DIR}/${merged}.hex)
  set_shared(IMAGE ${DOMAIN} PROPERTY PM_DOTCONF_FILES ${pm_out_dotconf_file})
  set_shared(IMAGE ${DOMAIN} PROPERTY PM_APP_HEX ${PROJECT_BINARY_DIR}/app.hex)
  set_shared(IMAGE ${IMAGE_NAME} APPEND PROPERTY BUILD_BYPRODUCTS ${PROJECT_BINARY_DIR}/${merged}.hex)
  if(CONFIG_SECURE_BOOT)
    # Only when secure boot is enabled the app will be signed.
    set_shared(IMAGE ${DOMAIN} PROPERTY PM_SIGNED_APP_HEX ${PROJECT_BINARY_DIR}/signed_by_b0_app.hex)
  endif()
else()
  list(REMOVE_DUPLICATES PM_DOMAINS)
  foreach (d ${PM_DOMAINS})
    # Don't include shared vars from own domain.
    if (NOT ("${DOMAIN}" STREQUAL "${d}"))
      get_shared(shared_header_files          IMAGE ${d} PROPERTY PM_DOMAIN_HEADER_FILES)
      get_shared(shared_prefixed_images       IMAGE ${d} PROPERTY PM_DOMAIN_IMAGES)
      get_shared(shared_pm_out_partition_file IMAGE ${d} PROPERTY PM_DOMAIN_PARTITIONS)
      get_shared(shared_pm_out_region_file    IMAGE ${d} PROPERTY PM_DOMAIN_REGIONS)
      get_shared(shared_domain_hex_files      IMAGE ${d} PROPERTY PM_HEX_FILE)

      list(APPEND header_files          ${shared_header_files})
      list(APPEND prefixed_images       ${shared_prefixed_images})
      list(APPEND pm_out_partition_file ${shared_pm_out_partition_file})
      list(APPEND pm_out_region_file    ${shared_pm_out_region_file})
      list(APPEND domain_hex_files      ${shared_domain_hex_files})
      list(APPEND global_hex_depends    ${${d}_PM_DOMAIN_DYNAMIC_PARTITION}_subimage)

      # Updated code.
      # All partition files are now created in this CMake invocation, so just add them:
      list(APPEND pm_out_partition_file ${APPLICATION_BINARY_DIR}/partitions_${d}.yml)
      list(APPEND pm_out_region_file    ${APPLICATION_BINARY_DIR}/regions_${d}.yml)

      # Add domain prefix cmake variables for all partitions
      # Here, we actually overwrite the already imported pm.config values
      # for our own domain. This is not an issue since all of these variables
      # are accessed through the 'partition_manager' target, and most likely
      # through generator expression, as this file is one of the last
      # cmake files executed in the configure stage.
      import_pm_config(${APPLICATION_BINARY_DIR}/pm_${d}.config ${d}_pm_var_names)

      foreach(name ${${d}_pm_var_names})
        set_property(
          TARGET partition_manager
          PROPERTY ${d}_${name}
          ${${name}}
          )
      endforeach()
    endif()
  endforeach()

# Start - Code related to network core update. Multi image updates are part of NCSDK-17807
#  if (CONFIG_BOOTLOADER_MCUBOOT)
#    # Create symbols for the offset required for moving the signed network
#    # core application to MCUBoots secondary slot. This is needed
#    # because  objcopy does not support arithmetic expressions as argument
#    # (e.g. '0x100+0x200'), and all of the symbols used to generate the
#    # offset are only available as a generator expression when MCUBoots
#    # cmake code executes.
#
#    # Check if a signed version of the network core application is defined.
#    # If so, this indicates that we need to support firmware updates on the
#    # network core. This again means that we should generate the required
#    # hex files.
#    get_shared(cpunet_signed_app_hex IMAGE CPUNET PROPERTY PM_SIGNED_APP_HEX)
#
#    if (CONFIG_NRF53_UPGRADE_NETWORK_CORE
#        AND DEFINED cpunet_signed_app_hex)
#      # The address coming from other domains are not available in this scope
#      # since it is imported by a different domain. Hence, it must be fetched
#      # through the 'partition_manager' target.
#      get_target_property(net_app_addr partition_manager CPUNET_PM_APP_ADDRESS)
#
#      get_shared(
#        mcuboot_NRF53_MULTI_IMAGE_UPDATE
#        IMAGE mcuboot
#        PROPERTY NRF53_MULTI_IMAGE_UPDATE
#        )
#
#      # Check if multi image updates are enabled, in which case we need
#      # to use the "_1" variant of the secondary partition for the network core.
#      if(DEFINED mcuboot_NRF53_MULTI_IMAGE_UPDATE)
#        set(sec_slot_idx "_1")
#      endif()
#
#      # Calculate the offset from the address which the net/app core app is linked
#      # against to the secondary slot. We need these values to generate hex files
#      # which targets the secondary slot.
#      math(EXPR net_app_to_secondary
#        "${xip_addr} \
#        + ${PM_MCUBOOT_SECONDARY${sec_slot_idx}_ADDRESS} \
#        - ${net_app_addr} \
#        + ${PM_MCUBOOT_PAD_SIZE}"
#        )
#
#      set_property(
#        TARGET partition_manager
#        PROPERTY net_app_TO_SECONDARY
#        ${net_app_to_secondary}
#        )
#
#      # This value is needed by `imgtool.py` which is used to sign the images.
#      set_property(
#        TARGET partition_manager
#        PROPERTY net_app_slot_size
#        ${PM_MCUBOOT_SECONDARY${sec_slot_idx}_SIZE}
#        )
#    endif()
#
#    math(EXPR app_to_secondary
#      "${xip_addr} \
#      + ${PM_MCUBOOT_SECONDARY_ADDRESS} \
#      - ${PM_MCUBOOT_PRIMARY_ADDRESS}"
#      )
#
#    set_property(
#      TARGET partition_manager
#      PROPERTY app_TO_SECONDARY
#      ${app_to_secondary}
#      )
#  endif()
# End - Code related to network core update. Multi image updates are part of NCSDK-17807

  # Explicitly add the root image domain hex file to the list
  list(APPEND domain_hex_files ${PROJECT_BINARY_DIR}/${merged}.hex)
  list(APPEND global_hex_depends ${merged}_hex)

  # Now all partition manager configuration from all images and domains are
  # available. Generate the global pm_config.h, and provide it to all images.
  set(pm_global_output_cmd
    ${PYTHON_EXECUTABLE}
    ${ZEPHYR_NRF_MODULE_DIR}/scripts/partition_manager_output.py
    --input-partitions ${pm_out_partition_file}
    --input-regions ${pm_out_region_file}
    --header-files ${header_files}
    --images ${prefixed_images}
    )

  execute_process(
    COMMAND
    ${pm_global_output_cmd}
    RESULT_VARIABLE ret
    )

  if(NOT ${ret} EQUAL "0")
    message(FATAL_ERROR "Partition Manager GLOBAL output generation failed,
    aborting. Command: ${pm_global_output_cmd}")
  endif()

  set_property(
    TARGET partition_manager
    PROPERTY PM_CONFIG_FILES
    ${pm_out_partition_file}
    )

  set_property(
    TARGET partition_manager
    PROPERTY PM_DEPENDS
    ${global_hex_depends}
    )

  add_custom_target(
    partition_manager_report
    COMMAND
    ${PYTHON_EXECUTABLE}
    ${ZEPHYR_NRF_MODULE_DIR}/scripts/partition_manager_report.py
    --input ${pm_out_partition_file}
    COMMAND_EXPAND_LISTS
    )

# ToDo: do we still want to merge hex files for all the domains ?
#  if (PM_DOMAINS)
#    # For convenience, generate global hex file containing all domains' hex
#    # files.
#    set(final_merged ${PROJECT_BINARY_DIR}/merged_domains.hex)
#
#    # Add command to merge files.
#    add_custom_command(
#      OUTPUT ${final_merged}
#      COMMAND
#      ${PYTHON_EXECUTABLE}
#      ${ZEPHYR_BASE}/scripts/build/mergehex.py
#      -o ${final_merged}
#      ${domain_hex_files}
#      DEPENDS
#      ${domain_hex_files}
#      ${global_hex_depends}
#      )
#
#    # Wrapper target for the merge command.
#    add_custom_target(merged_domains_hex ALL DEPENDS ${final_merged})
#  endif()

  set(ZEPHYR_RUNNER_CONFIG_KERNEL_HEX "${final_merged}"
    CACHE STRING "Path to merged image in Intel Hex format" FORCE)

endif()
