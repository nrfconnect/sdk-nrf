#
# Copyright (c) 2018 Nordic Semiconductor
#
# SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
#

zephyr_library()
zephyr_library_sources(
    coap_block.c
    coap_message.c
    coap_observe.c
    coap_option.c
    coap_queue.c
    coap_resource.c
    coap_transport_socket.c
    coap.c
)
