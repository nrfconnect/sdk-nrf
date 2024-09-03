.. _nrf54h_suit_sample:

SUIT: Device firmware update on the nRF54H20 SoC
################################################

.. contents::
   :local:
   :depth: 2

The sample demonstrates how to update and boot the nRF54H20 System-on-Chip (SoC) using the :ref:`Software Update for Internet of Things (SUIT) <ug_nrf54h20_suit_intro>` procedure on the application, radio, and Nordic-controlled cores of the SoC.
The update on the nRF54H20 SoC can be done over Bluetooth® Low Energy or UART.

Requirements
************

The sample supports the following development kit:

.. table-from-rows:: /includes/sample_board_rows.txt
   :header: heading
   :rows: nrf54h20dk_nrf54h20_cpuapp

You need the nRF Device Manager app for SUIT update over Bluetooth Low Energy:

* `nRF Device Manager mobile app for Android`_
  (The minimum required version is v2.2.1.)

* `nRF Device Manager mobile app for iOS`_
  (The minimum required version is v1.8.0.)

To perform a SUIT update over UART using the SMP protocol, install SUIT commands in nrfutil by running the ``nrfutil install suit`` command (requires nrfutil version 0.8.1 or higher).

Overview
********

The sample uses one of the following methods to perform a firmware update on the nRF54H20 SoC, featuring Nordic Semiconductor’s implementation of the SUIT procedure:

* Bluetooth Low Energy and the nRF Device Manager app
* UART and ``nrfutil suit`` commands

The sample demonstrates how to perform a DFU of the application and radio firmware.

SUIT is the only DFU procedure supported for the nRF54H20 SoCs.
To read more about the SUIT procedure, see :ref:`ug_nrf54h20_suit_intro`.

User interface
**************

LED 0:
    The ``CONFIG_N_BLINKS`` Kconfig option sets the number of blinks.
    The default is set to ``1``, causing a single blink to indicate *Version 1*.
    This value can be incremented to represent an update, such as setting it to ``2`` to indicate *Version 2*.

Configuration
*************

|config|

The default configuration uses UART with sequence number 1 (shown as Version 1 in the nRF Device Manager app).

To change the sequence number of the application, configure the ``APP_ROOT_SEQ_NUM`` inside the :file:`VERSION` file, used for :ref:`zephyr:app-version-details` in Zephyr and the |NCS|.
It sets the :ref:`sequence number <ug_suit_dfu_suit_manifest_elements>` of the :ref:`SUIT envelope <ug_suit_dfu_suit_concepts>`’s manifest.
If you do not provide the :file:`VERSION` file, the sample is built with sequence number set to 1 (shown as Version 1 in the nRF Device Manager app).

To change the number of blinks on **LED 0**, configure the ``CONFIG_N_BLINKS`` Kconfig option.
If you do not specify this configuration, the sample is built with the number of blinks set to 1.

Configuration options
=====================

Check and configure the following configuration option for the sample:

.. _CONFIG_N_BLINKS:

CONFIG_N_BLINKS - Configuration for the number of blinks.
   The sample configuration change the number of blinks on **LED 0**.
   The default value is ``1``.

.. _CONFIG_BT_DEVICE_NAME:

CONFIG_BT_DEVICE_NAME - Configuration for Bluetooth Device Name.
   The sample configuration changes the advertised Bluetooth name.
   Its default value is ``SUIT SMP Sample``.

Modify partition sizes
======================

You can also modify the size and location of the partitions.
This is done by modifying the values for the desired location and size of the partition in the devicetree :file:`.overlay` files.

* To modify the application core's partition size, modify the values for ``cpuapp_slot0_partition`` defined in the :file:`nrf/samples/suit/smp_transfer/sysbuild/nrf54h20dk_nrf54h20_memory_map.dtsi`.

* To modify the DFU partition, add the following snippet inside the :file:`samples/suit/smp_transfer/boards/nrf54h20dk_nrf54h20_cpuapp.overlay` file, modifying the addresses and sizes accordingly::

      &mram1x {
         cpuapp_rw_partitions: cpuapp-rw-partitions {
            /* DFU partition to store SUIT manifests and Nordic Firmware update */
            dfu_partition: partition@100000 {
                  reg = < 0x100000 DT_SIZE_K(300) >;
            };

         };
      };

  This partition is where the update candidate is stored before the update process begins.
  You can see an example of this snippet inside the :file:`nrf/applications/matter_bridge/boards/nrf54h20dk_nrf54h20_cpuapp.overlay` file.

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

.. _nrf54h_suit_sample_extflash:

External flash support
======================

You can build the application with external flash support by running the following command from the sample directory:

.. code-block:: console

   west build -b nrf54h20dk/nrf54h20/cpuapp -T ./sample.suit.smp_transfer.cache_push.extflash

With this configuration, the sample is configured to use UART as the transport and the external flash is enabled.
To see which Kconfig options are needed to achieve that, see the ``sample.suit.smp_transfer.cache_push.extflash`` configuration in the :file:`samples/suit/sample.yaml` file.

To enable both the external flash and the BLE transport, use the following command:

.. code-block:: console

   west build -b nrf54h20dk/nrf54h20/cpuapp -T ./sample.suit.smp_transfer.cache_push.extflash.bt

.. note::
   This way of building the application will enable the push scenario for updating from external flash.
   It will also extract the image to a DFU cache partition file.
   For more information, see :ref:`How to push SUIT payloads to multiple partitions <ug_nrf54h20_suit_push>`.

Building and running
********************

.. |sample path| replace:: :file:`samples/suit/smp_transfer`

This sample can be found under |sample path| in the |NCS| folder structure.

.. note::
   |54H_engb_2_8|

.. note::
    |sysbuild_autoenabled_ncs|

Building and programming using the command line
===============================================

To build and program the sample to the nRF54H20 DK, complete the following steps:

.. tabs::

   .. group-tab:: Over Bluetooth Low Energy

      1. |open_terminal_window_with_environment|
      #. Navigate to |sample path|.
      #. Build the sample using the following command, with the following Kconfig options set:

         .. code-block:: console

            west build -p -b nrf54h20dk/nrf54h20/cpuapp -- -DFILE_SUFFIX=bt -DCONFIG_N_BLINKS=1

         See :ref:`configure_application` for information on additional configuration options.

         The output build files can be found in the :file:`build/DFU` directory, including the :ref:`app_build_output_files_suit_dfu`.
         For more information on the contents of the build directory, see :ref:`zephyr:build-directory-contents` in the Zephyr documentation.
         For more information on the directory contents and structure provided by sysbuild, see :ref:`zephyr:sysbuild` in the Zephyr documentation.

      #. Connect the DK to your computer using a USB cable.
      #. Power on the DK.
      #. Program the sample to the kit (see :ref:`programming_cmd` for instructions).
      #. Update the SUIT envelope sequence number, by changing the following line to the :file:`VERSION` file:

         .. code-block:: console

            APP_ROOT_SEQ_NUM = 2

      #. Update the number of LED blinks, by rebuilding the sample with the following Kconfig options set:

         .. code-block:: console

            west build -b nrf54h20dk/nrf54h20/cpuapp -- -DFILE_SUFFIX=bt -DCONFIG_N_BLINKS=2

         Another :file:`root.suit` file is created after running this command, that contains the updated firmware.
         You must manually transfer this file onto the same mobile device you will use with the nRF Device Manager app.

   .. group-tab:: Over UART

      1. |open_terminal_window_with_environment|
      #. Navigate to |sample path|.
      #. Build the sample using the following command, ensuring the specified Kconfig options are set:

         .. code-block:: console

             west build -p -b nrf54h20dk/nrf54h20/cpuapp -- -DCONFIG_N_BLINKS=1

         If you want to further configure your sample, see :ref:`configure_application` for additional information.

         The output build files can be found in the :file:`build/DFU` directory, including the :ref:`app_build_output_files_suit_dfu`.
         For more information on the contents of the build directory, see :ref:`zephyr:build-directory-contents` in the Zephyr documentation.
         For more information on the directory contents and structure provided by sysbuild, see :ref:`zephyr:sysbuild` in the Zephyr documentation.

      #. Connect the DK to your computer using a USB cable.
      #. Power on the DK.
      #. Program the sample to the kit (see :ref:`programming_cmd` for instructions).
      #. Update the SUIT envelope sequence number, by changing the following line to the :file:`VERSION` file:

         .. code-block:: console

            APP_ROOT_SEQ_NUM = 2

      #. Update the number of LED blinks, by rebuilding the sample with the following Kconfig options set:

         .. code-block:: console

            west build -b nrf54h20dk/nrf54h20/cpuapp -- -DCONFIG_N_BLINKS=2


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

         #. From the **SUIT SMP Sample** screen, on the **Images** tab at the bottom of the screen, Tap on :guilabel:`ADVANCED` in the upper right corner of the app to open a new section called **Images**.

            .. figure:: /images/suit_smp_select_advanced.png
               :alt: Select ADVANCED

         #. Tap on the :guilabel:`Read` button within the **Images** section.

            .. figure:: /images/suit_smp_select_image_read.png
               :alt: Select Read from Images

            After the list of SUIT manifests appears in the section, scroll down to verify that ``Sequence number: 0x1`` is displayed for ``APP_ROOT``.

            .. figure:: /images/suit_smp_show_manifest_01.png
               :alt: Show manifests before update

         #. From the **Firmware Upload** section, tap on :guilabel:`SELECT FILE` and select the :file:`root.suit` file from your mobile device.

            .. note::
               As described in Step 1, you must manually add the :file:`root.suit` file to the same mobile device you are using for nRF Device Manager.

            .. figure:: /images/suit_smp_select_firmware_select_file.png
               :alt: Select Firmware Upload and Select File

         #. Tap on :guilabel:`Upload` to upload the :file:`root.suit` file.

            You should see an upload progress bar below the "UPLOADING…" text in the **Firmware Upload** section.

            .. figure:: /images/suit_smp_firmware_uploading.png
               :alt: Firmware UPLOADING


            The text "UPLOAD COMPLETE" appears in the **Firmware Upload** section once completed.

            .. figure:: /images/suit_smp_firmware_upload_complete.png
               :alt: Firmware UPLOAD COMPLETE

      #. Tap on the :guilabel:`Confirm` button within the **Images** section

         .. figure:: /images/suit_smp_firmware_upload_confirm.png
            :alt: Firmware UPLOAD CONFIRM

      #. The update is now applied, and **LED 0** flashes twice to indicate *Version 2* of the firmware.
      #. Under the **Images** section, tap on :guilabel:`Read`.

         In the section displaying the SUIT manifests list, scroll to the bottom to verify that "Sequence number: 0x2" is shown for ``APP_ROOT``.

         .. figure:: /images/suit_smp_show_manifest_02.png
            :alt: Show manifests after update


   .. group-tab:: Over UART

      Perform the following on the device connected to the serial port COM7 on Windows.
      To verify the serial port name for your operating system, use the ``nrfutil device list`` command.

      1. Upload the signed envelope:

         a. Read the sequence number of the installed root manifest with nrfutil:

            .. code-block:: console

               nrfutil suit manifests --serial-port COM7

            You should see an output similar to the following printed in the terminal:

            .. code-block:: console

               role(10) (Nordic Top)
                     classId: f03d385e-a731-5605-b15d-037f6da6097f (nRF54H20_nordic_top)
                     vendorId: 7617daa5-71fd-5a85-8f94-e28d735ce9f4 (nordicsemi.com)
                     downgradePreventionPolicy: downgrade forbidden
                     independentUpdateabilityPolicy: independent update allowed
                     signatureVerificationPolicy: signature verification on update and boot
                     digest: fd25c5e9d2a1bc8360a497c157d53f4634e8abe48d759065deb9fd84e9fbeece
                     digestAlgorithm: sha256
                     signatureCheck: signature check passed
                     sequenceNumber: 458752
                     semantic version: 0.7.0
               role(11) (SDFW and SDFW recovery updates)
                     classId: d96b40b7-092b-5cd1-a59f-9af80c337eba (nRF54H20_sec)
                     vendorId: 7617daa5-71fd-5a85-8f94-e28d735ce9f4 (nordicsemi.com)
                     downgradePreventionPolicy: downgrade forbidden
                     independentUpdateabilityPolicy: independent update forbidden
                     signatureVerificationPolicy: signature verification on update and boot
                     digest: deb46e703a49ec69524056a37d45d9534ff146194be6e4add53406e3014bdbd2
                     digestAlgorithm: sha256
                     signatureCheck: signature check passed
                     sequenceNumber: 134218240
                     semantic version: 8.0.2
               role(12) (System Controller Manifest)
                     classId: c08a25d7-35e6-592c-b7ad-43acc8d1d1c8 (nRF54H20_sys)
                     vendorId: 7617daa5-71fd-5a85-8f94-e28d735ce9f4 (nordicsemi.com)
                     downgradePreventionPolicy: downgrade forbidden
                     independentUpdateabilityPolicy: independent update forbidden
                     signatureVerificationPolicy: signature verification on update and boot
                     digest: 5dac29159363cbc50552a51ec8014d3927cbc854f83d8575af74ee27ee426c77
                     digestAlgorithm: sha256
                     signatureCheck: signature check passed
                     sequenceNumber: 50331648
                     semantic version: 3.0.0
               role(31) (Radio Local Manifest)
                     classId: 816aa0a0-af11-5ef2-858a-feb668b2e9c9 (nRF54H20_sample_rad)
                     vendorId: 7617daa5-71fd-5a85-8f94-e28d735ce9f4 (nordicsemi.com)
                     downgradePreventionPolicy: downgrade allowed
                     independentUpdateabilityPolicy: independent update forbidden
                     signatureVerificationPolicy: signature verification disabled
                     digest: 5fb501225293ab78ec0caf14457543514550044757de2fa8a5e764ec059eae0d
                     digestAlgorithm: sha256
                     signatureCheck: signature check not performed
                     sequenceNumber: 1
                     semantic version: 0.1.0
               role(20) (Root Manifest)
                     classId: 3f6a3a4d-cdfa-58c5-acce-f9f584c41124 (nRF54H20_sample_root)
                     vendorId: 7617daa5-71fd-5a85-8f94-e28d735ce9f4 (nordicsemi.com)
                     downgradePreventionPolicy: downgrade allowed
                     independentUpdateabilityPolicy: independent update allowed
                     signatureVerificationPolicy: signature verification disabled
                     digest: a0d99d177511ce908aaa7b74a8c595fc06060673976d09466b0d2c26f6547e2d
                     digestAlgorithm: sha256
                     signatureCheck: signature check not performed
                     sequenceNumber: 1
                     semantic version: 0.1.0
               role(22) (Application Local Manifest)
                     classId: 08c1b599-55e8-5fbc-9e76-7bc29ce1b04d (nRF54H20_sample_app)
                     vendorId: 7617daa5-71fd-5a85-8f94-e28d735ce9f4 (nordicsemi.com)
                     downgradePreventionPolicy: downgrade allowed
                     independentUpdateabilityPolicy: independent update forbidden
                     signatureVerificationPolicy: signature verification disabled
                     digest: f391a4e298e0fc76d3b5d3044e379255484a2afde5e57d03dee5e07c9b4f558d
                     digestAlgorithm: sha256
                     signatureCheck: signature check not performed
                     sequenceNumber: 1
                     semantic version: 0.1.0

         #. Upload the image with nrfutil:

            .. code-block:: console

               nrfutil suit upload-envelope --serial-port COM7 --envelope-file root.suit

            You should see an output similar to the following logged on UART::

            .. code-block:: console

               [00:00:07] ###### 100% [COM7] Uploaded

      #. If you have built the application with :ref:`external flash support <nrf54h_suit_sample_extflash>`, upload the cache partition to the external flash using the following command:

         .. code-block:: console

            nrfutil suit upload-cache-raw --serial-port COM7 --cache-file dfu_cache_partition_1.bin --pool 1


         .. note::
            The ``--pool 1`` parameter uploads to DFU cache partition 1, where image 0 represents the envelope, and image 1 represents cache partition 0.

      #. Start the installation of the new firmware as follows:

         .. code-block:: console

            nrfutil suit install --serial-port COM7

      #. Read the sequence number of the uploaded root manifest with nrfutil:

         .. code-block:: console

             nrfutil suit manifests --serial-port COM7


         You should see an output similar to the following printed in the terminal:

         .. code-block:: console

            role(10) (Nordic Top)
                  classId: f03d385e-a731-5605-b15d-037f6da6097f (nRF54H20_nordic_top)
                  vendorId: 7617daa5-71fd-5a85-8f94-e28d735ce9f4 (nordicsemi.com)
                  downgradePreventionPolicy: downgrade forbidden
                  independentUpdateabilityPolicy: independent update allowed
                  signatureVerificationPolicy: signature verification on update and boot
                  digest: fd25c5e9d2a1bc8360a497c157d53f4634e8abe48d759065deb9fd84e9fbeece
                  digestAlgorithm: sha256
                  signatureCheck: signature check passed
                  sequenceNumber: 458752
                  semantic version: 0.7.0
            role(11) (SDFW and SDFW recovery updates)
                  classId: d96b40b7-092b-5cd1-a59f-9af80c337eba (nRF54H20_sec)
                  vendorId: 7617daa5-71fd-5a85-8f94-e28d735ce9f4 (nordicsemi.com)
                  downgradePreventionPolicy: downgrade forbidden
                  independentUpdateabilityPolicy: independent update forbidden
                  signatureVerificationPolicy: signature verification on update and boot
                  digest: deb46e703a49ec69524056a37d45d9534ff146194be6e4add53406e3014bdbd2
                  digestAlgorithm: sha256
                  signatureCheck: signature check passed
                  sequenceNumber: 134218240
                  semantic version: 8.0.2
            role(12) (System Controller Manifest)
                  classId: c08a25d7-35e6-592c-b7ad-43acc8d1d1c8 (nRF54H20_sys)
                  vendorId: 7617daa5-71fd-5a85-8f94-e28d735ce9f4 (nordicsemi.com)
                  downgradePreventionPolicy: downgrade forbidden
                  independentUpdateabilityPolicy: independent update forbidden
                  signatureVerificationPolicy: signature verification on update and boot
                  digest: 5dac29159363cbc50552a51ec8014d3927cbc854f83d8575af74ee27ee426c77
                  digestAlgorithm: sha256
                  signatureCheck: signature check passed
                  sequenceNumber: 50331648
                  semantic version: 3.0.0
            role(31) (Radio Local Manifest)
                  classId: 816aa0a0-af11-5ef2-858a-feb668b2e9c9 (nRF54H20_sample_rad)
                  vendorId: 7617daa5-71fd-5a85-8f94-e28d735ce9f4 (nordicsemi.com)
                  downgradePreventionPolicy: downgrade allowed
                  independentUpdateabilityPolicy: independent update forbidden
                  signatureVerificationPolicy: signature verification disabled
                  digest: 5fb501225293ab78ec0caf14457543514550044757de2fa8a5e764ec059eae0d
                  digestAlgorithm: sha256
                  signatureCheck: signature check not performed
                  sequenceNumber: 1
                  semantic version: 0.1.0
            role(20) (Root Manifest)
                  classId: 3f6a3a4d-cdfa-58c5-acce-f9f584c41124 (nRF54H20_sample_root)
                  vendorId: 7617daa5-71fd-5a85-8f94-e28d735ce9f4 (nordicsemi.com)
                  downgradePreventionPolicy: downgrade allowed
                  independentUpdateabilityPolicy: independent update allowed
                  signatureVerificationPolicy: signature verification disabled
                  digest: 87af8a96ff73cf6afa27a1cd506829f45f2303bcdbd51ec78bb8a49b0d7aa5fb
                  digestAlgorithm: sha256
                  signatureCheck: signature check not performed
                  sequenceNumber: 2
                  semantic version: 0.1.0
            role(22) (Application Local Manifest)
                  classId: 08c1b599-55e8-5fbc-9e76-7bc29ce1b04d (nRF54H20_sample_app)
                  vendorId: 7617daa5-71fd-5a85-8f94-e28d735ce9f4 (nordicsemi.com)
                  downgradePreventionPolicy: downgrade allowed
                  independentUpdateabilityPolicy: independent update forbidden
                  signatureVerificationPolicy: signature verification disabled
                  digest: e5ad42837da8b4e5a3c8fe78ff3355f0128e98f599689238eba827e768302966
                  digestAlgorithm: sha256
                  signatureCheck: signature check not performed
                  sequenceNumber: 1
                  semantic version: 0.1.0

         You should now see that **LED 0** flashes twice now to indicate "Version 2" of the firmware.
