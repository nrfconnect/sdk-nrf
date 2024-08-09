#
# Copyright (c) 2019 Nordic Semiconductor
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

macro(add_region)
  set(oneValueArgs NAME SIZE BASE PLACEMENT DEVICE DEFAULT_DRIVER_KCONFIG DYNAMIC_PARTITION)
  cmake_parse_arguments(REGION "" "${oneValueArgs}" "" ${ARGN})
  list(APPEND regions ${REGION_NAME})
  list(APPEND region_arguments "--${REGION_NAME}-size;${REGION_SIZE}")
  list(APPEND region_arguments "--${REGION_NAME}-base-address;${REGION_BASE}")
  list(APPEND region_arguments
    "--${REGION_NAME}-placement-strategy;${REGION_PLACEMENT}")
  if (REGION_DEVICE)
    list(APPEND region_arguments "--${REGION_NAME}-device;${REGION_DEVICE}")
  list(APPEND region_arguments
       "--${REGION_NAME}-default-driver-kconfig;${REGION_DEFAULT_DRIVER_KCONFIG}")
  endif()
  if (REGION_DYNAMIC_PARTITION)
    list(APPEND region_arguments
      "--${REGION_NAME}-dynamic-partition;${REGION_DYNAMIC_PARTITION}")
  endif()
endmacro()

# Load static configuration if found.
# Try user defined file first, then file found in configuration directory,
# finally file from board directory.
if(SYSBUILD)
  zephyr_get(PM_STATIC_YML_FILE SYSBUILD GLOBAL)
elseif(CONFIG_PARTITION_MANAGER_ENABLED)
  message(DEPRECATION "
          ---------------------------------------------------------------------
          --- WARNING: Child and parent image functionality is deprecated   ---
          --- and should be replaced with sysbuild. Child and parent image  ---
          --- support remains only to allow existing customer applications  ---
          --- to build and allow porting to sysbuild, it is no longer       ---
          --- receiving updates or new features and it will not be possible ---
          --- to build using child/parent image at all in nRF Connect SDK   ---
          --- version 2.9 onwards.                                          ---
          ---------------------------------------------------------------------"
         )
endif()

if(DEFINED PM_STATIC_YML_FILE)
  string(CONFIGURE "${PM_STATIC_YML_FILE}" user_def_pm_static)
endif()

zephyr_get(COMMON_CHILD_IMAGE_CONFIG_DIR)
string(CONFIGURE "${COMMON_CHILD_IMAGE_CONFIG_DIR}" COMMON_CHILD_IMAGE_CONFIG_DIR)
foreach(config_dir ${APPLICATION_CONFIG_DIR} ${COMMON_CHILD_IMAGE_CONFIG_DIR})
  ncs_file(CONF_FILES ${config_dir}
           PM conf_dir_pm_static
           DOMAIN ${DOMAIN}
           BUILD ${CONF_FILE_BUILD_TYPE}
  )
  if(EXISTS ${conf_dir_pm_static})
    break()
  endif()
endforeach()

ncs_file(CONF_FILES ${BOARD_DIR}
         PM board_dir_pm_static
         DOMAIN ${DOMAIN}
         BUILD ${CONF_FILE_BUILD_TYPE}
)

ncs_file(CONF_FILES ${BOARD_DIR}
         PM board_dir_pm_static_common
         DOMAIN ${DOMAIN}
)

if(EXISTS "${user_def_pm_static}" AND NOT IS_DIRECTORY "${user_def_pm_static}")
  set(static_configuration_file ${user_def_pm_static})
elseif (EXISTS ${conf_dir_pm_static})
  set(static_configuration_file ${conf_dir_pm_static})
elseif (EXISTS ${board_dir_pm_static})
  set(static_configuration_file ${board_dir_pm_static})
elseif (EXISTS ${board_dir_pm_static_common})
  set(static_configuration_file ${board_dir_pm_static_common})
endif()

if (EXISTS "${static_configuration_file}" AND NOT SYSBUILD)
  message(STATUS "Found partition manager static configuration: "
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

if (NOT static_configuration AND
  (CONFIG_BOOTLOADER_MCUBOOT OR CONFIG_SECURE_BOOT) AND NOT SYSBUILD)
      message(WARNING "
        ---------------------------------------------------------------------
        --- WARNING: Using a bootloader without pm_static.yml.            ---
        --- There are cases where a deployed product can consist of       ---
        --- multiple images, and only a subset of these images can be     ---
        --- upgraded through a firmware update mechanism. In such cases,  ---
        --- the upgradable images must have partitions that are static    ---
        --- and are matching the partition map used by the bootloader     ---
        --- programmed onto the device.                                   ---
        ---------------------------------------------------------------------
        \n"
      )
endif()


# Check if current image is the dynamic partition in its domain.
# I.E. it is the only partition without a statically configured size in this
# domain. This is equivalent to the 'app' partition in the root domain.
#
# The dynamic partition is specified by the parent domain (i.e. the domain
# which creates the current domain through 'create_domain_image()'.
if(DEFINED ${DOMAIN}_PM_DOMAIN_DYNAMIC_PARTITION
   AND "${IMAGE_NAME}" STREQUAL "${${DOMAIN}_PM_DOMAIN_DYNAMIC_PARTITION}"
)
  set(is_dynamic_partition_in_domain TRUE)
endif()

get_property(PM_IMAGES GLOBAL PROPERTY PM_IMAGES)
get_property(PM_SUBSYS_PREPROCESSED GLOBAL PROPERTY PM_SUBSYS_PREPROCESSED)
get_property(PM_DOMAINS GLOBAL PROPERTY PM_DOMAINS)

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
if (NOT (
  (IMAGE_NAME AND is_dynamic_partition_in_domain) OR
  (NOT IMAGE_NAME AND static_configuration) OR
  (NOT IMAGE_NAME AND PM_IMAGES) OR
  (NOT IMAGE_NAME AND PM_DOMAINS) OR
  (PM_SUBSYS_PREPROCESSED AND CONFIG_PM_SINGLE_IMAGE)
  )
  OR SYSBUILD
  )
  return()
endif()

# Set the dynamic partition. This is the only partition which does not
# have a statically defined size. There is only one dynamic partition per
# domain. For the "root domain" (ie the domain of the root image) this is
# always "app".
if (NOT is_dynamic_partition_in_domain)
  set(dynamic_partition "app")
else()
  set(dynamic_partition ${${DOMAIN}_PM_DOMAIN_DYNAMIC_PARTITION})
  set(
    dynamic_partition_argument
    "--flash_primary-dynamic-partition;${dynamic_partition}"
    )
endif()

# Check if the dynamic partition image hex has already been defined
get_property(DYNAMIC_PARTITION_HEX GLOBAL PROPERTY
  ${dynamic_partition}_PM_HEX_FILE
  )
if (NOT DYNAMIC_PARTITION_HEX)
  # Add the dynamic partition as an image partition.
  set_property(GLOBAL PROPERTY
    ${dynamic_partition}_PM_HEX_FILE
    ${PROJECT_BINARY_DIR}/${KERNEL_HEX_NAME}
    )
endif()

get_property(DYNAMIC_PARTITION_TARGET GLOBAL PROPERTY
  ${dynamic_partition}_PM_TARGET
  )
if (NOT DYNAMIC_PARTITION_TARGET)
  set_property(GLOBAL PROPERTY
    ${dynamic_partition}_PM_TARGET
    ${logical_target_for_zephyr_elf}
    )
endif()

# Prepare the input_files, header_files, and images lists
set(generated_path include/generated)
foreach (image ${PM_IMAGES})
  list(APPEND prefixed_images ${DOMAIN}:${image})
  list(APPEND images ${image})

  get_shared(${image}_input_files IMAGE ${image} PROPERTY PM_YML_FILES)
  get_shared(${image}_binary_dir  IMAGE ${image} PROPERTY ZEPHYR_BINARY_DIR)

  list(APPEND input_files  ${${image}_input_files})
  list(APPEND header_files ${${image}_binary_dir}/${generated_path}/pm_config.h)

  # Re-configure (Re-execute all CMakeLists.txt code) when original
  # (not preprocessed) configuration file changes.
  get_shared(dependencies IMAGE ${image} PROPERTY PM_YML_DEP_FILES)
  set_property(
    DIRECTORY APPEND PROPERTY
    CMAKE_CONFIGURE_DEPENDS
    ${dependencies}
    )
endforeach()

# Explicitly add the dynamic partition image
list(APPEND prefixed_images "${DOMAIN}:${dynamic_partition}")
list(APPEND images ${dynamic_partition})
list(APPEND input_files ${ZEPHYR_BINARY_DIR}/${generated_path}/pm.yml)
list(APPEND header_files ${ZEPHYR_BINARY_DIR}/${generated_path}/pm_config.h)

# Add subsys defined pm.yml to the input_files
list(APPEND input_files ${PM_SUBSYS_PREPROCESSED})

if (DEFINED CONFIG_SOC_SERIES_NRF91X)
  # See nRF91 Product Specification, chapter "UICR"
  set(otp_start_addr "0xff8108")
  set(otp_size 756) # 189 * 4
elseif (DEFINED CONFIG_SOC_NRF5340_CPUAPP)
  # See nRF5340 Product Specification, chapter Application Core -> ... "UICR"
  set(otp_start_addr "0xff8100")
  set(otp_size 764)  # 191 * 4
endif()

if (DEFINED CONFIG_SOC_SERIES_NRF54LX)
  set(soc_nvs_controller rram_controller)
  set(soc_nvs_controller_driver_kc CONFIG_SOC_FLASH_NRF_RRAM)
else()
  set(soc_nvs_controller flash_controller)
  set(soc_nvs_controller_driver_kc CONFIG_SOC_FLASH_NRF)
endif()

add_region(
  NAME sram_primary
  SIZE ${CONFIG_PM_SRAM_SIZE}
  BASE ${CONFIG_PM_SRAM_BASE}
  PLACEMENT complex
  DYNAMIC_PARTITION sram_primary
  )

math(EXPR flash_size "${CONFIG_FLASH_SIZE} * 1024" OUTPUT_FORMAT HEXADECIMAL)

if (CONFIG_SOC_SERIES_NRF91X OR CONFIG_SOC_NRF5340_CPUAPP)
  add_region(
    NAME otp
    SIZE ${otp_size}
    BASE ${otp_start_addr}
    PLACEMENT start_to_end
    )
endif()
add_region(
  NAME flash_primary
  SIZE ${flash_size}
  BASE ${CONFIG_FLASH_BASE_ADDRESS}
  PLACEMENT complex
  DEVICE ${soc_nvs_controller}
  DEFAULT_DRIVER_KCONFIG ${soc_nvs_controller_driver_kc}
  )

dt_chosen(ext_flash_dev PROPERTY nordic,pm-ext-flash)
if (DEFINED ext_flash_dev)
  dt_prop(num_bits PATH ${ext_flash_dev} PROPERTY size)
  math(EXPR num_bytes "${num_bits} / 8")

  set(external_flash_driver_kconfig CONFIG_PM_EXTERNAL_FLASH_HAS_DRIVER)

  add_region(
    NAME external_flash
    SIZE ${num_bytes}
    BASE ${CONFIG_PM_EXTERNAL_FLASH_BASE}
    PLACEMENT start_to_end
    DEVICE "DT_CHOSEN(nordic_pm_ext_flash)"
    DEFAULT_DRIVER_KCONFIG ${external_flash_driver_kconfig}
    )
endif()

# If simultaneous updates of the network core and application core is supported
# we add a region which is used to emulate flash. In reality this data is being
# placed in RAM. This is used to bank the network core update in RAM while
# the application core update is banked in flash. This works since the nRF53
# application core has 512kB of RAM and the network core only has 256kB of flash
get_shared(
  mcuboot_NRF53_MULTI_IMAGE_UPDATE
  IMAGE mcuboot
  PROPERTY NRF53_MULTI_IMAGE_UPDATE
  )

get_shared(
  mcuboot_NRF53_RECOVERY_NETWORK_CORE
  IMAGE mcuboot
  PROPERTY NRF53_RECOVERY_NETWORK_CORE
  )

if ((DEFINED mcuboot_NRF53_MULTI_IMAGE_UPDATE) OR (DEFINED mcuboot_NRF53_RECOVERY_NETWORK_CORE))
  # This region will contain the 'mcuboot_secondary' partition, and the banked
  # updates for the network core will be stored here.
  get_shared(ram_flash_label IMAGE mcuboot PROPERTY RAM_FLASH_LABEL)
  get_shared(ram_flash_addr IMAGE mcuboot PROPERTY RAM_FLASH_ADDR)
  get_shared(ram_flash_size IMAGE mcuboot PROPERTY RAM_FLASH_SIZE)

  add_region(
    NAME ram_flash
    SIZE ${ram_flash_size}
    BASE ${ram_flash_addr}
    PLACEMENT start_to_end
    DEVICE ${ram_flash_label}
    DEFAULT_DRIVER_KCONFIG CONFIG_FLASH_SIMULATOR
    )
endif()

if (DOMAIN)
  set(UNDERSCORE_DOMAIN _${DOMAIN})
endif()

set(pm_out_partition_file ${APPLICATION_BINARY_DIR}/partitions${UNDERSCORE_DOMAIN}.yml)
set(pm_out_region_file ${APPLICATION_BINARY_DIR}/regions${UNDERSCORE_DOMAIN}.yml)
set(pm_out_dotconf_file ${APPLICATION_BINARY_DIR}/pm${UNDERSCORE_DOMAIN}.config)

set(pm_cmd
  ${PYTHON_EXECUTABLE}
  ${ZEPHYR_NRF_MODULE_DIR}/scripts/partition_manager.py
  --input-files ${input_files}
  --regions ${regions}
  --output-partitions ${pm_out_partition_file}
  --output-regions ${pm_out_region_file}
  ${dynamic_partition_argument}
  ${static_configuration}
  ${region_arguments}
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

# Create a dummy target that we can add properties to for
# extraction in generator expressions.
add_custom_target(partition_manager)

# Make Partition Manager configuration available in CMake
import_pm_config(${pm_out_dotconf_file} pm_var_names)

foreach(name ${pm_var_names})
  set_property(
    TARGET partition_manager
    PROPERTY ${name}
    ${${name}}
    )
endforeach()

# Turn the space-separated list into a Cmake list.
string(REPLACE " " ";" PM_ALL_BY_SIZE ${PM_ALL_BY_SIZE})

# Iterate over every partition, from smallest to largest.
foreach(part ${PM_ALL_BY_SIZE})
  string(TOUPPER ${part} PART)
  get_property(${part}_PM_HEX_FILE GLOBAL PROPERTY ${part}_PM_HEX_FILE)
  get_property(${part}_PM_ELF_FILE GLOBAL PROPERTY ${part}_PM_ELF_FILE)

  # Process container partitions (if it has a SPAN list it is a container partition).
  if(DEFINED PM_${PART}_SPAN)
    string(REPLACE " " ";" PM_${PART}_SPAN ${PM_${PART}_SPAN})
    list(APPEND containers ${part})
  endif()

  # Include the partition in the merge operation if it has a hex file.
  if(DEFINED ${part}_PM_HEX_FILE)
    get_property(${part}_PM_TARGET GLOBAL PROPERTY ${part}_PM_TARGET)
    list(APPEND explicitly_assigned ${part})
  else()
    if(${part} IN_LIST images)
      get_shared(${part}_bin_dir  IMAGE ${part} PROPERTY ZEPHYR_BINARY_DIR)
      get_shared(${part}_hex_file IMAGE ${part} PROPERTY KERNEL_HEX_NAME)
      get_shared(${part}_elf_file IMAGE ${part} PROPERTY KERNEL_ELF_NAME)
      set(${part}_PM_HEX_FILE ${${part}_bin_dir}/${${part}_hex_file})
      set(${part}_PM_ELF_FILE ${${part}_bin_dir}/${${part}_elf_file})
      set(${part}_PM_TARGET ${part}_subimage)
    elseif(${part} IN_LIST containers)
      set(${part}_PM_HEX_FILE ${PROJECT_BINARY_DIR}/${part}.hex)
      set(${part}_PM_TARGET ${part}_hex)
    endif()
    list(APPEND implicitly_assigned ${part})
  endif()
endforeach()

if (${is_dynamic_partition_in_domain})
  set(merged_suffix _${DOMAIN})
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
    # SES will load symbols from all elf files listed as dependencies to
    # ${PROJECT_BINARY_DIR}/merged.hex. Therefore we add
    # ${${container}elf_files} as dependency to ensure they are loaded by SES
    # even though it is unnecessary for building the application.
    ${${container}elf_files}
    )

  # Wrapper target for the merge command.
  add_custom_target(
    ${container}_hex
    ALL DEPENDS
    ${PROJECT_BINARY_DIR}/${container}.hex
    )

endforeach()

if (CONFIG_SECURE_BOOT AND CONFIG_BOOTLOADER_MCUBOOT)
  # Create symbols for the offsets required for moving test update hex files
  # to MCUBoots secondary slot. This is needed because objcopy does not
  # support arithmetic expressions as argument (e.g. '0x100+0x200'), and all
  # of the symbols used to generate the offset is only available as a
  # generator expression when MCUBoots cmake code exectues. This because
  # partition manager is performed as the last step in the configuration stage.
  if(CONFIG_PM_EXTERNAL_FLASH_MCUBOOT_SECONDARY AND CONFIG_HAS_HW_NRF_QSPI)
    if(DEFINED ext_flash_dev)
      get_filename_component(qspi_node ${ext_flash_dev} DIRECTORY)
    else()
      dt_nodelabel(qspi_node NODELABEL "qspi")
    endif()
    if(DEFINED qspi_node)
      dt_reg_addr(xip_addr PATH ${qspi_node} NAME qspi_mm)
      if(NOT DEFINED xip_addr)
        message(WARNING "\
        Could not find memory mapped address for XIP. Generated update hex files will \
        not have the correct base address. Hence they can not be programmed directly \
        to the external flash")
      endif()
    endif()
  else()
    set(xip_addr 0)
  endif()

  math(EXPR s0_offset "${xip_addr} + ${PM_MCUBOOT_SECONDARY_ADDRESS} - ${PM_S0_ADDRESS}")
  math(EXPR s1_offset "${xip_addr} + ${PM_MCUBOOT_SECONDARY_ADDRESS} - ${PM_S1_ADDRESS}")

  set_property(
    TARGET partition_manager
    PROPERTY s0_TO_SECONDARY
    ${s0_offset}
    )
  set_property(
    TARGET partition_manager
    PROPERTY s1_TO_SECONDARY
    ${s1_offset}
    )
endif()

# Calculate absolute address for the wi-fi firmware patch location.
if (CONFIG_WIFI_NRF70 AND CONFIG_NRF_WIFI_PATCHES_EXT_FLASH_STORE)
  if(DEFINED ext_flash_dev)
    get_filename_component(qspi_node ${ext_flash_dev} DIRECTORY)
  else()
    dt_nodelabel(qspi_node NODELABEL "qspi")
  endif()
  if(DEFINED qspi_node)
    dt_reg_addr(xip_addr PATH ${qspi_node} NAME qspi_mm)
    if(NOT DEFINED xip_addr)
      message(WARNING "\
      Could not find memory mapped address for XIP. Generated update hex files will \
      not have the correct base address. Hence they can not be programmed directly \
      to the external flash")
    endif()
  else()
    set(xip_addr 0)
  endif()

  math(EXPR wifi_fw_abs_addr "${xip_addr} + ${PM_NRF70_WIFI_FW_OFFSET}")
  set_property(
    TARGET partition_manager
    PROPERTY nrf70_wifi_fw_XIP_ABS_ADDR
    ${wifi_fw_abs_addr}
    )
endif()

if (is_dynamic_partition_in_domain)
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
  # This is the root image, generate the global pm_config.h
  # First, include the shared properties for all child images.
  if (PM_DOMAINS)
    # We ensure the existence of PM_DOMAINS to support older cmake versions.
    # When version >= 3.17 is required this check can be removed.
    list(REMOVE_DUPLICATES PM_DOMAINS)
  endif()
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

      # Add domain prefix cmake variables for all partitions
      # Here, we actually overwrite the already imported pm.config values
      # for our own domain. This is not an issue since all of these variables
      # are accessed through the 'partition_manager' target, and most likely
      # through generator expression, as this file is one of the last
      # cmake files executed in the configure stage.
      get_shared(conf_file IMAGE ${d} PROPERTY PM_DOTCONF_FILES)
      import_pm_config(${conf_file} ${d}_pm_var_names)

      foreach(name ${${d}_pm_var_names})
        set_property(
          TARGET partition_manager
          PROPERTY ${d}_${name}
          ${${name}}
          )
      endforeach()
    endif()
  endforeach()

  if (CONFIG_BOOTLOADER_MCUBOOT)
    if (CONFIG_PM_EXTERNAL_FLASH_MCUBOOT_SECONDARY AND CONFIG_HAS_HW_NRF_QSPI)
      # First we see if an ext flash dev has been chosen, if not, then we look
      # up the 'qspi' node and assume that this has the required address.
      if (DEFINED ext_flash_dev)
        get_filename_component(qspi_node ${ext_flash_dev} DIRECTORY)
      else()
        dt_nodelabel(qspi_node NODELABEL "qspi")
      endif()

      # If the qspi node is still not defined we are building on a platform
      # which does not have the qspi peripheral, in which case no hex files
      # will be generated for the secondary slot.
      if(DEFINED qspi_node)
        dt_reg_addr(xip_addr PATH ${qspi_node} NAME qspi_mm)
        if(NOT DEFINED xip_addr)
          message(WARNING "\
Could not find memory mapped address for XIP. Generated update hex files will \
not have the correct base address. Hence they can not be programmed directly \
to the external flash")
        endif()
      endif()
    endif()

    # Create symbols for the offset required for moving the signed network
    # core application to MCUBoots secondary slot. This is needed
    # because  objcopy does not support arithmetic expressions as argument
    # (e.g. '0x100+0x200'), and all of the symbols used to generate the
    # offset are only available as a generator expression when MCUBoots
    # cmake code executes.

    # Check if a signed version of the network core application is defined.
    # If so, this indicates that we need to support firmware updates on the
    # network core. This again means that we should generate the required
    # hex files.
    get_shared(cpunet_signed_app_hex IMAGE CPUNET PROPERTY PM_SIGNED_APP_HEX)

    if (CONFIG_NRF53_UPGRADE_NETWORK_CORE
        AND DEFINED cpunet_signed_app_hex)
      # The address coming from other domains are not available in this scope
      # since it is imported by a different domain. Hence, it must be fetched
      # through the 'partition_manager' target.
      get_target_property(net_app_addr partition_manager CPUNET_PM_APP_ADDRESS)

      get_shared(
        mcuboot_NRF53_MULTI_IMAGE_UPDATE
        IMAGE mcuboot
        PROPERTY NRF53_MULTI_IMAGE_UPDATE
        )

      # Check if multi image updates are enabled, in which case we need
      # to use the "_1" variant of the secondary partition for the network core.
      if(DEFINED mcuboot_NRF53_MULTI_IMAGE_UPDATE)
        set(sec_slot_idx "_1")
      endif()

      # Calculate the offset from the address which the net/app core app is linked
      # against to the secondary slot. We need these values to generate hex files
      # which targets the secondary slot.
      math(EXPR net_app_to_secondary
        "${xip_addr} \
        + ${PM_MCUBOOT_SECONDARY${sec_slot_idx}_ADDRESS} \
        - ${net_app_addr} \
        + ${PM_MCUBOOT_PAD_SIZE}"
        )

      set_property(
        TARGET partition_manager
        PROPERTY net_app_TO_SECONDARY
        ${net_app_to_secondary}
        )

      # This value is needed by `imgtool.py` which is used to sign the images.
      set_property(
        TARGET partition_manager
        PROPERTY net_app_slot_size
        ${PM_MCUBOOT_SECONDARY${sec_slot_idx}_SIZE}
        )
    endif()

    math(EXPR app_to_secondary
      "${xip_addr} \
      + ${PM_MCUBOOT_SECONDARY_ADDRESS} \
      - ${PM_MCUBOOT_PRIMARY_ADDRESS}"
      )

    set_property(
      TARGET partition_manager
      PROPERTY app_TO_SECONDARY
      ${app_to_secondary}
      )
  endif()

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

  if (PM_DOMAINS)
    # For convenience, generate global hex file containing all domains' hex
    # files.
    set(merged_domains  merged_domains)
    set(final_merged ${ZEPHYR_BINARY_DIR}/${merged_domains}.hex)

    # Add command to merge files.
    add_custom_command(
      OUTPUT ${final_merged}
      COMMAND
      ${PYTHON_EXECUTABLE}
      ${ZEPHYR_BASE}/scripts/build/mergehex.py
      -o ${final_merged}
      ${domain_hex_files}
      DEPENDS
      ${domain_hex_files}
      ${global_hex_depends}
      )

    # Wrapper target for the merge command.
    add_custom_target(merged_domains_hex ALL DEPENDS ${final_merged})
  endif()

  set(ZEPHYR_RUNNER_CONFIG_KERNEL_HEX "${final_merged}"
    CACHE STRING "Path to merged image in Intel Hex format" FORCE)

endif()

# We need to tell the flash runner use the merged hex file instead of
# 'zephyr.hex'This is typically done by setting the 'hex_file' property of the
# 'runners_yaml_props_target' target. However, since the CMakeLists.txt file
# reading those properties has already run, and the 'hex_file' property
# is not evaluated in a generator expression, it is too late at this point to
# set that variable. Hence we must operate on the 'yaml_contents' property,
# which is evaluated in a generator expression.

if (merged_domains)
  # Multiple domains are included in the build, point to the result of
  # merging the merged hex file for all domains.
  set(merged_hex_to_flash ${merged_domains}.hex)
else()
  set(merged_hex_to_flash ${merged}.hex)
endif()

get_target_property(runners_content runners_yaml_props_target yaml_contents)

string(REGEX REPLACE "hex_file:[^\n]*"
  "hex_file: ${merged_hex_to_flash}" runners_content_updated_hex_file ${runners_content})

set_property(
  TARGET         runners_yaml_props_target
  PROPERTY       yaml_contents ${runners_content_updated_hex_file}
  )
