.. _central_bas:

Bluetooth: Central BAS
######################

.. contents::
   :local:
   :depth: 2

The Central BAS sample demonstrates how do use the :ref:`bas_client_readme`.
It uses the BAS Client to receive battery level information from a compatible device.


Overview
********

When connected, the sample subscribes to battery level notifications.
Every notification that is received is printed to the terminal.
If the device does not support notifications for the Battery Level Characteristic, you can request to read the battery level by pressing Button 1.


Requirements
************

The sample supports the following development kits:

.. table-from-rows:: /includes/sample_board_rows.txt
   :header: heading
   :rows: nrf5340dk_nrf5340_cpuapp_and_cpuappns , nrf52840dk_nrf52840, nrf52840dk_nrf52811, nrf52833dk_nrf52833, nrf52833dk_nrf52820, nrf52dk_nrf52832, nrf52dk_nrf52810

The sample also requires a device running a BAS Server to connect with (for example, another development kit running the :ref:`peripheral_hids_mouse` or :ref:`peripheral_hids_keyboard` sample, or a Bluetooth Low Energy dongle and nRF Connect for Desktop)


User interface
**************

Button 1:
   Send read request for the battery level value.


Building and running
********************

.. |sample path| replace:: :file:`samples/bluetooth/central_bas`

.. include:: /includes/build_and_run.txt

.. _central_bas_testing:


Testing
=======

After programming the sample to your development kit, you can test it either by connecting to another kit that is running the :ref:`peripheral_hids_keyboard` or :ref:`peripheral_hids_mouse` sample, or by using `nRF Connect for Desktop`_ that emulates a BAS Server.


Testing with another kit
------------------------

1. |connect_terminal_specific|
#. Reset the kit.
#. Program the other development kit with the :ref:`peripheral_hids_keyboard` or :ref:`peripheral_hids_mouse` sample and reset it.
#. Wait until the BAS Server is detected by the central.
   In the terminal window, check for information similar to the following::

      The discovery procedure succeeded

#. Observe that the received notifications are output in the terminal window::

      [xx.xx.xx.xx.xx.xx (random)]: Battery notification: 99%
      [xx.xx.xx.xx.xx.xx (random)]: Battery notification: 98%
      [xx.xx.xx.xx.xx.xx (random)]: Battery notification: 97%

#. Press **Button 1** to send a read request and process the response::

      Reading BAS value:
      [xx.xx.xx.xx.xx.xx (random)]: Battery read: 97%


Testing with nRF Connect for Desktop
------------------------------------

1. |connect_terminal_specific|
#. Reset the kit.
#. Start `nRF Connect for Desktop`_ and select the connected dongle that is used for communication.
#. Go to the :guilabel:`Server setup` tab.
   Click the dongle configuration and select :guilabel:`Load setup`.
   Load the ``hids_keyboard.ncs`` file that is located under :file:`samples/bluetooth/central_bas` in the |NCS| folder structure.
#. Click :guilabel:`Apply to device`.
#. Go to the :guilabel:`Connection Map` tab.
   Click the dongle configuration and select :guilabel:`Advertising setup`.

   The current version of nRF Connect cannot store the advertising setup, so it must be configured manually.
   See the following image for the required target configuration:

   .. figure:: /images/bt_central_hids_nrfc_ad.png
      :alt: Advertising setup for HIDS keyboard simulator

   Advertising setup for HIDS keyboard simulator

   Complete the following steps to configure the advertising setup:

   a. Delete the default **Complete local name** from **Advertising data**.
   #. Add a **Custom AD type** with **AD type value** set to ``19`` and **Value** set to ``03c1``.
      This is the GAP Appearance advertising data.
   #. Add a **Custom AD type** with **AD type value** set to ``01`` and **Value** set to ``06``.
      This is the AD data with "General Discoverable" and "BR/EDR not supported" flags set.
   #. Add a **UUID 16 bit complete list** with two comma-separated values: ``1812`` and ``180F``.
      These are the values for HIDS and BAS.
   #. Add a **Complete local name** of your choice to the **Scan response data**.
   #. Click :guilabel:`Apply` and :guilabel:`Close`.

#. In the Adapter settings, choose :guilabel:`Start advertising`.
#. Wait until the kit that runs the Central BAS sample connects.
   In the terminal window, check for information similar to the following::

      The discovery procedure succeeded

#. Press **Button 1** to send read request and process the response::

      Reading BAS value:
      [xx.xx.xx.xx.xx.xx (random)]: Battery read: 100%

#. Change the value in :guilabel:`Battery Service` -> :guilabel:`Battery Level` to generate notifications.
#. Observe that the notification information is displayed::

      [xx.xx.xx.xx.xx.xx (random)]: Battery notification: 99%


Dependencies
************

This sample uses the following |NCS| libraries:

* :ref:`bas_client_readme`
* :ref:`gatt_dm_readme`
* :ref:`nrf_bt_scan_readme`

In addition, it uses the following Zephyr libraries:

* ``include/zephyr/types.h``
* ``boards/arm/nrf*/board.h``
* :ref:`zephyr:kernel_api`:

  * ``include/kernel.h``

* :ref:`zephyr:api_peripherals`:

   * ``include/uart.h``

* :ref:`zephyr:bluetooth_api`:

  * ``include/bluetooth/bluetooth.h``
  * ``include/bluetooth/gatt.h``
  * ``include/bluetooth/hci.h``
  * ``include/bluetooth/uuid.h``
