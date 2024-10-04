#
# Copyright (c) 2024 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

# TODO: This is a temporary solution to be able to pass additional configuration files
# through sample.yaml. This will be deleted when the possibility to pass string
# Kconfig options for radio core (such as hci_ipc_...) through sample.yaml

if(DEFINED hci_ipc_TARGET_EXTRA_CONF_FILE)
  list(APPEND hci_ipc_EXTRA_CONF_FILE "${CMAKE_CURRENT_LIST_DIR}/${hci_ipc_TARGET_EXTRA_CONF_FILE}")
  list(REMOVE_DUPLICATES hci_ipc_EXTRA_CONF_FILE)
  set(hci_ipc_EXTRA_CONF_FILE "${hci_ipc_EXTRA_CONF_FILE}" CACHE INTERNAL "")
endif()