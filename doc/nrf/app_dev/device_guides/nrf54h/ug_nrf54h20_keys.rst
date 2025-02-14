:orphan:

.. _ug_nrf54h20_keys:

Provisioning keys on the nRF54H20
#################################

.. contents::
   :local:
   :depth: 2

###TO DO: add script link, expand description, remove orphan directive.

The keys provisioning workflow for the nRF54H20 consists of two main steps:

1. Generating the required metadata using a script provided with the |NCS|.
#. Programming the keys to the nRF54H20 SoC.

Generating the keys
===================

A script is used to generate the necessary cryptographic keys, BLOBs, and metadata required for provisioning.
The script follows the PSA Crypto standard to generate the required 28-byte key.

The generated key data is stored in a JSON file, which serves as an input for the next step.

Programming the keys
====================

nRF Util is used to program the generated keys into the target device.
It takes the JSON file as input and injects it without validating its contents.
In this scenario, nRF Util functions as a transport layer, transferring the key data to the correct location in the device.
