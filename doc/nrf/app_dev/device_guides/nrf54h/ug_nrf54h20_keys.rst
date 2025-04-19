.. _ug_nrf54h20_keys:

Provisioning keys on the nRF54H20 SoC
#####################################

.. contents::
   :local:
   :depth: 2

The keys provisioning workflow for the nRF54H20 SoC consists of two main steps:

1. Generating the required metadata using a script provided with the |NCS|.
#. Provisioning the keys to the nRF54H20 SoC.

.. note::
   The nRF54H20 SoC must be in RoT lifecycle state for key provisioning to work.
   For more details on lifecycle states, see :ref:`ug_nrf54h20_architecture_lifecycle`.

.. _ug_nrf54h20_keys_generating:

Generating the keys
===================

A script is used to generate the necessary cryptographic keys, BLOBs, and metadata required for provisioning.
The script follows the PSA Crypto standard to generate the required 28-byte key.
It is located in the :file:`nrf/scripts/generate_psa_key_attributes.py` file.

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

#. Create a JSON input file with the ``generate_psa_key_attributes.py`` script:

   * For the application core::

        python generate_psa_key_attributes.py --usage VERIFY_MESSAGE_EXPORT --id 0x40022100 --type ECC_TWISTED_EDWARDS --size 255 --algorithm EDDSA_PURE --location LOCATION_CRACEN --key-from-file MANIFEST_APPLICATION_GEN1_pub.pem  --file all_keys.json --cracen_usage RAW --lifetime PERSISTENCE_DEFAULT

   * For the radio core::

        python generate_psa_key_attributes.py --usage VERIFY_MESSAGE_EXPORT --id 0x40032100 --type ECC_TWISTED_EDWARDS --size 255 --algorithm EDDSA_PURE --location LOCATION_CRACEN --key-from-file MANIFEST_RADIOCORE_GEN1_pub.pem --file all_keys.json --cracen_usage RAW --lifetime PERSISTENCE_DEFAULT

   * For the main root manifest::

        python generate_psa_key_attributes.py --usage VERIFY_MESSAGE_EXPORT --id 0x4000AA00 --type ECC_TWISTED_EDWARDS --size 255 --algorithm EDDSA_PURE --location LOCATION_CRACEN --key-from-file MANIFEST_OEM_ROOT_GEN1_pub.pem --file all_keys.json --cracen_usage RAW --lifetime PERSISTENCE_DEFAULT


The generated key data is stored in a JSON file, which serves as an input for the next step.

Provisioning the keys
=====================

nRF Util is used to provision the generated keys into the target device.
It takes the JSON file as input and injects it without validating its contents.
In this scenario, nRF Util functions as a transport layer, transferring the key data to the correct location in the device.

The Secure Domain Firmware on the device handles the actual key provisioning using PSA Crypto's ``psa_import_key`` function.
Provisioning a key calls the function to import the key:

* The ``metadata`` field from the JSON file is used for the function's attributes argument.
* The ``value`` field is passed to the function's data argument.
* The function's ``data_length`` is set to the length of the value field.

To provision the keys from ``all_keys.json`` onto the KMU of the nRF54H20 SoC, use nRF Util as follows::

      nrfutil device x-provision-keys --serial-number <snr> --key-file all_keys.json

For more information on how to provision keys, see the `Provisioning keys for hardware KMU`_ page in the nRF Util documentation.
