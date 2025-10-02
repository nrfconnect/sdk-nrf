.. _ug_nrf54h20_keys:

Provisioning keys on the nRF54H20 SoC
#####################################

.. contents::
   :local:
   :depth: 2

This guide describes how to generate and provision cryptographic public keys on an nRF54H20 SoC in the Root of Trust (RoT) lifecycle state.
It is intended for developers defining manifest signing keys for the application core, radio core, and OEM root.
A successful provisioning makes the keys available to Secure Domain Firmware through PSA Crypto key identifiers.

Prerequisites
=============

To follow this guide, your nRF54H20 device must meet the following requirement:

* On the nRF54H20 DK, you must :ref:`provision <ug_nrf54h20_gs_bringup_soc_bin>` the DK as described in the :ref:`ug_nrf54h20_getting_started` page.
* On a custom nRF54H20-based device, you must :ref:`configure, generate, and program the BICR <ug_nrf54h20_custom_pcb_bicr>` as described in the :ref:`ug_nrf54h20_custom_pcb` page.
* You must configure `UICR.SECURESTORAGE`_ in your |ISE| UICR configuration.

Overview
========

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

#. Create a JSON input file with the :ref:`generate_psa_key_attributes_script`:

   * For the application core::

        python generate_psa_key_attributes.py --usage VERIFY --allow-usage-export --id 0x40022100 --type ECC_PUBLIC_KEY_TWISTED_EDWARDS --key-bits 255 --algorithm EDDSA_PURE --location LOCATION_CRACEN --key-from-file MANIFEST_APPLICATION_GEN1_pub.pem  --file all_keys.json --cracen-usage RAW --persistence PERSISTENCE_DEFAULT

   * For the radio core::

        python generate_psa_key_attributes.py --usage VERIFY --allow-usage-export --id 0x40032100 --type ECC_PUBLIC_KEY_TWISTED_EDWARDS --key-bits 255 --algorithm EDDSA_PURE --location LOCATION_CRACEN --key-from-file MANIFEST_RADIOCORE_GEN1_pub.pem --file all_keys.json --cracen-usage RAW --persistence PERSISTENCE_DEFAULT

   * For the main root manifest::

        python generate_psa_key_attributes.py --usage VERIFY --allow-usage-export --id 0x4000AA00 --type ECC_PUBLIC_KEY_TWISTED_EDWARDS --key-bits 255 --algorithm EDDSA_PURE --location LOCATION_CRACEN --key-from-file MANIFEST_OEM_ROOT_GEN1_pub.pem --file all_keys.json --cracen-usage RAW --persistence PERSISTENCE_DEFAULT


The generated key data is stored in a JSON file, which serves as an input for the next step.

Provisioning the keys
=====================

.. include:: ../../../../../scripts/generate_psa_key_attributes/generate_psa_key_attributes.rst
   :start-after: nrfutil_provision_keys_info_start
   :end-before: nrfutil_provision_keys_info_end

The Secure Domain Firmware on the device handles the actual key provisioning using PSA Crypto's ``psa_import_key`` function.
Provisioning a key calls the function to import the key:

* The ``metadata`` field from the JSON file is used for the function's attributes argument.
* The ``value`` field is passed to the function's data argument.
* The function's ``data_length`` is set to the length of the value field.

.. include:: ../../../../../scripts/generate_psa_key_attributes/generate_psa_key_attributes.rst
   :start-after: nrfutil_provision_keys_command_start
   :end-before: nrfutil_provision_keys_command_end
