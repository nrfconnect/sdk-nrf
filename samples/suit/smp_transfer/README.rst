.. _nrf54h_suit_sample:

SUIT: Device firmware update on the nRF54H20 SoC
################################################

.. contents::
   :local:
   :depth: 2

The sample demonstrates how to update and boot the nRF54H20 System-on-Chip (SoC) using the :ref:`Software Update for Internet of Things (SUIT) <ug_nrf54h20_suit_intro>` procedure on the application and radio cores of the SoC.
The update on the nRF54H20 SoC can be done over Bluetooth® Low Energy or UART.

Requirements
************

The sample supports the following development kit:

.. table-from-rows:: /includes/sample_board_rows.txt
   :header: heading
   :rows: nrf54h20dk_nrf54h20_cpuapp

You need the nRF Device Manager app for SUIT update over Bluetooth Low Energy:

* `nRF Device Manager mobile app for Android`_
  (The minimum required version is v2.0.)

* `nRF Device Manager mobile app for iOS`_
  (The minimum required version is v1.7.)

For a SUIT update over UART, you need to install :ref:`zephyr:mcu_mgr`, a tool that can be used to upload SUIT envelopes through the SMP protocol.

Overview
********

The sample uses one of the following methods to perform a firmware update on the nRF54H20 SoC, featuring Nordic Semiconductor’s implementation of the SUIT procedure:

* Bluetooth Low Energy and the nRF Device Manager app
* UART and MCUmgr

The sample demonstrates how to perform a DFU of the application and radio firmware.

SUIT is the only DFU procedure supported for the nRF54H20 SoCs.
To read more about the SUIT procedure, see :ref:`ug_nrf54h20_suit_intro`.

User interface
**************

LED 0:
    The number of blinks indicates the application SUIT envelope sequence number.
    The default is set to ``1`` to blink once, indicating "Version 1".

Configuration
*************

|config|

The default configuration uses UART with sequence number 1 (shown as Version 1 in the nRF Device Manager app).
To change the sequence number of the application, configure the ``SB_CONFIG_SUIT_ENVELOPE_SEQUENCE_NUM`` sysbuild Kconfig option.
This also changes the number of blinks on **LED 0** and sets the :ref:`sequence number <ug_suit_dfu_suit_manifest_elements>` of the :ref:`SUIT envelope <ug_suit_dfu_suit_concepts>`’s manifest.

To use this configuration, build the sample with :ref:`configuration_system_overview_sysbuild` and set the ``SB_CONFIG_SUIT_ENVELOPE_SEQUENCE_NUM`` sysbuild Kconfig option to ``x``, where ``x`` is the version number.
For example:

.. code-block:: console

   west build -p -b nrf54h20dk/nrf54h20/cpuapp -- -DSB_CONFIG_SUIT_ENVELOPE_SEQUENCE_NUM=2

If you do not specify this configuration, the sample is built with sequence number 1 (shown as Version 1 in the nRF Device Manager app).

Configuration options
=====================

Check and configure the following configuration option for the sample:

.. _SB_CONFIG_SUIT_ENVELOPE_SEQUENCE_NUM:

SB_CONFIG_SUIT_ENVELOPE_SEQUENCE_NUM - Configuration for the sequence number.
   The sample configuration updates the sequence number of the SUIT envelope, which is reflected as the version of the application in the nRF Device Manager app.
   The default value is ``1``.

Modify partition sizes
======================

You can also modify the size and location of the partitions.
This is done by modifying the values for the desired location and size of the partition in the devicetree :file:`.overlay` files.

* To modify the application core's partition size, modify the values for ``cpuapp_slot0_partition`` defined in the :file:`nrf/samples/suit/smp_transfer/sysbuild/nrf54h20dk_nrf54h20_memory_map.dtsi`.

* To modify the DFU partition, modify the values for ``dfu_partition`` defined in :file:`samples/suit/smp_transfer/boards/nrf54h20dk_nrf54h20_cpuapp.overlay`.
  This partition is where the update candidate is stored before the update process begins.

Manifest template files
=======================

The SUIT DFU procedure requires an envelope to transport the firmware update, and SUIT envelopes require a SUIT manifest template as a source file.
All required manifest template files (used to later create SUIT envelopes) are automatically created during the first sample build, and are the following:

* The root manifest - :file:`root_hierarchical_envelope.yaml.jinja2`

* The application domain manifest - :file:`app_envelope.yaml.jinja2`

* The radio domain manifest - :file:`rad_envelope.yaml.jinja2`

See :ref:`app_build_output_files_suit_dfu` for a full table of SUIT-generated output files.

.. note::

   The radio domain manifest template (:file:`radio.suit`) is only created when building the Bluetooth Low Energy version of the sample, and not the UART version.
   Currently, it is not needed for the UART version.

If you want to make modifications to how the DFU is executed in this sample, you can do so by editing the manifest templates, or generating your own custom manifests.
See the :ref:`ug_nrf54h20_suit_customize_dfu` user guide for instructions and examples.

External flash support
======================

You can enable the external flash support by setting the following ``FILE_SUFFIX=extflash`` parameter:

.. code-block:: console

   west build -p -b nrf54h20dk/nrf54h20/cpuapp -- -DFILE_SUFFIX="extflash"

With this configuration, the sample is configured to use UART as the transport and the external flash is enabled.

To enable both the external flash and the BLE transport, use the following command:

.. code-block:: console

   west build -p -b nrf54h20dk/nrf54h20/cpuapp -- -DFILE_SUFFIX="extflash" -DOVERLAY_CONFIG="sysbuild/smp_transfer_bt.conf" -DSB_OVERLAY_CONFIG="sysbuild_bt.conf"

Building and running
********************

.. |sample path| replace:: :file:`samples/suit/smp_transfer`

This sample can be found under |sample path| in the |NCS| folder structure.

.. note::
    |sysbuild_autoenabled_ncs|

Building and programming using the command line
===============================================

To build and program the sample to the nRF54H20 DK, complete the following steps:

.. tabs::

   .. group-tab:: Over Bluetooth Low Energy

      1. Open a terminal window in |sample path|.
      #. Build the sample using the following command, with the following Kconfig options set:

         .. code-block:: console

            west build -p -b nrf54h20dk/nrf54h20/cpuapp -- -DFILE_SUFFIX=bt -DSB_CONFIG_SUIT_ENVELOPE_SEQUENCE_NUM=1

         .. note::

            |application_sample_long_path_windows|

            In this case, you may need to run the following instead:

            .. code-block:: console

               west build -p -b nrf54h20dk/nrf54h20/cpuapp -d C:/ncs-lcs/work-dir -- -DFILE_SUFFIX=bt -DSB_CONFIG_SUIT_ENVELOPE_SEQUENCE_NUM=1

         The output build files can be found in the :file:`build/DFU` directory, including the :ref:`app_build_output_files_suit_dfu`.
         For more information on the contents of the build directory, see :ref:`zephyr:build-directory-contents` in the Zephyr documentation.
         For more information on the directory contents and structure provided by sysbuild, see :ref:`zephyr:sysbuild` in the Zephyr documentation.

      #. Connect the DK to your computer using a USB cable.
      #. Power on the DK.
      #. Program the sample to the kit (see :ref:`programming_cmd` for instructions).

         .. note::

            |application_sample_long_path_windows|

            In this case, you may need to run the following instead:

            .. code-block:: console

               west flash --erase -d C:/ncs-lcs/work-dir

      #. Update the SUIT envelope sequence number, by rebuilding the sample with an updated sequence number:

         .. code-block:: console

            west build -p -b nrf54h20dk/nrf54h20/cpuapp -- -DFILE_SUFFIX=bt -DSB_CONFIG_SUIT_ENVELOPE_SEQUENCE_NUM=2

         .. note::

            |application_sample_long_path_windows|

            In this case, you may need to run the following instead:

            .. code-block:: console

               west build -p -b nrf54h20dk/nrf54h20/cpuapp -d C:/ncs-lcs/work-dir -- -DFILE_SUFFIX=bt -DSB_CONFIG_SUIT_ENVELOPE_SEQUENCE_NUM=2

         Another :file:`root.suit` file is created after running this command, that contains the updated firmware.
         You must manually transfer this file onto the same mobile device you will use with the nRF Device Manager app.

   .. group-tab:: Over UART

      1. Open a terminal window in |sample path|.
      #. Build the sample:

         .. code-block:: console

             west build -p -b nrf54h20dk/nrf54h20/cpuapp

         .. note::

            |application_sample_long_path_windows|

            In this case, you may need to run the following instead:

            .. code-block:: console

               west build -p -b nrf54h20dk/nrf54h20/cpuapp -d C:\ncs-lcs\west_working_dir\build\

         If you want to further configure your sample, see :ref:`configure_application` for additional information.

         After running the ``west build`` command, the output build files can be found in the :file:`build/dfu` directory.
         The output build files can be found in the :file:`build/DFU` directory, including the :ref:`app_build_output_files_suit_dfu`.
         For more information on the contents of the build directory, see :ref:`zephyr:build-directory-contents` in the Zephyr documentation.
         For more information on the directory contents and structure provided by sysbuild, see :ref:`zephyr:sysbuild` in the Zephyr documentation..

      #. Connect the DK to your computer using a USB cable.
      #. Power on the DK.
      #. Program the sample to the kit (see :ref:`programming_cmd` for instructions).

         .. note::

            |application_sample_long_path_windows|

            In this case, you may need to run the following instead:

            .. code-block:: console

               west flash --erase -d C:/ncs-lcs/work-dir

      #. Update the SUIT envelope sequence number, by rebuilding the sample with an updated sequence number:

         .. code-block:: console

            west build -p -b nrf54h20dk/nrf54h20/cpuapp -- -DSB_CONFIG_SUIT_ENVELOPE_SEQUENCE_NUM=2

         .. note::

            |application_sample_long_path_windows|

            In this case, you may need to run the following instead:

            .. code-block:: console

               west build -p -b nrf54h20dk/nrf54h20/cpuapp -d C:/ncs-lcs/work-dir -- -DSB_CONFIG_SUIT_ENVELOPE_SEQUENCE_NUM=2

         Another :file:`root.suit` file is created after running this command, that contains the updated firmware.

Testing
=======

After programming the sample to your development kit and updating the sequence number of the SUIT envelope, complete the following steps to test it.

.. tabs::

   .. group-tab:: Over Bluetooth Low Energy

      1. Upload the signed envelope onto your mobile phone:

         a. Open the nRF Device Manager app on your mobile phone.
         #. Select the device **SUIT SMP Sample**.
            You should see the following:

            .. figure:: /images/suit_smp_select_suit_smp_sample.png
               :alt: Select SUIT SMP Sample

         #. From the **SUIT SMP Sample** screen, on the **Images** tab at the bottom of the screen, click on :guilabel:`ADVANCED` in the upper right corner of the app to open a new section called **Images**.

            .. figure:: /images/suit_smp_select_advanced.png
               :alt: Select ADVANCED

         #. Click on the :guilabel:`READ` button within the **Images** section.

            .. figure:: /images/suit_smp_select_image_read.png
               :alt: Select READ from Images

            You should now see that "Version: 1" is printed in the **Images** section of the mobile app.

         #. From the **Firmware Upload** section, click on :guilabel:`SELECT FILE` and select the :file:`root.suit` file from your mobile device.

            .. note::
               As described in Step 1, you must manually add the :file:`root.suit` file to the same mobile device you are using for nRF Device Manager.

            .. figure:: /images/suit_smp_select_firmware_select_file.png
               :alt: Select Firmware Upload and Select File

         #. Click on :guilabel:`UPLOAD` to upload the :file:`root.suit` file.

            You should see an upload progress bar below the "UPLOADING…" text in the **Firmware Upload** section.

            .. figure:: /images/suit_smp_firmware_uploading.png
               :alt: Firmware UPLOADING


            The text "UPLOAD COMPLETE" appears in the **Firmware Upload** section once completed.

            .. figure:: /images/suit_smp_firmware_upload_complete.png
               :alt: Firmware UPLOAD COMPLETE

      #. Reconnect your device.
      #. Select the device **SUIT SMP Sample** once again.

         .. figure:: /images/suit_smp_images_v2.png
            :alt: Images Version 2

      #. Under the **Images** section, click on :guilabel:`READ`.

         You should see that "Version: 2" is now printed in the **Images** section of the mobile app.

         Additional, **LED 0** now flashes twice now to indicate "Version 2" of the firmware.

   .. group-tab:: Over UART

      1. Upload the signed envelope:

         a. Read the version and digest of the installed root manifest with MCUmgr:

            .. code-block:: console

               mcumgr --conntype serial --connstring "dev=/dev/ttyACM0,baud=115200" image list

            You should see an output similar to the following logged on UART:

            .. parsed-literal::
               :class: highlight

               image=0 slot=0
                  version: 1
                  bootable: true
                  flags: active confirmed permanent
                  hash: d496cdc8fa4969d271204e8c42c86c7499ae8632f131e098e2e0fb5c7bbe3a5f
               Split status: N/A (0)

         #. Upload the image with MCUmgr:

            .. code-block:: console

               mcumgr --conntype serial --connstring "dev=/dev/ttyACM0,baud=115200,mtu=512" image upload root.suit

            You should see an output similar to the following logged on UART:

            .. parsed-literal::
               :class: highlight

               0 / 250443 [---------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------]   0.00%
               18.99 KiB / 244.57 KiB [============>-------------------------------------------------------------------------------------------------------------------------------------------------]   7.76% 11.83 KiB/s 00m19s
               66.56 KiB / 244.57 KiB [==========================================>-------------------------------------------------------------------------------------------------------------------]  27.21% 18.44 KiB/s 00m09s
               112.12 KiB / 244.57 KiB [=======================================================================>-------------------------------------------------------------------------------------]  45.84% 19.97 KiB/s 00m06s
               154.08 KiB / 244.57 KiB [==================================================================================================>----------------------------------------------------------]  63.00% 20.22 KiB/s 00m04s
               197.40 KiB / 244.57 KiB [==============================================================================================================================>------------------------------]  80.71% 20.51 KiB/s 00m02s
               241.16 KiB / 244.57 KiB [=================================================================================================================================================================>--]  98.60% 20.74 KiB/s
               Done

      #. Read the version and digest of the uploaded root manifest with MCUmgr:

         .. code-block:: console

             mcumgr --conntype serial --connstring "dev=/dev/ttyACM0,baud=115200,mtu=512" image list


         You should see an output similar to the following logged on UART:

         .. parsed-literal::
            :class: highlight

            image=0 slot=0
               version: 2
               bootable: true
               flags: active confirmed permanent
               hash: 707efbd3e3dfcbda1c0ce72f069a55f35c30836b79ab8132556ed92ce609f943
            Split status: N/A (0)

         You should now see that **LED 0** flashes twice now to indicate "Version 2" of the firmware.
