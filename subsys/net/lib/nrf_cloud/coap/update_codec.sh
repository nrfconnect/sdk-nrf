# Copyright (c) 2023 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause

export HDR="
Copyright (c) $(date +%Y) Nordic Semiconductor ASA
SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
"

zcbor code -c cddl/nrf_cloud_coap_ground_fix.cddl --default-max-qty 10 -e \
	-t ground_fix_req \
	--oc generated/src/ground_fix_encode.c \
	--oh generated/include/ground_fix_encode.h \
	--oht generated/include/ground_fix_encode_types.h \
	--file-header "$HDR"

if [ $? -ne 0 ]; then
	echo "Encoder generation failed!"
	exit
fi

zcbor code -c cddl/nrf_cloud_coap_ground_fix.cddl --default-max-qty 10 -d \
	-t ground_fix_resp \
	--oc generated/src/ground_fix_decode.c \
	--oh generated/include/ground_fix_decode.h \
	--oht generated/include/ground_fix_decode_types.h \
	--file-header "$HDR"

if [ $? -ne 0 ]; then
	echo "Decoder generation failed!"
	exit
fi

zcbor code -c cddl/nrf_cloud_coap_device_msg.cddl --default-max-qty 10 -e \
	-t message_out \
	--oc generated/src/msg_encode.c \
	--oh generated/include/msg_encode.h \
	--oht generated/include/msg_encode_types.h \
	--file-header "$HDR"

if [ $? -ne 0 ]; then
	echo "Encoder generation failed!"
	exit
fi

zcbor code -c cddl/nrf_cloud_coap_agnss.cddl       --default-max-qty 10 -e \
	-t agnss_req \
	--oc generated/src/agnss_encode.c \
	--oh generated/include/agnss_encode.h \
	--oht generated/include/agnss_encode_types.h \
	--file-header "$HDR"

if [ $? -ne 0 ]; then
	echo "Encoder generation failed!"
	exit
fi

zcbor code -c cddl/nrf_cloud_coap_pgnss.cddl       --default-max-qty 10 -e \
	-t pgnss_req \
	--oc generated/src/pgnss_encode.c \
	--oh generated/include/pgnss_encode.h \
	--oht generated/include/pgnss_encode_types.h \
	--file-header "$HDR"

if [ $? -ne 0 ]; then
	echo "Encoder generation failed!"
	exit
fi

zcbor code -c cddl/nrf_cloud_coap_pgnss.cddl       --default-max-qty 10 -d \
	-t pgnss_resp \
	--oc generated/src/pgnss_decode.c \
	--oh generated/include/pgnss_decode.h \
	--oht generated/include/pgnss_decode_types.h \
	--file-header "$HDR"

if [ $? -ne 0 ]; then
	echo "Decoder generation failed!"
	exit
fi

# Format the generated files
clang-format -i generated/src/*code.c
clang-format -i generated/include/*encode*.h
clang-format -i generated/include/*decode*.h

# Commit the generated files and the matching CDDL file
git add generated/src/*.c generated/include/*.h
git add cddl/*.cddl

# alternate design -- combine all encoders into one file and all decoders into another
# zcbor code -c cddl/nrf_cloud_coap_ground_fix.cddl \
# 	-c cddl/nrf_cloud_coap_device_msg.cddl \
# 	-c cddl/nrf_cloud_coap_agnss.cddl \
# 	-c cddl/nrf_cloud_coap_pgnss.cddl \
# 	--default-max-qty 10 -e \
# 	-t ground_fix_req message_out agnss_req pgnss_req \
# 	--oc generated/src/coap_encode.c \
# 	--oh generated/include/coap_encode.h \
# 	--oht generated/include/coap_encode_types.h
# zcbor code -c cddl/nrf_cloud_coap_ground_fix.cddl \
# 	-c cddl/nrf_cloud_coap_pgnss.cddl \
# 	--default-max-qty 10 -d \
# 	-t ground_fix_resp pgnss_resp \
# 	--oc generated/src/coap_decode.c \
# 	--oh generated/include/coap_decode.h \
# 	--oht generated/include/coap_decode_types.h
