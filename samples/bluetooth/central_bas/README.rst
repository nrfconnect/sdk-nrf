.. _central_bas:

Bluetooth: Central BAS
######################

.. contents::
   :local:
   :depth: 2

The Central BAS sample demonstrates how do use the :ref:`bas_client_readme`.
It uses the BAS Client to receive battery level information from a compatible device.

Requirements
************

The sample supports the following development kits:

.. table-from-sample-yaml::

.. include:: /includes/tfm.txt

The sample also requires a device running a BAS Server to connect with (for example, another development kit running the :ref:`peripheral_hids_mouse` or :ref:`peripheral_hids_keyboard` sample, or a computer with a Bluetooth® Low Energy dongle and the `Bluetooth Low Energy app`_).

Overview
********

When connected, the sample subscribes to battery level notifications.
Every notification that is received is printed to the terminal.

.. tabs::

   .. group-tab:: nRF52 and nRF53 DKs

      If the device does not support notifications for the Battery Level Characteristic, press **Button 1** to request for reading the battery level.

   .. group-tab:: nRF54 DKs

      If the device does not support notifications for the Battery Level Characteristic, press **Button 0** to request for reading the battery level.

User interface
**************

.. tabs::

   .. group-tab:: nRF52 and nRF53 DKs

      Button 1:
         Send read request for the battery level value.

   .. group-tab:: nRF54 DKs

      Button 0:
         Send read request for the battery level value.

Building and running
********************

.. |sample path| replace:: :file:`samples/bluetooth/central_bas`

.. include:: /includes/build_and_run_ns.txt

.. _central_bas_testing:

Testing
=======

After programming the sample to your development kit, you can test it either by connecting to another kit that is running the :ref:`peripheral_hids_keyboard` or :ref:`peripheral_hids_mouse` sample, or by using the `Bluetooth Low Energy app`_ that emulates a BAS Server.


Testing with another kit
------------------------

.. tabs::

   .. group-tab:: nRF52 and nRF53 DKs

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

   .. group-tab:: nRF54 DKs

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

      #. Press **Button 0** to send a read request and process the response::

            Reading BAS value:
            [xx.xx.xx.xx.xx.xx (random)]: Battery read: 97%


Testing with Bluetooth Low Energy app
-------------------------------------

.. tabs::

   .. group-tab:: nRF52 and nRF53 DKs

      1. |connect_terminal_specific|
      #. Reset the kit.
      #. Start `nRF Connect for Desktop`_.
      #. Open the `Bluetooth Low Energy app`_ and select the connected dongle that is used for communication.
      #. Open the :guilabel:`SERVER SETUP` tab.
         Click the dongle configuration and select :guilabel:`Load setup`.
         Load the :file:`hids_keyboard.ncs` file that is located under :file:`samples/bluetooth/central_bas` in the |NCS| folder structure.
      #. Click :guilabel:`Apply to device`.
      #. Open the :guilabel:`CONNECTION MAP` tab.
         Click the dongle configuration and select :guilabel:`Advertising setup`.

         The current version of the app cannot store the advertising setup, so it must be configured manually.
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

      #. In the **Adapter settings**, select :guilabel:`Start advertising`.
      #. Wait until the kit that runs the Central BAS sample connects.
         In the terminal window, check for information similar to the following::

            The discovery procedure succeeded

      #. Press **Button 1** to send read request and process the response::

            Reading BAS value:
            [xx.xx.xx.xx.xx.xx (random)]: Battery read: 100%

      #. Change the value in :guilabel:`Battery Service` > :guilabel:`Battery Level` to generate notifications.
      #. Observe that the notification information is displayed::

            [xx.xx.xx.xx.xx.xx (random)]: Battery notification: 99%

   .. group-tab:: nRF54 DKs

      1. |connect_terminal_specific|
      #. Reset the kit.
      #. Start `nRF Connect for Desktop`_.
      #. Open the `Bluetooth Low Energy app`_ and select the connected dongle that is used for communication.
      #. Open the :guilabel:`SERVER SETUP` tab.
         Click the dongle configuration and select :guilabel:`Load setup`.
         Load the :file:`hids_keyboard.ncs` file that is located under :file:`samples/bluetooth/central_bas` in the |NCS| folder structure.
      #. Click :guilabel:`Apply to device`.
      #. Open the :guilabel:`CONNECTION MAP` tab.
         Click the dongle configuration and select :guilabel:`Advertising setup`.

         The current version of the app cannot store the advertising setup, so it must be configured manually.
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

      #. In the **Adapter settings**, select :guilabel:`Start advertising`.
      #. Wait until the kit that runs the Central BAS sample connects.
         In the terminal window, check for information similar to the following::

            The discovery procedure succeeded

      #. Press **Button 0** to send read request and process the response::

            Reading BAS value:
            [xx.xx.xx.xx.xx.xx (random)]: Battery read: 100%

      #. Change the value in :guilabel:`Battery Service` > :guilabel:`Battery Level` to generate notifications.
      #. Observe that the notification information is displayed::

            [xx.xx.xx.xx.xx.xx (random)]: Battery notification: 99%

Dependencies
************

This sample uses the following |NCS| libraries:

* :ref:`bas_client_readme`
* :ref:`gatt_dm_readme`
* :ref:`nrf_bt_scan_readme`

In addition, it uses the following Zephyr libraries:

* :file:`include/zephyr/types.h`
* :file:`boards/arm/nrf*/board.h`
* :ref:`zephyr:kernel_api`:

  * :file:`include/kernel.h`

* :ref:`zephyr:api_peripherals`:

   * :file:`include/uart.h`

* :ref:`zephyr:bluetooth_api`:

  * :file:`include/bluetooth/bluetooth.h`
  * :file:`include/bluetooth/gatt.h`
  * :file:`include/bluetooth/hci.h`
  * :file:`include/bluetooth/uuid.h`

The sample also uses the following secure firmware component:

* :ref:`Trusted Firmware-M <ug_tfm>`
