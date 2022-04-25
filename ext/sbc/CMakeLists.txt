#
# Copyright (c) 2022 Nordic Semiconductor
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

zephyr_library_named(sbc)

zephyr_include_directories(
    encoder/include
    decoder/include
)
zephyr_library_sources(
    encoder/srce/sbc_analysis.c
    encoder/srce/sbc_dct.c
    encoder/srce/sbc_dct_coeffs.c
    encoder/srce/sbc_enc_bit_alloc_mono.c
    encoder/srce/sbc_enc_bit_alloc_ste.c
    encoder/srce/sbc_enc_coeffs.c
    encoder/srce/sbc_encoder.c
    encoder/srce/sbc_packing.c
)

zephyr_library_sources(
    decoder/srce/alloc.c
    decoder/srce/bitalloc.c
    decoder/srce/bitalloc-sbc.c
    decoder/srce/bitstream-decode.c
    decoder/srce/decoder-oina.c
    decoder/srce/decoder-private.c
    decoder/srce/decoder-sbc.c
    decoder/srce/dequant.c
    decoder/srce/framing.c
    decoder/srce/framing-sbc.c
    decoder/srce/oi_codec_version.c
    decoder/srce/synthesis-sbc.c
    decoder/srce/synthesis-dct8.c
    decoder/srce/synthesis-8-generated.c
)
