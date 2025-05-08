.. _peripheral_ams_client:

Bluetooth: Peripheral AMS client
################################

.. contents::
   :local:
   :depth: 2

The Peripheral AMS client sample demonstrates how to use the :ref:`ams_client_readme`.

Requirements
************

The sample supports the following development kits:

.. table-from-sample-yaml::

.. include:: /includes/tfm.txt

The sample also requires a device running an AMS Server to connect with (for example, an iPhone, or a computer with a Bluetooth® Low Energy dongle and the `Bluetooth Low Energy app`_).

Overview
********

The AMS client sample serves a Media Remote (MR).
It can interact with a Media Source (MS), typically an iPhone or some other Apple device.

The sample prints the media details received from the MS on the UART.
It can request the MS to perform certain actions, such as starting playback and jumping to next track.

User interface
**************

.. tabs::

   .. group-tab:: nRF52 and nRF53 DKs

      LED 1:
         Blinks, toggling on/off every second, when the main loop is running and the device is advertising.

      LED 2:
         Lit when connected.

      Button 1:
         Enable and disable MS track attributes notification.

      Button 2:
         Request MS to start or pause playback.

      Button 3:
         Request MS to jump to the next track if it is supported.
         Otherwise, request MS to turn up the volume.

      Button 4:
         Request MS to jump to the previous track if it is supported.
         Otherwise, request MS to turn down the volume.

   .. group-tab:: nRF54 DKs

      LED 0:
         Blinks, toggling on/off every second, when the main loop is running and the device is advertising.

      LED 1:
         Lit when connected.

      Button 0:
         Enable and disable MS track attributes notification.

      Button 1:
         Request MS to start or pause playback.

      Button 2:
         Request MS to jump to the next track if it is supported.
         Otherwise, request MS to turn up the volume.

      Button 3:
         Request MS to jump to the previous track if it is supported.
         Otherwise, request MS to turn down the volume.

Building and running
********************

.. |sample path| replace:: :file:`samples/bluetooth/peripheral_ams_client`

.. include:: /includes/build_and_run_ns.txt

.. _peripheral_ams_client_testing:

Testing
=======

After programming the sample to your development kit, you can test it either by connecting to an iOS device or by using the `Bluetooth Low Energy app`_ of `nRF Connect for Desktop`_ that emulates an AMS Server.

Testing with an iOS device
--------------------------

.. tabs::

   .. group-tab:: nRF52 and nRF53 DKs

      #. |connect_terminal_specific|
      #. Reset the kit.
      #. Select the device in the iOS :guilabel:`Settings` > :guilabel:`Bluetooth` menu > :guilabel:`Connect`.
         When requested, proceed with pairing.
      #. Open the :guilabel:`Apple Music` app and bring up a playlist with some songs.
      #. Press **Button 1** to enable song detail updates.
      #. Press **Button 2** to play a song.
         Press again to pause playback.
      #. Press **Button 3** to jump to the next song.
      #. Press **Button 4** to jump to the previous song.

   .. group-tab:: nRF54 DKs

      #. |connect_terminal_specific|
      #. Reset the kit.
      #. Select the device in the iOS :guilabel:`Settings` > :guilabel:`Bluetooth` menu > :guilabel:`Connect`.
         When requested, proceed with pairing.
      #. Open the :guilabel:`Apple Music` app and bring up a playlist with some songs.
      #. Press **Button 0** to enable song detail updates.
      #. Press **Button 1** to play a song.
         Press again to pause playback.
      #. Press **Button 2** to jump to the next song.
      #. Press **Button 3** to jump to the previous song.

Testing with Bluetooth Low Energy app
-------------------------------------

#. |connect_terminal_specific|
#. Reset the kit.
#. Start the `Bluetooth Low Energy app`_ of `nRF Connect for Desktop`_ and select the connected dongle that is used for communication.
#. Set up the server:

   a. Select the :guilabel:`SERVER SETUP` tab.
   #. Select :guilabel:`Dongle configuration` > :guilabel:`Load setup`.
   #. Load the :file:`AMS_central.ncs` file located under :file:`samples/bluetooth/peripheral_ams_client` in the |NCS| folder structure.
   #. Click :guilabel:`Apply to device`.

#. Select the :guilabel:`CONNECTION MAP` tab.
#. Select :guilabel:`Dongle configuration` > :guilabel:`Security parameters`.
#. Check :guilabel:`Perform Bonding` and :guilabel:`Enable LE Secure Connection pairing`, and click :guilabel:`Apply`.
#. Connect to the device from the app.
   The device is advertising as ``Nordic_AMS``.
#. Wait until the bond is established.
   Verify that the UART data is received as follows::

      Connected xx:xx:xx:xx:xx:xx (random)
      The discovery procedure succeeded
      Security changed: xx:xx:xx:xx:xx:xx (random) level 2
      Pairing completed: xx:xx:xx:xx:xx:xx (random), bonded: 1

#. After bonding, verify in the app that the **Client Characteristic Configuration** (CCCD) value for **Apple Remote Command** and **Apple Entity Update** are set to ``01 00``.

Music setup
^^^^^^^^^^^

.. tabs::

   .. group-tab:: nRF52 and nRF53 DKs

      Complete the following steps to initiate a music player setup:

      #. In the `Bluetooth Low Energy app`_, verify that the **Apple Entity Update** is initiated to ``00 00 01 02``.
         The following table lists the attributes requested by the MR.

         +--------------+-------+----------------+
         | Field        | Value | Interpretation |
         +==============+=======+================+
         | Entity ID    | 00    | Player         |
         +--------------+-------+----------------+
         | Attribute ID | 00    | Name           |
         +--------------+-------+----------------+
         | Attribute ID | 01    | Playback Info  |
         +--------------+-------+----------------+
         | Attribute ID | 02    | Volume         |
         +--------------+-------+----------------+

      #. Press **Button 1** to enable track attributes notification.
      #. In the app, verify that the **Apple Entity Update** is updated to ``02 00 01 02 03``.
         The following table lists the attributes requested by the MR.

         +--------------+-------+----------------+
         | Field        | Value | Interpretation |
         +==============+=======+================+
         | Entity ID    | 02    | Track          |
         +--------------+-------+----------------+
         | Attribute ID | 00    | Artist         |
         +--------------+-------+----------------+
         | Attribute ID | 01    | Album          |
         +--------------+-------+----------------+
         | Attribute ID | 02    | Title          |
         +--------------+-------+----------------+
         | Attribute ID | 03    | Duration       |
         +--------------+-------+----------------+

      #. Set the **Apple Remote Command** value to ``00 01 02 03 04 05 06 07 08 09 0A 0B 0C``.
         It tells the MR the list of commands supported by the MS.
      #. Verify that the UART output is as follows::

            AMS RC: 00,01,02,03,04,05,06,07,08,09,0A,0B,0C

      #. Set the **Apple Entity Update** value to ``00 01 00 30 2C 30 2E 30 2C 30 2E 30 30 30``.
      #. Verify that the UART output is as follows::

            AMS EU: 00,01,00 0,0.0,0.000

         The following table explains the notification.

         +--------------+-------------+----------------+
         | Field        | Value       | Interpretation |
         +==============+=============+================+
         | Entity ID    | 00          | Player         |
         +--------------+-------------+----------------+
         | Attribute ID | 01          | Playback Info  |
         +--------------+-------------+----------------+
         | Flags        | 00          |                |
         +--------------+-------------+----------------+
         | Value        | 0,0.0,0.000 | Paused         |
         +--------------+-------------+----------------+

      #. Set the **Apple Entity Update** value to ``02 03 00 31 32 30 2E 30 30 30``.
      #. Verify that the UART output is as follows::

            AMS EU: 02,03,00 120.000

         The following table explains the notification.

         +--------------+---------+----------------+
         | Field        | Value   | Interpretation |
         +==============+=========+================+
         | Entity ID    | 02      | Track          |
         +--------------+---------+----------------+
         | Attribute ID | 03      | Duration       |
         +--------------+---------+----------------+
         | Flags        | 00      |                |
         +--------------+---------+----------------+
         | Value        | 120.000 | 120 seconds    |
         +--------------+---------+----------------+

      #. Set the **Apple Entity Update** value to ``02 02 00 6E 52 46 35 32 20 53 65 72 69 65 73 20 73 6F 6E 67``.
      #. Verify that the UART output is as follows::

            AMS EU: 02,02,00 nRF52 Series song

         The following table explains the notification.

         +--------------+-------------------+--------------------+
         | Field        | Value             | Interpretation     |
         +==============+===================+====================+
         | Entity ID    | 02                | Track              |
         +--------------+-------------------+--------------------+
         | Attribute ID | 02                | Title              |
         +--------------+-------------------+--------------------+
         | Flags        | 00                |                    |
         +--------------+-------------------+--------------------+
         | Value        | nRF52 Series song | Current song title |
         +--------------+-------------------+--------------------+

   .. group-tab:: nRF54 DKs

      Complete the following steps to initiate a music player setup:

      #. In the `Bluetooth Low Energy app`_, verify that the **Apple Entity Update** is initiated to ``00 00 01 02``.
         The following table lists the attributes requested by the MR.

         +--------------+-------+----------------+
         | Field        | Value | Interpretation |
         +==============+=======+================+
         | Entity ID    | 00    | Player         |
         +--------------+-------+----------------+
         | Attribute ID | 00    | Name           |
         +--------------+-------+----------------+
         | Attribute ID | 01    | Playback Info  |
         +--------------+-------+----------------+
         | Attribute ID | 02    | Volume         |
         +--------------+-------+----------------+

      #. Press **Button 0** to enable track attributes notification.
      #. In the app, verify that the **Apple Entity Update** is updated to ``02 00 01 02 03``.
         The following table lists the attributes requested by the MR.

         +--------------+-------+----------------+
         | Field        | Value | Interpretation |
         +==============+=======+================+
         | Entity ID    | 02    | Track          |
         +--------------+-------+----------------+
         | Attribute ID | 00    | Artist         |
         +--------------+-------+----------------+
         | Attribute ID | 01    | Album          |
         +--------------+-------+----------------+
         | Attribute ID | 02    | Title          |
         +--------------+-------+----------------+
         | Attribute ID | 03    | Duration       |
         +--------------+-------+----------------+

      #. Set the **Apple Remote Command** value to ``00 01 02 03 04 05 06 07 08 09 0A 0B 0C``.
         It tells the MR the list of commands supported by the MS.
      #. Verify that the UART output is as follows::

            AMS RC: 00,01,02,03,04,05,06,07,08,09,0A,0B,0C

      #. Set the **Apple Entity Update** value to ``00 01 00 30 2C 30 2E 30 2C 30 2E 30 30 30``.
      #. Verify that the UART output is as follows::

            AMS EU: 00,01,00 0,0.0,0.000

         The following table explains the notification.

         +--------------+-------------+----------------+
         | Field        | Value       | Interpretation |
         +==============+=============+================+
         | Entity ID    | 00          | Player         |
         +--------------+-------------+----------------+
         | Attribute ID | 01          | Playback Info  |
         +--------------+-------------+----------------+
         | Flags        | 00          |                |
         +--------------+-------------+----------------+
         | Value        | 0,0.0,0.000 | Paused         |
         +--------------+-------------+----------------+

      #. Set the **Apple Entity Update** value to ``02 03 00 31 32 30 2E 30 30 30``.
      #. Verify that the UART output is as follows::

            AMS EU: 02,03,00 120.000

         The following table explains the notification.

         +--------------+---------+----------------+
         | Field        | Value   | Interpretation |
         +==============+=========+================+
         | Entity ID    | 02      | Track          |
         +--------------+---------+----------------+
         | Attribute ID | 03      | Duration       |
         +--------------+---------+----------------+
         | Flags        | 00      |                |
         +--------------+---------+----------------+
         | Value        | 120.000 | 120 seconds    |
         +--------------+---------+----------------+

      #. Set the **Apple Entity Update** value to ``02 02 00 6E 52 46 35 32 20 53 65 72 69 65 73 20 73 6F 6E 67``.
      #. Verify that the UART output is as follows::

            AMS EU: 02,02,00 nRF52 Series song

         The following table explains the notification.

         +--------------+-------------------+--------------------+
         | Field        | Value             | Interpretation     |
         +==============+===================+====================+
         | Entity ID    | 02                | Track              |
         +--------------+-------------------+--------------------+
         | Attribute ID | 02                | Title              |
         +--------------+-------------------+--------------------+
         | Flags        | 00                |                    |
         +--------------+-------------------+--------------------+
         | Value        | nRF52 Series song | Current song title |
         +--------------+-------------------+--------------------+

Playback
^^^^^^^^

To test an audio playback, complete the following steps:

.. tabs::

   .. group-tab:: nRF52 and nRF53 DKs

      #. Press **Button 2** to start audio playback.
      #. In the `Bluetooth Low Energy app`_, verify that the **Apple Remote Command** is updated to ``02``.
      #. Set the **Apple Entity Update** value to ``00 01 00 31 2C 31 2E 30 2C 30 2E 30 31 34``.
      #. Verify that the UART output is as follows::

            AMS EU: 00,01,00 1,1.0,0.014

         The following table explains the notification.

         +--------------+-------------+------------------------+
         | Field        | Value       | Interpretation         |
         +==============+=============+========================+
         | Entity ID    | 00          | Player                 |
         +--------------+-------------+------------------------+
         | Attribute ID | 01          | Playback Info          |
         +--------------+-------------+------------------------+
         | Flags        | 00          |                        |
         +--------------+-------------+------------------------+
         | Value        | 1,1.0,0.014 | Playing at rate 1.0.   |
         |              |             | Elapsed 0.014 seconds. |
         +--------------+-------------+------------------------+

   .. group-tab:: nRF54 DKs

      #. Press **Button 1** to start audio playback.
      #. In the Bluetooth Low Energy app, verify that the **Apple Remote Command** is updated to ``02``.
      #. Set the **Apple Entity Update** value to ``00 01 00 31 2C 31 2E 30 2C 30 2E 30 31 34``.
      #. Verify that the UART output is as follows::

            AMS EU: 00,01,00 1,1.0,0.014

         The following table explains the notification.

         +--------------+-------------+------------------------+
         | Field        | Value       | Interpretation         |
         +==============+=============+========================+
         | Entity ID    | 00          | Player                 |
         +--------------+-------------+------------------------+
         | Attribute ID | 01          | Playback Info          |
         +--------------+-------------+------------------------+
         | Flags        | 00          |                        |
         +--------------+-------------+------------------------+
         | Value        | 1,1.0,0.014 | Playing at rate 1.0.   |
         |              |             | Elapsed 0.014 seconds. |
         +--------------+-------------+------------------------+

Next track
^^^^^^^^^^

.. tabs::

   .. group-tab:: nRF52 and nRF53 DKs

      To test the next track feature, complete the following steps:

      #. Press **Button 3** to jump to next song.
      #. In the `Bluetooth Low Energy app`_, verify that the **Apple Remote Command** is updated to ``03``.
      #. Set the **Apple Entity Update** value to ``02 03 00 31 38 30 2E 30 30 30``.
      #. Verify that the UART output is as follows::

            AMS EU: 02,03,00 180.000

         The notification shows the updated song duration.

      #. Set the **Apple Entity Update** value to ``02 02 00 6E 52 46 35 33 20 53 65 72 69 65 73 20 73 6F 6E 67``.
      #. Verify that the UART output is as follows::

            AMS EU: 02,02,00 nRF53 Series song

         The notification shows the updated song title.

   .. group-tab:: nRF54 DKs

      To test the next track feature, complete the following steps:

      #. Press **Button 2** to jump to next song.
      #. In the `Bluetooth Low Energy app`_, verify that the **Apple Remote Command** is updated to ``03``.
      #. Set the **Apple Entity Update** value to ``02 03 00 31 38 30 2E 30 30 30``.
      #. Verify that the UART output is as follows::

            AMS EU: 02,03,00 180.000

         The notification shows the updated song duration.

      #. Set the **Apple Entity Update** value to ``02 02 00 6E 52 46 35 33 20 53 65 72 69 65 73 20 73 6F 6E 67``.
      #. Verify that the UART output is as follows::

            AMS EU: 02,02,00 nRF53 Series song

         The notification shows the updated song title.

Disconnect
^^^^^^^^^^

Disconnect the device in the `Bluetooth Low Energy app`_.
As the bond information is preserved by the app, you can immediately reconnect to the device by clicking :guilabel:`Connect`.

Dependencies
************

This sample uses the following |NCS| libraries:

* :ref:`ams_client_readme`
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
