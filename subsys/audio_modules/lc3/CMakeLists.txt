#
# Copyright (c) 2018 Nordic Semiconductor
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

add_subdirectory_ifdef(CONFIG_AUDIO_MODULE_LC3_DECODER_T2 t2/decoder)
add_subdirectory_ifdef(CONFIG_SW_LC3_ENCODER_T2 t2/encoder)
