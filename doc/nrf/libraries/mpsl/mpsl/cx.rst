.. _mpsl_cx:

Short-Range Protocols External Radio Coexistence
################################################

.. contents::
   :local:
   :depth: 2

The radio coexistence feature allows the application to interface with several types of packet traffic arbiters (PTAs).
PTAs arbitrate the requested radio operations between all radios to avoid interference, providing better radio performance to devices using multiple interfering radios simultaneously, like a combination of IEEE 802.15.4, Bluetooth® Low Energy (LE), and Wi-Fi.
The arbitration algorithm used can vary between PTAs.

.. note::
  The radio coexistence feature is not needed for the arbitration between |BLE| and IEEE 802.15.4 in dynamic multiprotocol applications, as the dynamic multiprotocol does not use PTA for the arbitration.

Overview
********

The radio coexistence feature allows short-range protocol drivers (e.g. IEEE 802.15.4, Bluetooth LE) to communicate with the packet traffic arbiter (PTA) using :ref:`MPSL CX API <mpsl_api_sr_cx>`.
The MPSL CX API is hardware-agnostic and separates the implementation of the protocol driver from an implementation specific to given PTA.
To perform any radio operation, the protocol drivers must first request the appropriate access to the medium from the PTA.
The request informs the PTA implementation about which radio operations it wants to perform at that moment or shortly after, and what is the priority of the operation.
The radio operation can be executed only when the PTA grants it.
When the PTA revokes access to the medium during an ongoing radio operation, the radio protocol must abort this operation immediately.
When the radio operation is finished, the protocol driver must release the requested operation.

The implementation of MPSL CX translates the request invoked by radio protocol to hardware signals compatible with the PTA available in the system.
It also translates the hardware signals received from the PTA indicating which radio operations are allowed at that moment, and it passes this information to the radio protocol.

.. note::
  To learn more about the arbitration process during various transmission and reception modes for the IEEE 802.15.4 Radio Driver, see :ref:`Wi-Fi Coexistence module (PTA support) <rd_coex>`

Selecting CX Implementation
***************************

The :ref:`mpsl` itself does not provide any implementation of the CX interface.
For details on the implementations present in the |NCS|, see :ref:`ug_radio_coex`.
An application that needs to use CX must call :c:func:`mpsl_cx_interface_set()` during the system initialization.
The initialization of any resource needed by the selected CX implementation is not in scope of :ref:`mpsl` and must also be done during the system initialization.

.. note::
  In the |NCS|, the selection of a CX implementation and its appropriate initialization is done automatically, using Kconfig and Device Tree configuration options.
  Please refer to :ref:`ug_radio_coex`.

Using CX API in a protocol driver
*********************************

To use the CX API in a protocol driver, you must follow the :ref:`MPSL CX API <mpsl_api_sr_cx>` defined by :file:`nrfxlib/mpsl/include/protocol/mpsl_cx_protocol_api.h`.

Implementing CX API
*******************

For details on the implementations of the MPSL CX API for certain PTAs already supported by the |NCS|, see :ref:`ug_radio_coex_mpsl_cx_based`.
If your PTA is not supported yet, see :ref:`ug_radio_mpsl_cx_custom` for guidelines on how to add your own implementation.
