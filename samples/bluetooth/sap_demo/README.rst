.. Copyright (c) 2026 Nordic Semiconductor ASA
.. SPDX-License-Identifier: LicenseRef-Nordic-5-Clause

.. _secure_application_pairing_sample:

.. ncs-sample::
   :title: Bluetooth: Secure Application Pairing

The Secure Application Pairing sample demonstrates certificate-backed application authentication between Bluetooth® Low Energy devices.

The sample uses a Bluetooth LE link as the carrier for the SAP handshake.
After SAP authenticates both devices, the peripheral exposes a protected status service, and both roles send encrypted application messages.

Requirements
************

The sample supports the following development kits:

.. table-from-sample-yaml::

.. include:: /includes/tfm.txt

The sample requires two devices: one running the central role and one running the peripheral role.

Overview
********

The sample has two roles:

Central
   Scans for SAP peripherals, connects, runs SAP authentication, reads the protected status service, and sends a secure text payload.

Peripheral
   Advertises the SAP service, authenticates the central, and registers the protected status service only after SAP succeeds.
   It also sends a secure text payload to the central after authentication.

The SAP handshake uses:

* A shared SAP CA public key compiled into the image.
* Per-device ECDSA identity keys and compact SAP certificates.
* Signed authentication transcript messages.
* Ephemeral ECDH over ``secp256r1``.
* HKDF-SHA256 session key derivation.
* AES-GCM application frames with 48-bit packet counters.

SAP authentication, failure, payload, and disconnect events are delivered through the SAP callback table registered by the sample.

.. caution::
   The preset keys in :file:`src/demo_credentials.c` are insecure demonstration credentials.
   Do not reuse them in a product.
   The :kconfig:option:`CONFIG_SAMPLE_BT_SAP_PRIVATE_KEY_FILE`, :kconfig:option:`CONFIG_SAMPLE_BT_SAP_CERTIFICATE_FILE`, and :kconfig:option:`CONFIG_SAMPLE_BT_SAP_CA_PUBLIC_KEY_FILE` options also embed raw key bytes in the built image and are intended only for demos and tests.
   Production firmware must use product provisioning and secure credential storage, such as PSA persistent keys or hardware-backed storage where available.

Secure transport
****************

SAP protects application traffic with AES-GCM.
Each secure frame uses a 96-bit GCM nonce made from a 48-bit direction nonce base and a 48-bit monotonically increasing packet counter.

SAP sends each GATT-carried SAP frame as one characteristic value.
The central exchanges MTU before SAP discovery and only starts authentication if the negotiated ATT MTU is at least the value of the :c:macro:`SAP_REQUIRED_ATT_MTU` macro.
The SAP payload size is configured with the :kconfig:option:`CONFIG_BT_SAP_MAX_APP_PAYLOAD_SIZE` option.

Protected service
*****************

The peripheral registers the protected status service only after SAP authentication succeeds.
The central discovers and reads this service after it receives the SAP authentication event.

Firmware update flows should use encrypted images and application policy appropriate for the product.
This sample does not implement a firmware update.

Configuration
*************

|config|

The sample provides role configuration fragments:

``central.conf``
   Selects the central role.

``peripheral.conf``
   Selects the peripheral role.

Configuration options
=====================

The following sample-specific Kconfig options are defined in :file:`samples/bluetooth/sap_demo/Kconfig`:

.. options-from-kconfig::
   :show-type:

Additional configuration
========================

Check and configure the following library option used by the sample:

* :kconfig:option:`CONFIG_BT_SAP_MAX_APP_PAYLOAD_SIZE` - Sets the maximum plaintext application payload accepted by SAP secure-frame APIs.

Building and running
********************

.. |sample path| replace:: :file:`samples/bluetooth/sap_demo`

.. include:: /includes/build_and_run.txt

Testing
*******

After programming the central and peripheral images to two devices, test the sample by completing the following steps:

1. Connect a terminal to each device.
#. Reset the peripheral device.
#. Reset the central device.
#. Observe that the central terminal reports that it connected to the peripheral, completed SAP authentication, read the protected status service, received the peripheral secure text payload, and sent the central secure text payload.
#. Observe that the peripheral terminal reports that it authenticated the central, registered the protected status service, sent the peripheral secure text payload, and received the central secure text payload.

Limitations
***********

* The sample uses preset demonstration credentials unless demo or test credential files are selected with Kconfig options.
* The sample peripheral is intentionally single-connection so the protected service can be hidden with dynamic registration.
* The sample uses custom GATT characteristics for the SAP transport.
