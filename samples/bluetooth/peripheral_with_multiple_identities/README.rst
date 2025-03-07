.. _bt_peripheral_with_multiple_identities:

Bluetooth: Peripheral with multiple identities
##############################################

.. contents::
   :local:
   :depth: 2

The sample demonstrates how to use a single physical device to create and manage multiple advertisers, making it appear as multiple distinct devices by assigning each a unique identity.
You can use this sample to test a central device that requires connections to multiple peripheral devices when you do not have several development kits available.

Requirements
************

The sample supports the following development kits:

.. table-from-sample-yaml::

Overview
********

Each Bluetooth® device is identified by its identity address.
When a peripheral is advertising, it uses this identity address or generates an address from this address using an Identity Resolving Key (IRK).
The sample application starts multiple connectable advertisers, each with their own identity, allowing a single device to advertise as multiple devices.

This sample has the following features:

* It uses a single physical device to emulate multiple Bluetooth advertisers.
* Each advertiser is assigned a unique identity and a corresponding random static address, ensuring they can be distinctly identified.
* Advertisers are restarted upon disconnection.

Number of advertisers
=====================

The sample sets up and starts as many advertisers as specified by the :kconfig:option:`CONFIG_BT_EXT_ADV_MAX_ADV_SET` Kconfig option.
If the values specified in the configurations are not equal, additional considerations are required to ensure proper setup and restarting of each advertiser.

Creating multiple identities
============================

The :c:func:`create_id` function configures a new identity by passing an identity value within the range ``0`` to the value set in the :kconfig:option:`CONFIG_BT_ID_MAX` Kconfig option.

The identities are created using the :c:func:`bt_id_create` API, where both arguments are set to ``NULL``.
This generates a new random static address and a random IRK for the identity.
Alternatively, you can pass a specific address and IRK values can be passed to customize the identity's configuration.

Setting up and initializing advertisers
=======================================

Each advertiser is set up as a *connectable legacy advertiser* with a long advertising interval.
You can adjust these values to specific use cases using the :c:struct:`bt_le_adv_param` structure.
Set the :c:member:`bt_le_adv_param.id` field to assign a given identity to an advertiser.

The extended advertising version of the API is used as it supports multiple advertising instances and sets.
These instances are stored in an array where the advertiser's identity serves as the index, to simplify the retrieval and management.
A unique name is generated for each advertiser combining :kconfig:option:`CONFIG_BT_DEVICE_NAME` with its identity.
This is set by using the :c:func:`bt_le_ext_adv_set_data` function.

Starting an advertiser
======================

A work item is initialized for each advertiser to ensure that tasks are executed independently, preventing race conditions and avoiding blocking within callback functions.
Each advertiser's work item is submitted to the work queue, where it is started using the :c:func:`bt_le_ext_adv_start` function.
By default, this function does not attempt to resume connectable advertising after a connection is established.

Connection callbacks
====================

After the initial setup of each advertiser, the sample continues to execute asynchronously using callbacks to define its behavior.
The sample uses the following two connection callbacks (see the :c:struct:`bt_conn_cb` structure):

* Connected callback - When a connection is established, the sample prints the central's address.
* Disconnected callback - When a disconnection occurs, the central's address is printed along with the advertiser that was disconnected.
  The identity associated with the connection is retrieved by calling the :c:func:`bt_conn_get_info()` function to obtain :c:member:`bt_conn_info.id`.
  The work item (advertiser) associated with this identity is submitted to the work queue to restart the advertiser.
  The identity is used as the index to retrieve the associated advertiser from the array.

Extending the sample
====================

You can also add the following features to this sample:

* Increase the number of advertisers - Modify the configuration options listed above to support more advertisers.
* Customize individual advertisers - Extend the :c:struct:`advertiser_info` structure to customize advertising parameters, advertising data, and a name for each advertiser.
* Custom advertising data - Customize the advertising data to include additional information, in addition to supporting scan responses.
* Reuse identities for multiple advertisers - Once all identities have been initialized, the sample supports reuse for other advertisers.
* Advanced features - Extend the sample to implement other Bluetooth features or use it in conjunction with other samples.

Configuration
*************

|config|

Check and configure the following Kconfig options:

* :kconfig:option:`CONFIG_BT_ID_MAX` defines how many identities will be used.

* :kconfig:option:`CONFIG_BT_MAX_CONN` defines how many connections can be established.
  The sample expects this configuration to be set to the same value as :kconfig:option:`CONFIG_BT_ID_MAX`.

* :kconfig:option:`CONFIG_BT_EXT_ADV_MAX_ADV_SET` defines the maximum number of advertising sets available.
  The sample expects this configuration to be set to the same value as :kconfig:option:`CONFIG_BT_ID_MAX`.

* :kconfig:option:`CONFIG_BT_DEVICE_NAME` defines the base name given to each advertiser.

Building and running
********************

.. |sample path| replace:: :file:`samples/bluetooth/peripheral_with_multiple_identities`

.. include:: /includes/build_and_run_ns.txt

.. |sample_or_app| replace:: sample
.. |ipc_radio_dir| replace:: :file:`sysbuild/ipc_radio`

.. include:: /includes/ipc_radio_conf.txt

Testing
=======

|test_sample|

1. |connect_kit|
#. |connect_terminal|
#. Start the `nRF Connect for Mobile`_ application on your smartphone or tablet.

   There should now be multiple devices advertising with the name ``Nordic Peripheral ID`` followed by the ID.
#. Connect to the device and observe that the central's address is printed.
#. Disconnect from any device and observe that the correct advertiser is disconnected.
#. The disconnected advertiser is restarted.
#. In the **SCANNER** tab, observe that there are still the configured number of advertisers.

Sample output
=============

The result should look similar to the following output::

   I: SoftDevice Controller build revision:
   I: d6 da c7 ae 08 db 72 6f |......ro
   I: 2a a3 26 49 2a 4d a8 b3 |*.&I*M..
   I: 98 0e 07 7f             |....
   I: HW Platform: Nordic Semiconductor (0x0002)
   I: HW Variant: nRF52x (0x0002)
   I: Firmware: Standard Bluetooth controller (0x00) Version 58.49843 Build 3849690618
   I: Identity: FA:BB:79:57:D6:45 (random)
   I: HCI: version 6.0 (0x0e) revision 0x1055, manufacturer 0x0059
   I: LMP: version 6.0 (0x0e) subver 0x1055
   Bluetooth initialized
   Starting 20 advertisers
   Using current id: 0
   Created Nordic Peripheral ID: 0: 0x2000bce8
   Advertiser 0 successfully started
   New id: 1
   Using current id: 1
   Created Nordic Peripheral ID: 1: 0x2000bd00
   Advertiser 1 successfully started
   New id: 2
   Using current id: 2
   Created Nordic Peripheral ID: 2: 0x2000bd18
   Advertiser 2 successfully started
   New id: 3
   Using current id: 3
   Created Nordic Peripheral ID: 3: 0x2000bd30
   Advertiser 3 successfully started
   New id: 4
   Using current id: 4
   Created Nordic Peripheral ID: 4: 0x2000bd48
   Advertiser 4 successfully started
   New id: 5
   Using current id: 5
   Created Nordic Peripheral ID: 5: 0x2000bd60
   Advertiser 5 successfully started
   New id: 6
   Using current id: 6
   Created Nordic Peripheral ID: 6: 0x2000bd78
   Advertiser 6 successfully started
   New id: 7
   Using current id: 7
   Created Nordic Peripheral ID: 7: 0x2000bd90
   Advertiser 7 successfully started
   New id: 8
   Using current id: 8
   Created Nordic Peripheral ID: 8: 0x2000bda8
   Advertiser 8 successfully started
   New id: 9
   Using current id: 9
   Created Nordic Peripheral ID: 9: 0x2000bdc0
   Advertiser 9 successfully started
   New id: 10
   Using current id: 10
   Created Nordic Peripheral ID: 10: 0x2000bdd8
   Advertiser 10 successfully started
   New id: 11
   Using current id: 11
   Created Nordic Peripheral ID: 11: 0x2000bdf0
   Advertiser 11 successfully started
   New id: 12
   Using current id: 12
   Created Nordic Peripheral ID: 12: 0x2000be08
   Advertiser 12 successfully started
   New id: 13
   Using current id: 13
   Created Nordic Peripheral ID: 13: 0x2000be20
   Advertiser 13 successfully started
   New id: 14
   Using current id: 14
   Created Nordic Peripheral ID: 14: 0x2000be38
   Advertiser 14 successfully started
   New id: 15
   Using current id: 15
   Created Nordic Peripheral ID: 15: 0x2000be50
   Advertiser 15 successfully started
   New id: 16
   Using current id: 16
   Created Nordic Peripheral ID: 16: 0x2000be68
   Advertiser 16 successfully started
   New id: 17
   Using current id: 17
   Created Nordic Peripheral ID: 17: 0x2000be80
   Advertiser 17 successfully started
   New id: 18
   Using current id: 18
   Created Nordic Peripheral ID: 18: 0x2000be98
   Advertiser 18 successfully started
   New id: 19
   Using current id: 19
   Created Nordic Peripheral ID: 19: 0x2000beb0
   Advertiser 19 successfully started

Dependencies
************

The sample uses the following Zephyr libraries:

* :file:`include/kernel.h`
* :file:`include/sys/printk.h`

* :ref:`zephyr:bluetooth_api`:

  * :file:`include/bluetooth/bluetooth.h`
  * :file:`include/bluetooth/conn.h`
  * :file:`include/bluetooth/hci.h`
