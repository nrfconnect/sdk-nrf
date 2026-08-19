#
# Copyright (c) 2026 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

# Keep debug access open in the provisioning image.
#
# SB_CONFIG_APPROTECT_LOCK and SB_CONFIG_SECURE_APPROTECT_LOCK apply to every
# image in the build, including this one. On nRF54L that would make the
# provisioning image lock the TAMPC PROTECT debug signals in SystemInit(), before
# main() runs, so the debugger could no longer attach once provisioning had run.
# The application must be programmed after provisioning, and the only way past a
# locked device is "west flash --recover", which erases the UICR OTP and the KMU
# and therefore undoes the provisioning that just completed.
#
# The provisioning image is a one-shot production step that hands the device over
# to the application, so it leaves the debug signals open and lets the
# application image chain (b0 first) apply the lock instead.
#
# This runs as the image's IMAGE_CONF_SCRIPT, which sysbuild includes when
# configuring the image. That is after the sysbuild-level APPROTECT settings have
# been applied, so these values are the ones that end up in the image
# configuration.
set_config_bool(provisioning_image CONFIG_NRF_APPROTECT_LOCK n)
set_config_bool(provisioning_image CONFIG_NRF_SECURE_APPROTECT_LOCK n)
set_config_bool(provisioning_image CONFIG_NRF_APPROTECT_DISABLE y)
set_config_bool(provisioning_image CONFIG_NRF_SECURE_APPROTECT_DISABLE y)
