#
# Copyright (c) 2022 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

set(PROVISION_HEX_NAME     provision_counters.hex)
set(PROVISION_HEX          ${PROJECT_BINARY_DIR}/${PROVISION_HEX_NAME})

if (CONFIG_SECURE_BOOT)
  set(monotonic_counter_arg
    --num-counter-slots-version ${CONFIG_SB_NUM_VER_COUNTER_SLOTS}
  )
endif()

if(CONFIG_MCUBOOT_HARDWARE_DOWNGRADE_PREVENTION)
  set(mcuboot_counters_slots
	--num-counter-slots-mcuboot
	${CONFIG_MCUBOOT_HW_DOWNGRADE_PREVENTION_COUNTER_SLOTS}
  )
endif()

add_custom_command(
  OUTPUT
  ${PROVISION_HEX}
  COMMAND
  ${PYTHON_EXECUTABLE}
  ${NRF_DIR}/scripts/bootloader/provision_counters.py
  --address $<TARGET_PROPERTY:partition_manager,PM_PROVISION_COUNTERS_ADDRESS>
  --size    $<TARGET_PROPERTY:partition_manager,PM_PROVISION_COUNTERS_SIZE>
  --output ${PROVISION_HEX}
  ${monotonic_counter_arg}
  ${mcuboot_counters_slots}
  WORKING_DIRECTORY
  ${PROJECT_BINARY_DIR}
  COMMENT
  "Creating counters data to be provisioned to the Bootloader, storing to ${PROVISION_HEX_NAME}"
  USES_TERMINAL
)

add_custom_target(
  provision_counters_target
  DEPENDS
  ${PROVISION_HEX}
  )

get_property(
  provision_counters_set
  GLOBAL PROPERTY provision_counters_PM_HEX_FILE SET
  )

if(NOT provision_counters_set)
  # Set hex file and target for the 'provision_counters' placeholder partition.
  # This includes the hex file (and its corresponding target) to the build.
  set_property(
    GLOBAL PROPERTY
    provision_counters_PM_HEX_FILE
    ${PROVISION_HEX}
    )

  set_property(
    GLOBAL PROPERTY
    provision_counters_PM_TARGET
    provision_counters_target
    )
endif()
