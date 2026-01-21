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
  if(REGION_DEVICE)
    list(APPEND region_arguments "--${REGION_NAME}-device;${REGION_DEVICE}")
  list(APPEND region_arguments
       "--${REGION_NAME}-default-driver-kconfig;${REGION_DEFAULT_DRIVER_KCONFIG}")
  endif()
  if(REGION_DYNAMIC_PARTITION)
    list(APPEND region_arguments
      "--${REGION_NAME}-dynamic-partition;${REGION_DYNAMIC_PARTITION}")
  endif()
endmacro()

# Load static configuration if found.
# Try user defined file first, then file found in configuration directory,
# finally file from board directory.
zephyr_get(PM_STATIC_YML_FILE SYSBUILD GLOBAL)

if(DEFINED PM_STATIC_YML_FILE)
  string(CONFIGURE "${PM_STATIC_YML_FILE}" user_def_pm_static)
endif()

zephyr_get(COMMON_CHILD_IMAGE_CONFIG_DIR)
string(CONFIGURE "${COMMON_CHILD_IMAGE_CONFIG_DIR}" COMMON_CHILD_IMAGE_CONFIG_DIR)
foreach(config_dir ${APPLICATION_CONFIG_DIR} ${COMMON_CHILD_IMAGE_CONFIG_DIR})
  ncs_file(CONF_FILES ${config_dir}
           PM conf_dir_pm_static
           DOMAIN ${DOMAIN}
  )
  if(EXISTS ${conf_dir_pm_static})
    break()
  endif()
endforeach()

ncs_file(CONF_FILES ${BOARD_DIR}
         PM board_dir_pm_static
         DOMAIN ${DOMAIN}
)

ncs_file(CONF_FILES ${BOARD_DIR}
         PM board_dir_pm_static_common
         DOMAIN ${DOMAIN}
)

if(EXISTS "${user_def_pm_static}" AND NOT IS_DIRECTORY "${user_def_pm_static}")
  set(static_configuration_file ${user_def_pm_static})
elseif(EXISTS ${conf_dir_pm_static})
  set(static_configuration_file ${conf_dir_pm_static})
elseif(EXISTS ${board_dir_pm_static})
  set(static_configuration_file ${board_dir_pm_static})
elseif(EXISTS ${board_dir_pm_static_common})
  set(static_configuration_file ${board_dir_pm_static_common})
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
