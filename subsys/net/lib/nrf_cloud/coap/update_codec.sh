# Copyright (c) 2023 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause

export HDR="
Copyright (c) 2023 Nordic Semiconductor ASA
SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
"

zcbor code -c cddl/nrf_cloud_coap_ground_fix.cddl --default-max-qty 10 -e \
	-t ground_fix_req \
	--oc src/ground_fix_encode.c \
	--oh include/ground_fix_encode.h \
	--oht include/ground_fix_encode_types.h \
	--file-header "$HDR"

if [ $? -ne 0 ]; then
	echo "Encoder generation failed!"
	exit
fi

zcbor code -c cddl/nrf_cloud_coap_ground_fix.cddl --default-max-qty 10 -d \
	-t ground_fix_resp \
	--oc src/ground_fix_decode.c \
	--oh include/ground_fix_decode.h \
	--oht include/ground_fix_decode_types.h \
	--file-header "$HDR"

if [ $? -ne 0 ]; then
	echo "Decoder generation failed!"
	exit
fi

zcbor code -c cddl/nrf_cloud_coap_device_msg.cddl --default-max-qty 10 -e \
	-t message_out \
	--oc src/msg_encode.c \
	--oh include/msg_encode.h \
	--oht include/msg_encode_types.h \
	--file-header "$HDR"

if [ $? -ne 0 ]; then
	echo "Encoder generation failed!"
	exit
fi

zcbor code -c cddl/nrf_cloud_coap_agnss.cddl       --default-max-qty 10 -e \
	-t agnss_req \
	--oc src/agnss_encode.c \
	--oh include/agnss_encode.h \
	--oht include/agnss_encode_types.h \
	--file-header "$HDR"

if [ $? -ne 0 ]; then
	echo "Encoder generation failed!"
	exit
fi

zcbor code -c cddl/nrf_cloud_coap_pgps.cddl       --default-max-qty 10 -e \
	-t pgps_req \
	--oc src/pgps_encode.c \
	--oh include/pgps_encode.h \
	--oht include/pgps_encode_types.h \
	--file-header "$HDR"

if [ $? -ne 0 ]; then
	echo "Encoder generation failed!"
	exit
fi

zcbor code -c cddl/nrf_cloud_coap_pgps.cddl       --default-max-qty 10 -d \
	-t pgps_resp \
	--oc src/pgps_decode.c \
	--oh include/pgps_decode.h \
	--oht include/pgps_decode_types.h \
	--file-header "$HDR"

if [ $? -ne 0 ]; then
	echo "Decoder generation failed!"
	exit
fi

# Format the generated files
clang-format -i src/*code.c
clang-format -i include/*encode*.h
clang-format -i include/*decode*.h

# Commit the generated files and the matching CDDL file
git add src/*code.c include/*encode*.h include/*decode*.h
git add cddl/*.cddl

# alternate design -- combine all encoders into one file and all decoders into another
# zcbor code -c cddl/nrf_cloud_coap_ground_fix.cddl \
# 	-c cddl/nrf_cloud_coap_device_msg.cddl \
# 	-c cddl/nrf_cloud_coap_agnss.cddl \
# 	-c cddl/nrf_cloud_coap_pgps.cddl \
# 	--default-max-qty 10 -e \
# 	-t ground_fix_req message_out agnss_req pgps_req \
# 	--oc src/coap_encode.c \
# 	--oh include/coap_encode.h \
# 	--oht include/coap_encode_types.h
# zcbor code -c cddl/nrf_cloud_coap_ground_fix.cddl \
# 	-c cddl/nrf_cloud_coap_pgps.cddl \
# 	--default-max-qty 10 -d \
# 	-t ground_fix_resp pgps_resp \
# 	--oc src/coap_decode.c \
# 	--oh include/coap_decode.h \
# 	--oht include/coap_decode_types.h
