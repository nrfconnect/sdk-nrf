.. _ble_mesh_dfu_target:

Bluetooth: Mesh Device Firmware Update (DFU) target
###################################################

.. contents::
   :local:
   :depth: 2

The Bluetooth® mesh DFU target sample demonstrates how to update device firmware over Bluetooth mesh network.
The sample implements the Target role of the :ref:`Bluetooth mesh DFU subsystem <zephyr:bluetooth_mesh_dfu>`.

The specification that the Bluetooth mesh DFU subsystem is based on is not adopted yet, and therefore this feature should be used for experimental purposes only.

Requirements
************

The sample supports the following development kits:

.. table-from-sample-yaml::

For provisioning and configuring of the mesh model instances, the sample requires a smartphone with Nordic Semiconductor's nRF Mesh mobile app installed in one of the following versions:

* `nRF Mesh mobile app for Android`_
* `nRF Mesh mobile app for iOS`_

For uploading an image to the Distributor, this sample also requires a smartphone with Nordic Semiconductor's nRF Connect Device Manager mobile app installed in one of the following versions:

* `nRF Device Manager mobile app for Android`_
* `nRF Device Manager mobile app for iOS`_

Overview
********

This sample can be used as a base image or be transferred over Bluetooth mesh to update existing nodes.

To distribute this sample as a new image over Bluetooth mesh network, use the :ref:`ble_mesh_dfu_distributor` sample.

Provisioning
============

The sample supports provisioning over both the Advertising and the GATT Provisioning Bearers (i.e.
PB-ADV and PB-GATT).
The provisioning is handled by the :ref:`bt_mesh_dk_prov`.
It supports four types of out-of-band (OOB) authentication methods, and uses the Hardware Information driver to generate a deterministic UUID to uniquely represent the device.

Use `nRF Mesh mobile app`_ for provisioning and configuring of models supported by the sample.

Models
======

The following table shows the mesh composition data for this sample:

.. table::
   :align: left

   +------------------------+
   | Element 1              |
   +========================+
   | Config Server          |
   +------------------------+
   | Health Server          |
   +------------------------+
   | BLOB Transfer Server   |
   +------------------------+
   | Firmware Update Server |
   +------------------------+

The models are used for the following purposes:

* Config Server allows configurator devices to configure the node remotely.
* Health Server provides ``attention`` callbacks that are used during provisioning to call your attention to the device.
  These callbacks trigger blinking of the LEDs.
* Binary Large Object (BLOB) Transfer models are the underlying transport mechanism for the mesh DFU feature.
  BLOB Transfer Server is instantiated on the primary element and used to receive the firmware image binary from the Distributor node.
* The :ref:`zephyr:bluetooth_mesh_dfu_srv` model is instantiated on the primary element.
  Together with the extended BLOB Transfer Server model, the Firmware Update Server model implements all the required functionality for receiving firmware updates over the mesh network.

Configuration
*************

|config|

Source file setup
=================

This sample is split into the following source files:

* A :file:`main.c` file to handle Bluetooth mesh initialization, including the model handling for Device Composition Data, Health and Configuration Server models.
* File :file:`dfu_target.c` with the Target role implementation.
* File :file:`dfu_dist.c` with the Distributor role implementation.
* File :file:`smp_bt.c` implementing SMP Bluetooth service advertisement.

Building and running
********************

.. |sample path| replace:: :file:`samples/bluetooth/mesh/mesh_dfu/target`

.. include:: /includes/build_and_run.txt

Testing
=======

This sample has been tested with the nRF52840 DK (nrf52840dk_nrf52840) board.

.. _ble_mesh_dfu_target_provisioning:

Provisioning the device
-----------------------

.. |device name| replace:: :guilabel:`Mesh DFU Target`

.. include:: /includes/mesh_device_provisioning.txt

.. _ble_mesh_dfu_target_model_config:

Configuring models
------------------

See :ref:`ug_bt_mesh_model_config_app` for details on how to configure the mesh models with the nRF Mesh mobile app.

Configure the BLOB Transfer Server model and the Firmware Update Server on the primary element on the **Mesh DFU Target** node:

* Bind each model to **Application Key 1**.

Once the models are bound to the application key, together they implement all the required functionality for receiving firmware updates over the mesh network from the Distributor node.

.. _ble_mesh_dfu_target_upgrade:

Performing a Device Firmware Update
-----------------------------------

This sample can be transferred as a DFU over a mesh network to update the existing nodes.
The sample can also be the Target node updated by any firmware image that is compiled as the MCUboot application and transferred over the mesh network.
In both cases, the firmware needs to be signed and the firmware version increased to pass the validation when the MCUboot swaps the images.

To set a new version, alter the Kconfig option :kconfig:option:`CONFIG_MCUBOOT_IMAGE_VERSION` to a version that is higher than the default version of: ``"1.0.0+0"``.
Then, after rebuilding the sample, the binary of the updated sample can be found in :file:`samples/bluetooth/mesh_dfu/target/build/zephyr/app_update.bin`.

To perform a DFU with this sample, the following additional information is required:

Firmware ID
   Firmware ID used in this sample corresponds to the image version that is encoded in the format defined by the :c:struct:`mcuboot_img_sem_ver` structure.
   For example, when the new version is ``2.0.0+0``, the encoded value will be ``0200000000000000``.

Firmware metadata
   This sample enables the :kconfig:option:`CONFIG_BT_MESH_DFU_METADATA` option and uses the format defined by the :ref:`Bluetooth mesh DFU subsystem<zephyr:bluetooth_mesh_dfu>`.
   How to generate valid metadata for this sample is described in :ref:`bluetooth_mesh_dfu_eval_md`.

The firmware distribution process starts on a target node with checking a metadata supplied with a new firmware.
If the metadata data is decoded successfully, the following checks are performed in this sample:

* That the new firmware version is higher than the existing one.
* That the new firmware fits into the flash storage.

If the metadata check completes successfully, the sample selects a value from :c:enum:`bt_mesh_dfu_effect` depending on whether the composition data changes after programming the new firmware or not.
This has an effect on the Distributor and the Target node.
Only 2 options are supported by this sample:

:c:enum:`BT_MESH_DFU_EFFECT_NONE`
   This effect is chosen if the composition data of the new firmware doesn't change.
   In this case the device will stay provisioned after the new firmware is programmed.

:c:enum:`BT_MESH_DFU_EFFECT_UNPROV`
   This effect is chosen if the composition data in the new firmware changes.
   In this case, the device unprovisions itself before programming the new firmware.
   The unprovisioning happens before the device reboots, so if the MCUboot fails to validate the new firmware, the device will boot unprovisioned anyway.

In this sample, the device flash is split into fixed partitions using devicetree as defined in :zephyr_file:`nrf52840dk_nrf52840.dts<boards/arm/nrf52840dk_nrf52840/nrf52840dk_nrf52840.dts>`.
When the DFU transfer starts, the sample stores the new firmware at slot-1 using :ref:`zephyr:flash_map_api`.

When the DFU transfer ends, the sample requests the MCUboot to replace slot-0 with slot-1 and reboots the device.
The MCUboot performs the validation of the image located at slot-1.
Upon successful validation, the MCUboot replaces the old firmware with the new one and boots it.
After booting, the sample confirms the image so the old image does not get reverted at the next reboot.

When the sample is used as a new firmware, independently of the provisioning state, it sets the Firmware Update Server model to Idle state after booting.
If the device stays provisioned, it lets the Distributor successfully finalize the firmware distribution process.
If the device is unprovisioned, it has no effect on the DFU Server.
The firmware distribution process then succeeds on the Distributor side, if the Target node doesn't respond to the Distributor after programming the new firmware.

For more information about the firmware distribution process, see :ref:`zephyr:bluetooth_mesh_dfu`.

Logging
=======

In this sample, UART and SEGGER RTT are available as logging backends.

Dependencies
************

This sample uses the following |NCS| libraries:

* :ref:`bt_mesh_dk_prov`
* :ref:`dk_buttons_and_leds_readme`

It also requires :ref:`MCUboot <mcuboot_ncs>` and :ref:`zephyr:mcu_mgr`.

In addition, it uses the following Zephyr libraries:

* :ref:`zephyr:bluetooth_mesh`:

  * :file:`include/bluetooth/mesh.h`
