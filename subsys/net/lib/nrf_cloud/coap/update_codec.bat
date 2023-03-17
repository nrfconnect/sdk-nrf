set HDR="Copyright (c) 2023 Nordic Semiconductor ASA SPDX-License-Identifier: LicenseRef-Nordic-5-Clause"
zcbor code -c cddl\nrf_cloud_coap_ground_fix.cddl --default-max-qty 10 -e -t ground_fix_req --oc src\ground_fix_encode.c --oh include\ground_fix_encode.h --oht include\ground_fix_encode_types.h --file-header %HDR%
zcbor code -c cddl\nrf_cloud_coap_ground_fix.cddl --default-max-qty 10 -d -t ground_fix_resp --oc src\ground_fix_decode.c --oh include\ground_fix_decode.h --oht include\ground_fix_decode_types.h --file-header %HDR%
zcbor code -c cddl\nrf_cloud_coap_device_msg.cddl --default-max-qty 10 -e -t message_out --oc src\msg_encode.c --oh include\msg_encode.h --oht include\msg_encode_types.h --file-header %HDR%
zcbor code -c cddl\nrf_cloud_coap_agps.cddl       --default-max-qty 10 -e -t agps_req --oc src\agps_encode.c --oh include\agps_encode.h --oht include\agps_encode_types.h --file-header %HDR%
zcbor code -c cddl\nrf_cloud_coap_pgps.cddl       --default-max-qty 10 -e -t pgps_req --oc src\pgps_encode.c --oh include\pgps_encode.h --oht include\pgps_encode_types.h --file-header %HDR%
zcbor code -c cddl\nrf_cloud_coap_pgps.cddl       --default-max-qty 10 -d -t pgps_resp --oc src\pgps_decode.c --oh include\pgps_decode.h --oht include\pgps_decode_types.h --file-header %HDR%
clang-format -i src/*code.c
clang-format -i include/*encode*.h
clang-format -i include/*decode*.h
git add src/*code.c include/*encode*.h include/*decode*.h
git add cddl/*.cddl
