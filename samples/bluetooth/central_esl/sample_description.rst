.. _central_esl_sample_desc:

Sample description
##################

.. contents::
   :local:
   :depth: 2

The Central ESL sample demonstrates how to use the :ref:`esl_service_client_readme`.
The DK acts as central Access Point(AP) defined in `Electronic Shelf Label Profile <https://www.bluetooth.com/specifications/specs/electronic-shelf-label-profile-1-0/>`_.
It uses the ESL Client to communicates with ESLs connectionlessly when one or more ESLs are in the Synchronized state.

Overview
********

In this sample, Nordic provides full set of shell commands to control the AP and auto onboarding feature.
With the auto onboarding feature, Access Point starts periodic advertising with response(PAwR) after powered up.
It will scan and connect one by one to devices with ESL service.
When connected, AP will try to configure the ESL Tag and send periodic advertising sync info(PAST) to transition ESL Tag to synchronized state.

Auto onboarding feature is meant to be used for small scale testing and development purposes. For larger scale, shell commands give more flexibility and control over the AP functionalities.

.. _central_esl_debug:

Debugging
*********

In this sample, debug messages are printed by the RTT logger.
If you want to view the debug messages, follow the procedure in :ref:`testing_rtt_connect`.

Requirements
************

The sample supports the following development kits:

.. table-from-rows:: /includes/sample_board_rows.txt
   :header: heading
   :rows: nrf5340dk_nrf5340_cpuapp, nrf52840dk_nrf52840, thingy53_nrf5340_cpuapp, nrf52840dk_nrf52840

The sample also requires another development kit running a compatible application (see :ref:`peripheral_esl`).

Building and running
********************
.. |sample path| replace:: :file:`samples/bluetooth/central_esl`

.. include:: /includes/build_and_run.txt



.. _central_esl_auto_onboarding:

Auto onboarding
===============

To associatie an ESL Tag requires numbers of steps. Which includes configuring ESL Address, configuring key materials, reading ESL capability information, updating image. This could involve several shell commands input.
The make developing process easier, this sample provides auto onboarding feature. Access Point will scan and connect to ESL Tag periodically. Then configure mandatory data and send image accodring PNPID defined in sample code.

.. _central_esl_auto_past:

Auto PAST
=========

Every ESL Tag must transistion to synchronized through PAST(Periodic Advertising Syncinfo Transfer) procedure. The PAST procedure includes the following steps.

1. AP scan and connect unsynchonrized ESL Tag.
#. AP send Update complete OP command to connected ESL Tag.
#. AP send PAST info to ESL Tag.

This could involve several shell commands input. The make developing process easier, this sample provides auto PAST feature. The AP will store information of associated ESL Tags into non-volatile database. Access Point will send PAST automatically if connected ESL Tag is in database.

.. _central_esl_testing:

Testing with other DevKits
==========================

After programming the sample to your development kit, test this sample in combination with other DevKits, which are running :ref:`peripheral_esl`, by the following steps:

1. Connect the computer to the connecter labeled **nRF USB** on DevKit or the USB connector on the Thingy:53 using a USB cable. Turn on the kit; the computer will assign to the kit two virtual COM ports (Windows) or ttyACM devices (Linux), which are visible in the Device Manager. One port is for shell commands. The other port is for EPD image transfer (SMP protocol).
#. |connect_terminal_specific|

.. note::

   To know if the right port is selected, enter new line and “ESL_AP:~$” should show up.


3. Optionally, connect the RTT console to display debug messages. See :ref:`central_esl_debug`.
#. Test the sample either by by auto onboarding feature or using shell command manually:

   .. tabs::

      .. group-tab:: Auto onboarding feature

         1. Reset the kit. (On Thingy:53 switch off and on)
            **LED 1** on the Devkit/Thingy:53 should be stable on.
         #. Press button 1 to let Auto onboarding AP feature start.
            **LED 1** on th eDevKit/Thingy:53 should start blinking, signaling the AP is scanning for ESL tags, connecting one by one to them, configuring them and transferring synchronization (PAST).
         #. Press button 3 of DK to let ESL Tag to change image on display.
         #. Press button 4 of DK to let ESL Tag to flash LED in predefined pattern.
         #. Press button 2 of DK to erase the database of associated ESL Tags which is used by :ref:`central_esl_auto_past` from the AP.

         .. note::

            Button 3 and 4 will continually send ESL command through PAwR and cycles between group 0 to :kconfig:option:`CONFIG_BT_ESL_AP_GROUP_PER_BUTTON`


      .. group-tab:: Shell command

         * Start scan continuously with command:

         .. code-block:: console

            esl_c scan 1 0

         The sample shows the following output:

         .. code-block:: console

            #SCAN:start
            #TAG_SCANNED: 1,XX:XX:XX:XX:XX:XX (random) to list

         * Connect ESL Tag to connection index 0 with command:

         .. code-block:: console

            esl_c acl connect 0

         The sample shows the following output to indicate that connection and service discovery is succeed:

         .. code-block:: console

            Connected:Conn_idx:00
            #DISCOVERY: 1,0x00

         * Optionally, AP can connect Tag by knowing BLE address out-of-band without scanning:

         .. code-block:: console

            esl_c acl connect_addr 0 AA:BB:CC:DD:EE:FF

         * Configure ESL Tag to esl address ``0x0101`` with command:

         .. code-block:: console

            esl_c acl configure 0 0101

         The sample shows the following output to indicate that ESL Tag has been configured and update complete OP code sent:

         .. code-block:: console

            Configure conn_idx 0 esl_addr 0x0101
            CONFIGURED: 1,0x0000,0x0101
            #BASIC_STATE:1,0x0101
            SERVICE_NEEDED:off
            SYNCHRONIZED:off
            ACTIVE_LED:off
            PENDING_LED_UPDATE:off
            PENDING_DISPLAY_UPDATE:off
            BASIC_RFU:off

         * Read information of connected Tag for further use with command:

         .. code-block:: console

            esl_c esl_c dump

         The sample shows the following output:

         .. code-block:: console

            AP Sync Key
            00000000: 6d 3a 14 4f f1 19 45 f7  4d 5e a8 50 75 45 b9 d2 |m:.O..E. M^.PuE..|
            00000010: ff da 94 bd 8e 9b 33 61                          |......3a         |
            Randomizer
            00000000: 60 83 38 82 f6                                   |`.8..            |
            #ABSTIME:570155
            Connected Tag counts 0
            Tag 0
            ESL handles: 0x002e 0x0030 0x0032 0x0034 0x0036 0x0038 0x003a 0x003c 0x003e 0x003f 0x0016
            OTS client handles:
            start_handle 0x0001
            end_handle 0xffff
            obj_feat 0x001b
            obj_name 0x001d
            obj_type 0x001f
            obj_size 0x0021
            obj_prop 0x0025
            obj_created 0x0000
            obj_modified 0x0000
            obj_id 0x0023
            obj_ocap 0x0027
            obj_olcp 0x002a
            BLE ADDR:C8:6D:40:1E:C2:AE (ra#ndom)
            CHR_READ:1
            ESL_ADDR:0x0101
            RSP_KEY:0x0b73c85e8a37d6094fe536812f6720a06d60d1cf4c9eadd1
            Max_image_index:1
            DISPLAY:1,0x6400640001
            SENSOR:2,0x00540000000115198794
            Sensor 0 Mesh property ID Sensor type: 0x0054
            Sensor 1 vendor_specific_sensor Company ID: 0x1915 Sensor Code: 0x9487
            LED:4,0x4c4c4c4c
            led idx 0, type Mono color 0x0c:Green
            led idx 1, type Mono color 0x0c:Green
            led idx 2, type Mono color 0x0c:Green
            led idx 3, type Mono color 0x0c:Green
            PNPID:VID:0x1915, PID:0x5484

         * Send image(optional) on AP index `0` to ESL Tag image index `0` connection `0` with command:

         .. code-block:: console

            esl_c obj_c write 0 0 0

         * This command is equivalent to previous one. You can use filename without original pattern `ots_image_XX`

         .. code-block:: console

            esl_c obj_c write_filename 0 0 ots_image_00

         The sample shows the following output indicate that image has been transferred:

         .. code-block:: console

            #OTS_WRITE:1,0,0,esl_image_00
            #OTS_WRITTEN:1,00,0,1246,0x7bd5f88

         * After configured all data and sent all image, use periodic advertising sync transfer procedure to let ESL Tag entering synced with command:

         .. code-block:: console

            esl_c acl past 0

         The sample shows the following output indicate that ESL received PAST and disconnect:

         .. code-block:: console

            PAST:1,00
            Disconnected:Conn_idx:0x00 (reason 0x13)

         * Ping ESL Tag of ESL address `0x0201` with PAwR by raw ESL payload:

         .. code-block:: console

            esl_c pawr push_sync_buf 2 0001

         * Control LED 0 to flash to all ESL Tags of group `0` with PAwR with predefined command:

         .. code-block:: console

            esl_c pawr update_pawr 0 1

         * Turn LED 0 off to all ESL Tags of group `0` with PAwR with predefined command:

         .. code-block:: console

            esl_c pawr update_pawr 0 4

         * Display image 0 to all ESL Tags of group `2` with PAwR with predefined command:

         .. code-block:: console

            esl_c pawr update_pawr 2 7

         * See :ref:`central_esl_subcommands` for a list of available subcommands.


Setup - Transfer Images from PC to AP over USB
==============================================

One of major task of Access Point is to provide images to ESL Tag in configuring and updating state.
Before activating auto onboarding (**Button 1**) or, anytime before sending images using :ref:`Object Transfer Service <bluetooth_services>` via shell commands between AP and Tag, it is necessary to transfer the images from PC to the AP (in external flash) over USB.
:ref:`mcumgr_smp_protocol_specification` is used over the other COM port available over USB. Here example:

1. Install :ref:`mcu_mgr` tool
#. Prepare the images to be transferred. The image format is depending on what EPD and color depth your are using.

.. note::

   Nordic provides two sample files that can be used with the :ref:`peripheral_esl` EPD variant.

   * EPD format: Use this format on Access Point :download:`esl_image_bt <https://github.com/pirun/esl_sample_image/raw/main/esl_image_bt>` :download:`esl_image_nordic <https://github.com/pirun/esl_sample_image/raw/main/esl_image_nordic>`
   * BMP format: This format is only for reference :download:`esl_image_00_240x120_bt.BMP <https://github.com/pirun/esl_sample_image/blob/main/esl_image_00_240x120_bt.BMP>` :download:`esl_image_01_240x120_nordic_logo.BMP <https://github.com/pirun/esl_sample_image/blob/main/esl_image_01_240x120_nordic_logo.BMP>`


3. Transfer images from the PC to the AP: run from cmd /command line

   .. code-block:: console

      mcumgr --conntype="serial" --connstring="COM4,baud=115200,mtu=512" fs upload esl_image_bt /ots_image/esl_image_00
      mcumgr --conntype="serial" --connstring="COM4,baud=115200,mtu=512" fs upload esl_image_nordic /ots_image/esl_image_01

   .. note::

      Remember to change **COM4** to the right COM port in your PC.
      Images are stored in external non-volatile flash.

      Non-volatile external flash is kept even after reset of DK (power cycle), or after programming or after AP database erase (**Button 2**)

      Once the images are in the AP, it is possible to transfer them to the tags at any time using shell commands, assuming the AP and Tag are in an ACL connection. Refer to :ref:`central_esl_subcommands` for this.


#. As an alternative, for demo purposes, Auto Onboarding feature can be used:
If auto onboarding is started after the transfer of image (i.e. a new Tag is associated for the first time after transferring the image to the AP and after pushing **Button 1**), the AP will transfer automatically these images to the ESL Tag DevKit with EPD shield, during the associating procedure.


Dependencies
************

This sample uses the following |NCS| libraries:

* :ref:`esl_service_client_readme`
* :ref:`gatt_dm_readme`

In addition, it uses the following Zephyr libraries:

* ``include/zephyr/types.h``
* ``boards/arm/nrf*/board.h``
* :ref:`zephyr:kernel_api`:

  * ``include/kernel.h``

* :ref:`zephyr:api_peripherals`:

   * ``include/esl_client.h``

* :ref:`zephyr:bluetooth_api`:

  * ``include/bluetooth/bluetooth.h``
  * ``include/bluetooth/gatt.h``
  * ``include/bluetooth/hci.h``
  * ``include/bluetooth/uuid.h``
