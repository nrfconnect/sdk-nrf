.. _ug_nrf54h20_keys:

Provisioning keys on the nRF54H20 SoC
#####################################

.. contents::
   :local:
   :depth: 2

The keys provisioning workflow for the nRF54H20 SoC consists of two main steps:

1. Generating the required metadata using a script provided with the |NCS|.
#. Programming the keys to the nRF54H20 SoC.

Generating the keys
===================

A script is used to generate the necessary cryptographic keys, BLOBs, and metadata required for provisioning.
The script follows the PSA Crypto standard to generate the required 28-byte key.

To generate the keys, follow these steps:

1. Generate private keys using Ed25519::

      openssl genpkey -algorithm Ed25519 -out MANIFEST_APPLICATION_GEN1_priv.pem
      openssl genpkey -algorithm Ed25519 -out MANIFEST_RADIOCORE_GEN1_priv.pem
      openssl genpkey -algorithm Ed25519 -out MANIFEST_OEM_ROOT_GEN1_priv.pem

#. Extract public keys::

      openssl pkey -in MANIFEST_APPLICATION_GEN1_priv.pem -pubout -out MANIFEST_APPLICATION_GEN1_pub.pem
      openssl pkey -in MANIFEST_RADIOCORE_GEN1_priv.pem -pubout -out MANIFEST_RADIOCORE_GEN1_pub.pem
      openssl pkey -in MANIFEST_OEM_ROOT_GEN1_priv.pem -pubout -out MANIFEST_OEM_ROOT_GEN1_pub.pem

#. Check the required key IDs::

      MANIFEST_PUBKEY_APPLICATION_GEN1 = 0x40022100
      MANIFEST_PUBKEY_APPLICATION_GEN2 = 0x40022101
      MANIFEST_PUBKEY_APPLICATION_GEN3 = 0x40022102
      MANIFEST_PUBKEY_OEM_ROOT_GEN1 = 0x4000AA00
      MANIFEST_PUBKEY_OEM_ROOT_GEN2 = 0x4000AA01
      MANIFEST_PUBKEY_OEM_ROOT_GEN3 = 0x4000AA02
      MANIFEST_PUBKEY_RADIOCORE_GEN1 = 0x40032100
      MANIFEST_PUBKEY_RADIOCORE_GEN2 = 0x40032101
      MANIFEST_PUBKEY_RADIOCORE_GEN3 = 0x40032102

#. Convert the public keys to RAW format::

      from cryptography.hazmat.primitives import serialization

      for key_path in [
         'MANIFEST_APPLICATION_GEN1_pub.pem',
         'MANIFEST_RADIOCORE_GEN1_pub.pem',
         'MANIFEST_OEM_ROOT_GEN1_pub.pem'
      ]:
          with open(key_path, 'rb') as key_file:
              key_data = key_file.read()
              key = serialization.load_pem_public_key(key_data)
              pub_key_bytes = key.public_bytes(
                  encoding=serialization.Encoding.Raw,
                  format=serialization.PublicFormat.Raw
              )
              print(f"Key: {key_path}, value: {pub_key_bytes.hex()}")

      Key: MANIFEST_APPLICATION_GEN1_pub.pem, value: 5e9c6bc9a4a74a56a54c7b281082619aeaa76cff929d676dc0649d4135bda744
      Key: MANIFEST_RADIOCORE_GEN1_pub.pem, value: 8c2c1686a1afe09831a6068f573c08becb345f0add41b87a2d3dca8219c908af
      Key: MANIFEST_OEM_ROOT_GEN1_pub.pem, value: 58f3f146a3f26a60e566e23f0d9f8cc01cb2dce5d2ca7413719de58a50ddda63


#. Create a JSON input file with the ``generate_psa_key_attributes.py`` script:

   * For the application core::

         python generate_psa_key_attributes.py --usage VERIFY_MESSAGE_EXPORT --id 0x40022100 --type ECC_TWISTED_EDWARDS  --size 255 --algorithm EDDSA_PURE --location PERSISTENT_CRACEN --key 5e9c6bc9a4a74a56a54c7b281082619aeaa76cff929d676dc0649d4135bda744  --file all_keys.json --cracen_usage RAW

   * For the radio core::

         python generate_psa_key_attributes_new.py --usage VERIFY_MESSAGE_EXPORT --id 0x40032100 --type ECC_TWISTED_EDWARDS  --size 255 --algorithm EDDSA_PURE --location PERSISTENT_CRACEN --key 8c2c1686a1afe09831a6068f573c08becb345f0add41b87a2d3dca8219c908af  --file all_keys.json --cracen_usage RAW

   * For the main root manifest::

         python generate_psa_key_attributes_new.py --usage VERIFY_MESSAGE_EXPORT --id 0x4000AA00 --type ECC_TWISTED_EDWARDS  --size 255 --algorithm EDDSA_PURE --location PERSISTENT_CRACEN --key 58f3f146a3f26a60e566e23f0d9f8cc01cb2dce5d2ca7413719de58a50ddda63  --file all_keys.json --cracen_usage RAW


The generated key data is stored in a JSON file, which serves as an input for the next step.

Programming the keys
====================

nRF Util is used to program the generated keys into the target device.
It takes the JSON file as input and injects it without validating its contents.
In this scenario, nRF Util functions as a transport layer, transferring the key data to the correct location in the device.

To provision the keys, use nRF Util as follows::

      export snr=<snr number>
      nrfutil device x-provision-nrf54h-keys --serial-number $snr --key-file key_application_gen_1.json
