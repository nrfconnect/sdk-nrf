#
# Generated using zcbor version 0.6.0
# https://github.com/NordicSemiconductor/zcbor
#

add_library(cbor_encode)
find_package(Python3)
target_sources(cbor_encode PRIVATE
    ${Python3_LIBRARY}/zcbor/src/zcbor_decode.c
    ${Python3_LIBRARY}/zcbor/src/zcbor_encode.c
    ${Python3_LIBRARY}/zcbor/src/zcbor_common.c
    src/cbor_encode.c
    )
target_include_directories(cbor_encode PUBLIC
    src
    ${Python3_LIBRARY}/zcbor/include
    )
