#
# Copyright (c) 2018 Nordic Semiconductor
#
# SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
#

add_subdirectory_ifdef(CONFIG_CLOUD_API cloud)
add_subdirectory_ifdef(CONFIG_NRF_CLOUD nrf_cloud)
add_subdirectory_ifdef(CONFIG_DOWNLOAD_CLIENT download_client)
add_subdirectory_ifdef(CONFIG_FOTA_DOWNLOAD fota_download)
add_subdirectory_ifdef(CONFIG_AWS_JOBS aws_jobs)
add_subdirectory_ifdef(CONFIG_AWS_FOTA aws_fota)
add_subdirectory_ifdef(CONFIG_AWS_IOT aws_iot)
add_subdirectory_ifdef(CONFIG_ZZHC zzhc)
add_subdirectory_ifdef(CONFIG_ICALENDAR_PARSER icalendar_parser)
