.. _lwm2m_client_fota_external_mcu:

Evaluating LwM2M Advanced Firmware Update for external MCU
##########################################################

.. contents::
   :local:
   :depth: 2

This page describes how to evaluate FOTA to external MCU using LwM2M with Advanced Firmware Update object on the nRF9160 DK.

Connecting nRF9160 SiP to nRF52840 SoC
**************************************

.. include:: /includes/connecting_soc.txt
The nRF9160 SiP is configured as an MCUmgr client and nRF52840 SoC as a server.

External MCU FOTA options
*************************

By default, the Advanced Firmware Update object supports two instances.
To support an external DFU SMP target, the extra instance is defined for SMP FOTA as follows:

+-------------+-------------+---------------------------------------------------------------------+
| Instance ID | Owner       | DFU types                                                           |
+=============+=============+=====================================================================+
| 0           | Application | DFU_TARGET_IMAGE_TYPE_MCUBOOT                                       |
+-------------+-------------+---------------------------------------------------------------------+
| 1           | Modem       | DFU_TARGET_IMAGE_TYPE_MODEM_DELTA, DFU_TARGET_IMAGE_TYPE_FULL_MODEM |
+-------------+-------------+---------------------------------------------------------------------+
| 2           | SMP         | DFU_TARGET_SMP                                                      |
+-------------+-------------+---------------------------------------------------------------------+

The sample supports firmware update from the nRF9160 to nRF52840.
External FOTA SMP adds a new instance to the Advanced FOTA object and registers that to the device management server.

Some additional :ref:`overlay files <overlay_advanced_fw_object>` need to be included in the sample configuration for evaluating LwM2M Advanced Firmware Update for external MCU.
For the purpose of evaluating, the :ref:`smp_svr` sample is used.
It is a reference implementation that uses MCUboot recovery mode or serial SMP server mode to allow image update.

Verifying image update
**********************

The following steps use generic MCUmgr client mode and Coiote without bootstrap.

#. Set up the :file:`/scripts/fota.py` script by using the same username and password that you used for AVSystem's `Coiote Device Management server`_:

   .. code-block:: console

      # Setup phase
      export COIOTE_PASSWD='my-password'
      export COIOTE_USER='my-username'

#. Build the :ref:`smp_svr` sample and flash it to the nRF9160 DK after making sure that the **PROG/DEBUG** switch is set to **nRF52**.

   .. code-block:: console

      cd samples/cellular/smp_svr/
      west build --pristine -b nrf9160dk/nrf52840 -- -DEXTRA_CONF_FILE="overlay-serial.conf" -DEXTRA_DTC_OVERLAY_FILE="nrf9160dk_nrf52840_mcumgr_srv.overlay"
      west flash --erase

#. Open the :file:`prj.conf` file.
#. Change :kconfig:option:`CONFIG_MCUBOOT_IMGTOOL_SIGN_VERSION` to ``1.1.0``.
#. Rebuild the sample and copy the binary file to the :file:`lwm2m_client` folder:

   .. code-block:: console

      west build
      cp build/zephyr/app_update.bin ../lwm2m_client/app_update52.bin
      cd ..

#. Open a terminal connection for the nRF52840 SoC (VCOM1), in Linux ``/dev/ttyACM1``, and call the ``mcuboot`` command on the shell.

   .. code-block:: console

      uart:~$ mcuboot
      swap type: none
      confirmed: 1

      primary area (2):
         version: 1.0.0+0
         image size: 70136
         magic: unset
         swap type: none
         copy done: unset
         image ok: unset

      failed to read secondary area (5) header: -5
      uart:~$

#. :ref:`Register your nRF9160 DK with the Coiote Device management server <add_device_coiote>`.
#. Build and flash the :ref:`lwm2m_client` sample after making sure the **PROG/DEBUG** switch is set to **nRF91**.

   .. code-block:: console

      cd lwm2m_client/
      west build  --pristine -b nrf9160dk/nrf9160/ns --  -DEXTRA_CONF_FILE="overlay-adv-firmware.conf;overlay-fota_helper.conf;overlay-avsystem.conf;overlay-lwm2m-1.1.conf;overlay-mcumgr_client.conf" -DEXTRA_DTC_OVERLAY_FILE="nrf9160dk_mcumgr_client_uart2.overlay"
      west flash --erase

#. Wait for the device registration to complete.
#. Use the :file:`/scripts/fota.py` script to update the nRF52840 SoC firmware.

   .. code-block:: console

      ./scripts/fota.py -id urn:imei:351358811331351 update 2 file app_update52.bin

   Replace the ``urn:imei:...`` with your device IMEI from the above.
   This value is printed on the development kit.

   Following is an example output of the command:

   .. code-block:: console

      [INFO] fota.py - Client identity: urn:imei:351358811331351
      [INFO] fota.py - Binary app_update52.bin, Size 70799 (bytes)
      [INFO] coiote.py - Creating fota resource for binary app_update52.bin with id lwm2m_client_fota_instance_2
      [INFO] fota.py - Init setup for instance 2 firmware Update resource:lwm2m_client_fota_instance_2
      [INFO] fota.py - Start Firmware Update
      [INFO] fota.py - Delete Observation Multicomponent Firmware Update
      [WARNING] coiote.py - Coiote: Path 'Multicomponent Firmware Update' was not observed
      [INFO] fota.py - Write Fota Download url to Multicomponent Firmware Update.2.Package URI
      [INFO] coiote.py - Device is Queuemode Coiote have to wait next Registration Update
      [INFO] fota.py - Download Url Write done
      [INFO] fota.py - Enable Observation Multicomponent Firmware Update
      [INFO] fota.py - Downloading instance: 2
      [INFO] fota.py - Download ready instance: 2
      [INFO] coiote.py - Device is Queuemode Coiote have to wait next Registration Update
      [INFO] fota.py - Firmware Update Successfully instance: 2
      [INFO] fota.py - From: to
      [INFO] fota.py - Firmware update process finished
      [INFO] fota.py - Delete Observation Multicomponent Firmware Update

#. Call the ``mcuboot`` shell command on the terminal to validate the updated image version on the nRF52840 SoC:

   .. code-block:: console

      *** Booting Zephyr OS build v3.3.99-ncs1-2938-gc7094146b5b4 ***
      I: Starting bootloader
      I: Primary image: magic=good, swap_type=0x2, copy_done=0x1, image_ok=0x1
      I: Secondary image: magic=good, swap_type=0x2, copy_done=0x3, image_ok=0x3
      I: Boot source: none
      I: Swap type: test
      I: Starting swap using move algorithm.
      I: Bootloader chainload address offset: 0xc000
      �: Jumping to the first image slot

      <inf> board_control: vcom0_pins_routing is ENABLED
      <inf> board_control: vcom2_pins_routing is ENABLED
      <inf> board_control: led1_pin_routing is ENABLED
      <inf> board_control: led2_pin_routing is ENABLED
      <inf> board_control: led3_pin_routing is ENABLED
      <inf> board_control: led4_pin_routing is ENABLED
      <inf> board_control: switch1_pin_routing is ENABLED
      <inf> board_control: switch2_pin_routing is ENABLED
      <inf> board_control: button1_pin_routing is ENABLED
      <inf> board_control: button2_pin_routing is ENABLED
      <inf> board_control: nrf_interface_pins_0_2_routing is disabled
      <inf> board_control: nrf_interface_pins_3_5_routing is disabled
      <inf> board_control: nrf_interface_pins_6_8_routing is disabled
      <inf> board_control: nrf_interface_pin_9_routing is ENABLED
      <inf> board_control: io_expander_pins_routing is disabled
      <inf> board_control: external_flash_pins_routing is ENABLED
      <inf> board_control: Board configured.
      *** Booting Zephyr OS build v3.3.99-ncs1-2938-gc7094146b5b4 ***
      <inf> app_smp_svr: Booting image: build time: Sep  4 2023 10:38:09
      <inf> app_smp_svr: Image Version 1.1.0-0
      <inf> app_smp_svr: Image is not confirmed OK
      <inf> app_smp_svr: Marked image as OK
      uart:~$ mcuboot
      swap type: none
      confirmed: 1

      primary area (2):
        version: 1.1.0+0
        image size: 70136
        magic: good
        swap type: test
        copy done: set
        image ok: set

      secondary area (5):
        version: 1.0.0+0
        image size: 70136
        magic: unset
        swap type: none
        copy done: unset
        image ok: unset
      uart:~$

   You can see that MCUboot swaps the running image to version 1.1.0 and the previous image is on the secondary slot.
