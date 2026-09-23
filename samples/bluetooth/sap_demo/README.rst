.. Copyright (c) 2026 Nordic Semiconductor ASA
.. SPDX-License-Identifier: LicenseRef-Nordic-5-Clause

.. _secure_application_pairing_sample:

.. ncs-sample::
   :title: Bluetooth: Secure Application Pairing

The Secure Application Pairing (SAP) sample demonstrates certificate-backed application authentication between Bluetooth® Low Energy (LE) devices.

The sample uses a Bluetooth LE link as the transport for the SAP handshake.
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

The SAP handshake uses the following elements:

* A shared SAP certificate authority (CA) public key compiled into the image
* Per-device Elliptic Curve Digital Signature Algorithm (ECDSA) identity keys and compact SAP certificates
* Signed authentication transcript messages
* Ephemeral Elliptic Curve Diffie-Hellman (ECDH) over ``secp256r1``
* Session key derivation with HKDF-SHA256
* Advanced Encryption Standard in Galois/Counter Mode (AES-GCM) application frames with 48-bit packet counters

The sample registers a SAP callback table that receives SAP authentication, failure, payload, and disconnect events.

.. caution::
   The preset keys in :file:`src/demo_credentials.c` are insecure demonstration credentials.
   Do not reuse them in a product.
   Production firmware must use product provisioning and secure credential storage, such as PSA persistent keys or hardware-backed storage where available.

Secure transport
****************

SAP protects application traffic with AES-GCM.
Each secure frame uses a 96-bit AES-GCM nonce made from a 48-bit direction nonce base and a 48-bit monotonically increasing packet counter.

SAP sends each SAP frame over Generic Attribute Profile (GATT) as one characteristic value.
The central exchanges the maximum transmission unit (MTU) before SAP discovery.
The central starts authentication only if the negotiated Attribute Protocol (ATT) MTU is at least the value of the :c:macro:`SAP_REQUIRED_ATT_MTU` macro.
The :kconfig:option:`CONFIG_BT_SAP_MAX_APP_PAYLOAD_SIZE` option sets the SAP payload size.

Protected service
*****************

The peripheral registers the protected status service only after SAP authentication succeeds.
The central discovers and reads this service after it receives the SAP authentication event.

Configuration
*************

|config|

The sample provides the following role configuration fragments:

:file:`central.conf`
   Selects the central role.

:file:`peripheral.conf`
   Selects the peripheral role.

Configuration options
=====================

The following sample-specific Kconfig options are defined in :file:`samples/bluetooth/sap_demo/Kconfig`:

.. options-from-kconfig::
   :show-type:

.. caution::
   The :kconfig:option:`CONFIG_SAMPLE_BT_SAP_PRIVATE_KEY_FILE`, :kconfig:option:`CONFIG_SAMPLE_BT_SAP_CERTIFICATE_FILE`, and :kconfig:option:`CONFIG_SAMPLE_BT_SAP_CA_PUBLIC_KEY_FILE` options embed raw key bytes in the built image.
   Use these options only for demos and tests.

Additional configuration
========================

Check and configure the following library option used by the sample:

* :kconfig:option:`CONFIG_BT_SAP_MAX_APP_PAYLOAD_SIZE` - Sets the maximum plaintext application payload accepted by SAP secure-frame APIs.

Building and running
********************

.. |sample path| replace:: :file:`samples/bluetooth/sap_demo`

.. include:: /includes/build_and_run_ns.txt

.. |sample_or_app| replace:: sample
.. |ipc_radio_dir| replace:: :file:`sysbuild/ipc_radio`

.. include:: /includes/ipc_radio_conf.txt

.. _secure_application_pairing_sample_roles:

Selecting the role
==================

You must build the sample twice, once for each role, and program each image to a different development kit.
To select the role, set :makevar:`EXTRA_CONF_FILE` to one of the role configuration fragments using the respective :ref:`CMake option <cmake_options>`:

* For the central role, set it to :file:`central.conf`.
* For the peripheral role, set it to :file:`peripheral.conf`.

For example, when building on the command line for the nRF54L15 DK, use a separate build directory for each role:

.. code-block:: console

   west build -b nrf54l15dk/nrf54l15/cpuapp -d build_central samples/bluetooth/sap_demo -- -DEXTRA_CONF_FILE=central.conf
   west build -b nrf54l15dk/nrf54l15/cpuapp -d build_peripheral samples/bluetooth/sap_demo -- -DEXTRA_CONF_FILE=peripheral.conf

Then program each image to its own development kit:

.. code-block:: console

   west flash -d build_central --dev-id <central_serial_number>
   west flash -d build_peripheral --dev-id <peripheral_serial_number>

If you build more than one peripheral, give each one a unique identity by setting the :kconfig:option:`CONFIG_SAMPLE_BT_SAP_PERIPHERAL_ID` Kconfig option to a different value from ``1`` to ``4``.
For example:

.. code-block:: console

   west build -b nrf54l15dk/nrf54l15/cpuapp -d build_peripheral_2 samples/bluetooth/sap_demo -- -DEXTRA_CONF_FILE=peripheral.conf -DCONFIG_SAMPLE_BT_SAP_PERIPHERAL_ID=2

For more information about configuration files in the |NCS|, see :ref:`app_build_system`.

Testing
*******

After programming the central and peripheral images to two devices, test the sample by completing the following steps:

1. |connect_terminal_both|
#. Reset the peripheral device.
#. Reset the central device.
#. Observe the output in the central terminal.
   The central reports that it has done the following:

   * Connected to the peripheral.
   * Completed SAP authentication.
   * Read the protected status service.
   * Received the peripheral secure text payload.
   * Sent the central secure text payload.

#. Observe the output in the peripheral terminal.
   The peripheral reports that it has done the following:

   * Authenticated the central.
   * Registered the protected status service.
   * Sent the peripheral secure text payload.
   * Received the central secure text payload.

Limitations
***********

* The sample uses preset demonstration credentials unless you select demo or test credential files with Kconfig options.
* The sample peripheral supports only one connection, so that it can hide the protected service with dynamic registration.
* The sample uses custom GATT characteristics for the SAP transport.
* The sample does not implement a firmware update.
  For firmware update flows, use encrypted images and application policy appropriate for the product.
