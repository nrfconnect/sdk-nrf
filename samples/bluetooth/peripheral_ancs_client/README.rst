.. _peripheral_ancs_client:

Bluetooth: Peripheral ANCS client
#################################

.. contents::
   :local:
   :depth: 2

The Peripheral ANCS client sample demonstrates how to use the :ref:`ancs_client_readme`.

Requirements
************

The sample supports the following development kits:

.. table-from-sample-yaml::

.. include:: /includes/tfm.txt

The sample also requires a device running an ANCS Server to connect with (for example, an iPhone which runs iOS, or a Bluetooth® Low Energy dongle and the `Bluetooth Low Energy app`_).

User interface
**************

.. tabs::

   .. group-tab:: nRF52 and nRF53 DKs

      LED 1:
         Blinks, toggling on/off every second, when the main loop is running and the device is advertising.

      LED 2:
         Lit when connected.

      Button 1:
         Request iOS notification attributes (content) on UART.

      Button 2:
         Request iOS app attributes on UART.

      Button 3:
         Perform a positive action as a response to the last received notification.

      Button 4:
         Perform a negative action as a response to the last received notification.

   .. group-tab:: nRF54 DKs

      LED 0:
         Blinks, toggling on/off every second, when the main loop is running and the device is advertising.

      LED 1:
         Lit when connected.

      Button 0:
         Request iOS notification attributes (content) on UART.

      Button 1:
         Request iOS app attributes on UART.

      Button 2:
         Perform a positive action as a response to the last received notification.

      Button 3:
         Perform a negative action as a response to the last received notification.

Building and running
********************

.. |sample path| replace:: :file:`samples/bluetooth/peripheral_ancs_client`

.. include:: /includes/build_and_run_ns.txt

.. _peripheral_ancs_client_testing:

Testing
=======

After programming the sample to your development kit, you can test it either by connecting to an iOS device or by using the `Bluetooth Low Energy app`_ that emulates an ANCS Server.

Testing with an iOS device
--------------------------

.. tabs::

   .. group-tab:: nRF52 and nRF53 DKs

      #. |connect_terminal_specific|
      #. Reset the kit.
      #. Select the device in the iOS settings Bluetooth menu and connect.
      #. Observe that notifications that are displayed in the iOS notification tab also show up on the UART from the sample.
      #. Press **Button 1** to retrieve the notification attributes and observe that you receive, among other information, the app identifier for the last received notification.
         For example, if you got a notification from the Calendar app and request the app identifier, it is "com.apple.mobilecal".
      #. Press **Button 2** to retrieve the app attributes and observe that you receive the display name for the app identifier from the previous step.
         For example, requesting the app attributes for "com.apple.mobilecal" yields "Calendar".
      #. If the notification has a flag for a positive or negative action, perform the notification action with **Button 3** or **Button 4**, respectively.

   .. group-tab:: nRF54 DKs

      #. |connect_terminal_specific|
      #. Reset the kit.
      #. Select the device in the iOS settings Bluetooth menu and connect.
      #. Observe that notifications that are displayed in the iOS notification tab also show up on the UART from the sample.
      #. Press **Button 0** to retrieve the notification attributes and observe that you receive, among other information, the app identifier for the last received notification.
         For example, if you got a notification from the Calendar app and request the app identifier, it is "com.apple.mobilecal".
      #. Press **Button 1** to retrieve the app attributes and observe that you receive the display name for the app identifier from the previous step.
         For example, requesting the app attributes for "com.apple.mobilecal" yields "Calendar".
      #. If the notification has a flag for a positive or negative action, perform the notification action with **Button 2** or **Button 3**, respectively.

Testing with Bluetooth Low Energy app
-------------------------------------

#. |connect_terminal_specific|
#. Reset the kit.
#. Start `nRF Connect for Desktop`_.
#. Open the `Bluetooth Low Energy app`_ and select the connected dongle that is used for communication.
#. Click the :guilabel:`SERVER SETUP` tab.
   Click the dongle configuration and select :guilabel:`Load setup`.
   Load the :file:`ANCS_central.ncs` file that is located under :file:`samples/bluetooth/peripheral_ancs_client` in the |NCS| folder structure.
#. Click :guilabel:`Apply to device`.
#. Click the :guilabel:`CONNECTION MAP` tab.
   Click the dongle configuration and select :guilabel:`Security parameters`.
   Check :guilabel:`Perform Bonding` and :guilabel:`Enable LE Secure Connection pairing`, and click :guilabel:`Apply`.
#. Connect to the device from the app.
   The device is advertising as ``Nordic_ANCS``.
#. Wait until the bond is established. Verify that the UART data is received as follows::

      Connected xx:xx:xx:xx:xx:xx (random)
      Security changed: xx:xx:xx:xx:xx:xx (random) level 2
      Pairing completed: xx:xx:xx:xx:xx:xx (random), bonded: 1
      GATT Service could not be found during the discovery
      The discovery procedure for ANCS succeeded

#. After bonding, verify in the app that the **Client Characteristic Configuration** (CCCD) value for **Apple Notification Source** and **Apple Data Source** are set to ``01 00``.

Send an iOS notification to the application
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

The following table shows the format of a notification that you can send to the sample application:

   +------------------+---------------+--------------------------+
   | Field            | Example value | Interpretation           |
   +==================+===============+==========================+
   | Event ID         | 0             | Notification added       |
   +------------------+---------------+--------------------------+
   | Flags            | 18            | Positive/negative action |
   +------------------+---------------+--------------------------+
   | Category         | 06            | Email                    |
   +------------------+---------------+--------------------------+
   | Category count   | 02            |                          |
   +------------------+---------------+--------------------------+
   | Notification UID | 01 02 03 04   | 67305985 (0x4030201)     |
   +------------------+---------------+--------------------------+

#. In the `Bluetooth Low Energy app`_, set the value of **Apple Notification Source** to ``00 18 06 02 01 02 03 04`` and click :guilabel:`Update`.
#. Verify that the UART data is received as follows::

      Notification
      Event:       Added
      Category ID: Email
      Category Cnt:2
      UID:         67305985
      Flags:
       Positive Action
       Negative Action

Perform a notification action
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

The received notification has two flags: Positive Action and Negative Action.
This means that you can perform these two actions on the notification.

The following table shows the format of the message that the application must send back to perform a notification action:

   +------------------+---------------+-----------------------------+
   | Field            | Example value | Interpretation              |
   +==================+===============+=============================+
   | Command ID       | 2             | Perform notification action |
   +------------------+---------------+-----------------------------+
   | Notification UID | 01 02 03 04   | 67305985 (0x4030201)        |
   +------------------+---------------+-----------------------------+
   | Action           | 00            | Positive                    |
   |                  |               |                             |
   |                  | 01            | Negative                    |
   +------------------+---------------+-----------------------------+

.. tabs::

   .. group-tab:: nRF52 and nRF53 DKs

      * Press **Button 3** to perform a positive action and verify that the **Apple Control Point** value is updated to ``02 01 02 03 04 00``.
      * You can also press **Button 4** and observe that the server receives a negative action ``02 01 02 03 04 01``.

   .. group-tab:: nRF54 DKs

      * Press **Button 2** to perform a positive action and verify that the **Apple Control Point** value is updated to ``02 01 02 03 04 00``.
      * You can also press **Button 3** and observe that the server receives a negative action ``02 01 02 03 04 01``.

Retrieve notification attributes
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

The following table shows the relevant part of a request to retrieve notification attributes:

   +------------------+---------------+-----------------------------+
   | Field            | Example value | Interpretation              |
   +==================+===============+=============================+
   | Command ID       | 0             | Get notification attributes |
   +------------------+---------------+-----------------------------+
   | Notification UID | 01 02 03 04   | 67305985 (0x4030201)        |
   +------------------+---------------+-----------------------------+
   | Attribute ID     | 00            | App identifier              |
   +------------------+---------------+-----------------------------+
   | Attribute ID     | 01            | Title                       |
   +------------------+---------------+-----------------------------+
   | Length           | 20 00         | 0x0020                      |
   +------------------+---------------+-----------------------------+
   | Attribute ID     | 03            | Message                     |
   +------------------+---------------+-----------------------------+
   | Length           | 20 00         | 0x0020                      |
   +------------------+---------------+-----------------------------+

Note that the sample application will request all existing attribute types, not only a subset.

The following table shows the format of a response that contains some of the requested notification attributes:

   +------------------+---------------+-----------------------------+
   | Field            | Example value | Interpretation              |
   +==================+===============+=============================+
   | Command ID       | 0             | Get notification attributes |
   +------------------+---------------+-----------------------------+
   | Notification UID | 01 02 03 04   | 67305985 (0x4030201)        |
   +------------------+---------------+-----------------------------+
   | Attribute ID     | 01            | Title                       |
   +------------------+---------------+-----------------------------+
   | Length           | 03 00         | 0x0003                      |
   +------------------+---------------+-----------------------------+
   | Data             | 6E 52 46      | "nRF"                       |
   +------------------+---------------+-----------------------------+
   | Attribute ID     | 03            | Message                     |
   +------------------+---------------+-----------------------------+
   | Length           | 02 00         | 0x0002                      |
   +------------------+---------------+-----------------------------+
   | Data             | 35 32         | "52"                        |
   +------------------+---------------+-----------------------------+
   | Attribute ID     | 00            | App identifier              |
   +------------------+---------------+-----------------------------+
   | Length           | 03 00         | 0x0003                      |
   +------------------+---------------+-----------------------------+
   | Data             | 63 6F 6D      | "com"                       |
   +------------------+---------------+-----------------------------+


.. tabs::

   .. group-tab:: nRF52 and nRF53 DKs

      #. Press **Button 1** to request notification attributes for the iOS notification that was received.
      #. In the `Bluetooth Low Energy app`_, verify that the **Apple Control Point** is updated to ``00 01 02 03 04 00 01 20 00 02 20 00 03 20 00 04 05 06 07``.
      #. Respond to the request by sending two notification attributes: the title and the message.
         Set the **Apple Data Source** value in the Server to ``00 01 02 03 04 01 03 00 6E 52 46 03 02 00 35 32``.
      #. The application will print the received data on UART.
         Verify that the UART output is as follows::

            Title: nRF
            Message: 52

      #. Update the **Apple Data Source** value again with the app identifier ``00 03 00 63 6F 6D``.
      #. Verify that the notification is received and the UART output is as follows::

            App Identifier: com'

   .. group-tab:: nRF54 DKs

      #. Press **Button 0** to request notification attributes for the iOS notification that was received.
      #. In the `Bluetooth Low Energy app`_, verify that the **Apple Control Point** is updated to ``00 01 02 03 04 00 01 20 00 02 20 00 03 20 00 04 05 06 07``.
      #. Respond to the request by sending two notification attributes: the title and the message.
         Set the **Apple Data Source** value in the Server to ``00 01 02 03 04 01 03 00 6E 52 46 03 02 00 35 32``.
      #. The application will print the received data on UART.
         Verify that the UART output is as follows::

            Title: nRF
            Message: 52

      #. Update the **Apple Data Source** value again with the app identifier ``00 03 00 63 6F 6D``.
      #. Verify that the notification is received and the UART output is as follows::

            App Identifier: com

Retrieve app attributes
^^^^^^^^^^^^^^^^^^^^^^^

With the app identifier, you can request attributes of the app that sent the notification.

The following table shows the format of a request to retrieve app attributes:

   +------------------+---------------+--------------------+
   | Field            | Example value | Interpretation     |
   +==================+===============+====================+
   | Command ID       | 1             | Get app attributes |
   +------------------+---------------+--------------------+
   | App identifier   | 6D 6F 63 00   | "com" + '\0'       |
   +------------------+---------------+--------------------+
   | Attribute ID     | 0             | Display name       |
   +------------------+---------------+--------------------+

The following table shows the format of a response that contains the requested app attributes:

   +------------------+---------------+--------------------+
   | Field            | Example value | Interpretation     |
   +==================+===============+====================+
   | Command ID       | 1             | Get app attributes |
   +------------------+---------------+--------------------+
   | App identifier   | 6D 6F 63 00   | "com" + '\0'       |
   +------------------+---------------+--------------------+
   | Attribute ID     | 0             | Display name       |
   +------------------+---------------+--------------------+
   | Length           | 04 00         | 0x0004             |
   +------------------+---------------+--------------------+
   | Data             | 4D 61 69 6C   | "Mail"             |
   +------------------+---------------+--------------------+


.. tabs::

   .. group-tab:: nRF52 and nRF53 DKs

      #. Press **Button 2** to request app attributes for the app with the app identifier "com" (the last received app identifier).
      #. In the `Bluetooth Low Energy app`_, verify that the **Apple Control Point** is updated to ``01 63 6F 6D 00 00``.
      #. Respond to the request by sending the app attribute.
         Set the **Apple Data Source** value in the Server to ``01 63 6F 6D 00 00 04 00 4D 61 69 6C``.
      #. The application will print the received data on UART. Verify that the UART output is as follows::

            Display Name: Mail

   .. group-tab:: nRF54 DKs

      #. Press **Button 1** to request app attributes for the app with the app identifier "com" (the last received app identifier).
      #. In the `Bluetooth Low Energy app`_, verify that the **Apple Control Point** is updated to ``01 63 6F 6D 00 00``.
      #. Respond to the request by sending the app attribute.
         Set the **Apple Data Source** value in the Server to ``01 63 6F 6D 00 00 04 00 4D 61 69 6C``.
      #. The application will print the received data on UART. Verify that the UART output is as follows::

            Display Name: Mail

Disconnect
^^^^^^^^^^

Disconnect the device in the `Bluetooth Low Energy app`_.
As the bond information is preserved by the app, you can immediately reconnect to the device by clicking :guilabel:`Connect`.

Dependencies
************

This sample uses the following |NCS| libraries:

* :ref:`ancs_client_readme`
* :ref:`gattp_readme`
* :ref:`gatt_dm_readme`
* :ref:`dk_buttons_and_leds_readme`

In addition, it uses the following Zephyr libraries:

* :file:`include/zephyr/types.h`
* :file:`lib/libc/minimal/include/errno.h`
* :file:`include/sys/printk.h`
* :ref:`zephyr:bluetooth_api`:

  * :file:`include/bluetooth/bluetooth.h`
  * :file:`include/bluetooth/conn.h`
  * :file:`include/bluetooth/uuid.h`
  * :file:`include/bluetooth/gatt.h`

The sample also uses the following secure firmware component:

* :ref:`Trusted Firmware-M <ug_tfm>`
