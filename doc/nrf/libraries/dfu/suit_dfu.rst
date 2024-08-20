.. _subsys_suit:

Software Updates for Internet of Things (SUIT)
##############################################

.. contents::
   :local:
   :depth: 2

The Software Updates for Internet of Things (SUIT) Device Firmware Upgrade (DFU) library provides functionality to a local domain for managing the update based on the SUIT manifest.

Overview
********

The SUIT is the DFU and secure boot method for nRF54H Series of System-on-Chip.
For a conceptual overview of the SUIT procedures, manifests, and components, read the :ref:`ug_nrf54h20_suit_dfu` page.

The SUIT DFU library provides APIs to start and prepare the device before the update procedure begins.
The preparation for the DFU can be described as a sequence within a SUIT manifest (suit-payload-fetch).
To interpret and execute this sequence, a SUIT processor must be instantiated in the local domain's firmware.
This library provides a simplified API that manages the SUIT processor state, executes the fetching sequence from the SUIT manifest, and passes the update candidate information to the Secure Domain.

The application requires the following steps to perform the Device Firmware Update:

1. Call :c:func:`suit_dfu_initialize` to initialize the SUIT DFU module before any other SUIT functionality is used.
   This function must be called only once, after the device drivers are ready.

#. If the design of the DFU system used by this device allows for the fetch of additional data in the ``suit-payload-fetch`` sequence,
   register fetch sources using :c:func:`suit_dfu_fetch_source_register`.

#. Store the SUIT candidate envelope in the non-volatile memory area defined as ``dfu_partition`` in the devicetree.

#. After the SUIT candidate envelope is stored, call :c:func:`suit_dfu_candidate_envelope_stored`.

#. Call :c:func:`suit_dfu_candidate_preprocess`.
   Any fetch source registered in step number 2 can be used by this function to fetch additional data.

#. Call :c:func:`suit_dfu_update_start`.
   This will start the firmware update by resetting the device and passing control over the update to the Secure Domain.

In addition to this library, a compatibility layer for the :ref:`lib_dfu_target` is available.
The compatibility layer is limited to updates that contain a single update binary and does not allow the local domain to process SUIT manifests.
To use other upgrade schemes, the SUIT DFU API must be used.

Dependencies
************

This module uses the following |NCS| libraries and drivers:

* `zcbor`_
* `suit-processor`_

This module also uses the following Zephyr drivers:

* :ref:`zephyr:flash_api`

API documentation
*****************

| Header file: :file:`include/dfu/suit_dfu.h`
| Source files: :file:`subsys/suit`

.. doxygengroup:: suit
