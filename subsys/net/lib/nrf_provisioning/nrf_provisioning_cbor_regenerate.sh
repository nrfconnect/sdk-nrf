#
# Copyright (c) 2023 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause

zcbor code --default-max-qty 1234567890 -c nrf_provisioning_cbor.cddl \
	-t responses -e \
	--oc  src/nrf_provisioning_cbor_encode.c \
	--oh  include/nrf_provisioning_cbor_encode.h \
	--oht include/nrf_provisioning_cbor_encode_types.h \
	--file-header "Copyright (c) 2024 Nordic Semiconductor ASA

SPDX-License-Identifier: LicenseRef-Nordic-5-Clause"

if [ $? -ne 0 ]; then
	echo "Encoder generation failed!"
	exit
fi

zcbor code --default-max-qty CONFIG_NRF_PROVISIONING_CBOR_RECORDS -c nrf_provisioning_cbor.cddl \
	-t commands -d \
	--oc  src/nrf_provisioning_cbor_decode.c \
	--oh  include/nrf_provisioning_cbor_decode.h \
	--oht include/nrf_provisioning_cbor_decode_types.h \
	--file-header "Copyright (c) 2024 Nordic Semiconductor ASA

SPDX-License-Identifier: LicenseRef-Nordic-5-Clause"

if [ $? -ne 0 ]; then
	echo "Decoder generation failed!"
	exit
fi

git add -A
git commit -mdummy # Add dummy commit so apply works

if [ $? -ne 0 ]; then
	echo "dummy git commit failed!"
	exit
fi

# Apply required patch
git apply -3 --apply nrf_provisioning_cbor.patch

if [ $? -ne 0 ]; then
	echo "Patching failed!"
	git reset HEAD~1 # remove dummy commit
	exit
fi

git reset HEAD~1 # remove dummy commit

# Format the generated files
clang-format -i src/nrf_provisioning_cbor*.c
clang-format -i include/nrf_provisioning_cbor*.h
