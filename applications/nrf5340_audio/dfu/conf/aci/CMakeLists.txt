#
# Copyright (c) 2022 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

# This check is needed to avoid infinite recursion. This module code will
# be executed for all images in the build, also for the child image being
# added below.

if (CONFIG_EMPTY_NET_CORE_CHILD_IMAGE)

  add_child_image(
    NAME empty_net
    SOURCE_DIR ${CMAKE_CURRENT_LIST_DIR}/../../empty_net
    DOMAIN CPUNET
    BOARD ${CONFIG_DOMAIN_CPUNET_BOARD}
    )
endif()
