#!/usr/bin/env python3
#
# Copyright (c) 2023 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause

"""
Utility for combining Thread and Wi-Fi application binary images into one Matter OTA compliant file
"""

import sys
import argparse
import tempfile
import os
import cbor2
import struct
import io

ZEPHYR_BASE = os.getenv('ZEPHYR_BASE')
if ZEPHYR_BASE is not None:
    NCS_PATH = os.path.join(ZEPHYR_BASE, '../')
else:
    NCS_PATH = os.path.join(os.path.dirname(__file__), '../../../../../')
sys.path.append(os.path.join(NCS_PATH, 'modules/lib/matter/src/app'))
sys.path.append(os.path.join(NCS_PATH, 'nrf/scripts/bootloader'))

import ota_image_tool as ota
import dfu_multi_image_tool as multi

# ota_image_tool args translators
class ParseHeaderArgs:
    def __init__(self, imageFile,):
        self.image_file = imageFile

class RemoveHeaderArgs:
    def __init__(self, imageFile, outputFile):
        self.image_file = imageFile
        self.output_file = outputFile

tlvHeaderKeysMap = {
    ota.HeaderTag.VENDOR_ID : 'vendor_id',
    ota.HeaderTag.PRODUCT_ID : 'product_id',
    ota.HeaderTag.VERSION : 'version',
    ota.HeaderTag.VERSION_STRING : 'version_str',
    ota.HeaderTag.MIN_VERSION : 'min_version',
    ota.HeaderTag.MAX_VERSION : 'max_version',
    ota.HeaderTag.RELEASE_NOTES_URL : 'release_notes',
    ota.HeaderTag.DIGEST_TYPE : 'digest_algorithm'
}

class GenerateOtaImageArgs:
    def __init__(self, tlv: dict, input_file: str, output_file: str):
        for key in tlvHeaderKeysMap:
            valueToSet = tlv[key] if key in tlv else None
            if tlvHeaderKeysMap[key] == 'digest_algorithm':
                valueToSet = next(key for key, value in ota.DIGEST_ALGORITHM_ID.items() if value == valueToSet) # inverse lookup
            self.__dict__[tlvHeaderKeysMap[key]] = valueToSet
        self.input_files = [input_file]
        self.output_file = output_file


# Helper functions
def remove_ota_header(imageFilePath: str) -> bytes:
    """
    Return binary representation of OTA DFU image file with Matter specific OTA header removed
    """
    with tempfile.TemporaryDirectory() as tempDir:
        outputFile = os.path.join(tempDir, 'outputFile')
        args = RemoveHeaderArgs(imageFilePath, outputFile)
        ota.remove_header(args)
        with open(args.output_file, 'rb') as extractedImageFile:
            return extractedImageFile.read()

def parse_multi_image_header(image: bytes) -> object:
    """
    Return JSON representation of NRF multi image DFU header extracted from input bytes object
    """
    return multi.parse_header(io.BytesIO(image))

def extract_core_images(fullImage: bytes) -> list:
    """
    Extract app core and net core raw images from given NRF multi image binary representation
    """
    cborHeader = parse_multi_image_header(fullImage)
    APP_CORE_ID = 0
    offset = cborHeader["img"][APP_CORE_ID]['size']
    bytesHeader = cbor2.dumps(cborHeader)
    imageStart = len(bytesHeader) + len(struct.pack('<H', len(bytesHeader))) # the header is preceded by its size (encoded on 2 bytes)
    headerLessImage = fullImage[imageStart:]
    return headerLessImage[:offset], headerLessImage[offset:]

def tlv_headers_equal(first: dict, second: dict) -> bool:
    """
    Verify if the relevant TLV header fields are the same across two provided JSONs
    """
    exclude_key_list = [ota.HeaderTag.PAYLOAD_SIZE, ota.HeaderTag.DIGEST]
    return all(first.get(key) == second.get(key) for key in first if key not in exclude_key_list)

def generate_multi_image(images : list) -> bytes:
    """
    Generate the NRF multi image bytes based on provided image list
    """
    imageFiles = []
    with tempfile.TemporaryDirectory() as tempDir:
        for idx, image in enumerate(images):
            outputFile = os.path.join(tempDir, f'outputFile{idx}')
            imageFiles.append([idx, outputFile])
            with open(outputFile, 'wb') as tempImgFile:
                tempImgFile.write(image)
        outputPath = os.path.join(tempDir, "multiImageFile")
        multi.generate_image(imageFiles, outputPath)
        with open(outputPath, 'rb') as result:
            return result.read()

def generate_ota_multi_variant_image(tlv: dict, multiImage: bytes, outputFile: str) -> None:
    """
    Generate the Matter OTA file based on the provided TLV header and NRF multi image bytes
    """
    with tempfile.TemporaryDirectory() as tempDir:
        inputFile = os.path.join(tempDir, 'inputFile')
        with open(inputFile, 'wb') as tempImgFile:
            tempImgFile.write(multiImage)
        otaArgs = GenerateOtaImageArgs(tlv, inputFile, outputFile)
        ota.validate_header_attributes(otaArgs)
        ota.generate_image(otaArgs)

def main():
    parser = argparse.ArgumentParser(
        description='Matter OTA multi-variant application image utility')
    parser.add_argument('input_file1', help='Path to the Thread image file')
    parser.add_argument('input_file2', help='Path to the Wi-Fi image file')
    parser.add_argument('output_file', help='Path to the output image file')

    parseArgs = parser.parse_args()

    _, _, _, threadHeaderTlv = ota.parse_header(ParseHeaderArgs(parseArgs.input_file1))
    _, _, _, wifiHeaderTlv  = ota.parse_header(ParseHeaderArgs(parseArgs.input_file2))

    if not tlv_headers_equal(threadHeaderTlv, wifiHeaderTlv):
        print("Headers of provided .ota files must be equal!")
        return

    targetHeaderTlv = threadHeaderTlv = wifiHeaderTlv

    threadMultiImage = remove_ota_header(parseArgs.input_file1)
    wifiMultiImage = remove_ota_header(parseArgs.input_file2)

    threadAppCore, threadNetCore = extract_core_images(threadMultiImage)
    wifiAppCore, wifiNetCore = extract_core_images(wifiMultiImage)

    multiImage = generate_multi_image([threadAppCore, threadNetCore, wifiAppCore, wifiNetCore])
    generate_ota_multi_variant_image(targetHeaderTlv, multiImage, parseArgs.output_file)

    print("OTA file generated successfully!")

if __name__ == "__main__":
    main()
