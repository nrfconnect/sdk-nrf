.. _ug_nrf54h20_keys:

Provisioning keys on the nRF54H20 SoC
#####################################

.. contents::
   :local:
   :depth: 2

This guide describes how to provision pre-generated cryptographic keys on an nRF54H20 SoC.
It is intended for developers who wish to create a predefined set of keys for their application, made available through PSA Crypto key identifiers.

Prerequisites
=============

To follow this guide, your nRF54H20 device must meet the following requirements:

* On the nRF54H20 DK, you must :ref:`provision <ug_nrf54h20_gs_bringup_soc_bin>` the DK as described in the :ref:`ug_nrf54h20_gs` page.
* On a custom nRF54H20-based device, you must :ref:`configure, generate, and program the BICR <ug_nrf54h20_custom_pcb_bicr>` as described in the :ref:`ug_nrf54h20_custom_pcb` page.

Overview
========

The keys provisioning workflow for the nRF54H20 SoC consists of three main steps:

1. Choosing key identifiers based on the desired properties for each key.
#. Generating the required metadata using a script provided with the |NCS|.
#. Provisioning the keys to the nRF54H20 SoC.

.. rst-class:: numbered-step

Choosing a key ID
=================

Key provisioning is supported by the |ISE| firmware through the :ref:`ug_crypto_architecture_implementation_standards_ironside` of the PSA Crypto API.
|ISE| defines two categories of keys that can be provisioned: standard *user keys* and non-standard *revocable keys*.
These categories are tied to distinct key ID ranges.

User keys
---------

These are the standard persistent keys conforming to the :ref:`supported version of the PSA Crypto API <ug_psa_certified_api_overview_crypto_ncs>`:

* Minimum ID: ``0x00000001`` (``PSA_KEY_ID_USER_MIN``)
* Maximum ID: ``0x3FFFFFFF`` (``PSA_KEY_ID_USER_MAX``)

In order to successfully provision user keys, you must first configure cryptographic partitions in your :ref:`ug_nrf54h20_ironside_uicr_securestorage` configuration.

Revocable keys
--------------

These are an extension of user keys with special properties for secure provisioning during device manufacturing:

* Minimum ID: ``0x40002000``
* Maximum ID: ``0x4FFFFFFF``

Revocable keys can only be provisioned as long as the :ref:`ug_nrf54h20_ironside_se_uicr_lock` configuration is disabled.
Once the UICR is locked, no more keys can be created in this range, which means that when a revocable key is destroyed, it cannot be replaced.

These keys are provisioned into |ISE|'s internal storage, not the location controlled by :ref:`ug_nrf54h20_ironside_uicr_securestorage`.

.. _ug_nrf54h20_keys_generating:

.. rst-class:: numbered-step

Generating key metadata
=======================

The :ref:`generate_psa_key_attributes_script` is used to generate a JSON file containing the necessary cryptographic keys, BLOBs, and metadata required for provisioning.

Here is an example command to generate metadata for provisioning the public key part of an Ed25519 key, from a pre-existing PEM file, as a revocable key:

.. parsed-literal::
   :class: highlight

   python generate_psa_key_attributes.py --usage VERIFY --id 0x40002000 --type ECC_PUBLIC_KEY_TWISTED_EDWARDS --key-bits 255 --algorithm EDDSA_PURE --location LOCATION_LOCAL_STORAGE --key-from-file public_key.pem --file all_keys.json --persistence PERSISTENCE_DEFAULT

The output file (named :file:`all_keys.json` in the previous example) serves as an input for the next step.

.. rst-class:: numbered-step

Provisioning the keys
=====================

.. include:: ../../../../../scripts/generate_psa_key_attributes/generate_psa_key_attributes.rst
   :start-after: nrfutil_provision_keys_info_start
   :end-before: nrfutil_provision_keys_info_end

The |ISE| firmware on the device handles the actual key provisioning using PSA Crypto's ``psa_import_key`` function.
Provisioning a key calls the function to import the key:

* The ``metadata`` field from the JSON file is used for the function's ``attributes`` argument.
* The ``value`` field is passed to the function's ``data`` argument.
* The length of the ``value`` field is passed to the function's ``data_length`` argument.

.. include:: ../../../../../scripts/generate_psa_key_attributes/generate_psa_key_attributes.rst
   :start-after: nrfutil_provision_keys_command_start
   :end-before: nrfutil_provision_keys_command_end

.. note::
   The :ref:`ug_nrf54h20_ironside_se_eraseall_command` destroys all keys stored on the device.
   Whenever you execute this boot command, you have to provision your keys all over again.
