.. _ug_nrf54h20_suit_recovery:

SUIT manufacturer application recovery
######################################

.. contents::
   :local:
   :depth: 2

During the device lifetime, the device may encounter various issues that can lead to boot failure.
In such cases the device needs a way to recover from the failure and continue to operate.

In SUIT, this is solved by running a specially prepared recovery firmware.
Although the recovery firmware needs some additional space in the device, it is a highly recommended feature for all devices that use SUIT.

This document describes scenarios in which a nRF54H20 device using SUIT can enter recovery mode and how the recovering process works.

When is recovery mode entered?
******************************

When booting the device, the Secure Domain verifies the currently installed manifests and firmware images.
If the verification fails, the device enters recovery mode.
The reasons for validation failure can be (among others):

* Invalid manifest signature.
* The digest of the installed firmware image does not match the digest in the manifest.
* The manifest attempts to perform unauthorized operations.

The reasons for the above failures can be:

* Tampering by an attacker.
* Bitflips in the MRAM memory due to radiation or other external conditions.
* Hardware failures in advanced cases where an inplace update is performed and a firmware image is partially overwritten.
* An incorrectly constructed manifest, leading to a succesful update but failure during boot.

The last case should never happen in a production environment, but it is possible during development.
The manufacturer should always ensure that the manifest is correctly constructed and all the images managed by SUIT are compatible with each other after the update.
Still, some bugs can be missed during the development phase, which could lead to inconsitencies in the firmware.

How the recovery mode and recovery manifests work
*************************************************

The recovery manifests form a separate hierarchy from the normal manifests.
In this hierarchy the application recovery (APP_RECOVERY) manifest has both the responsibility of managing the application core image as well as managing other manifests such as the radio recovery manifest.

If a failure during a boot process occured, the Secure Domain sets the recovery flag and reboots the device.
Upon each boot the Secure Domain checks if the recovery flag is set.
If it is set, the device enters recovery mode.
If it isn't booting proceeds normally by running the root manifest.

After entering the recovery mode it is verified if MPI configuration for the APP_RECOVERY is present.
If it is, the APP_RECOVERY manifest is processed.
It no MPI configuration is found the Secure Domain performs an attempt to process the normal manufacturer root manifest.
This is needed, as the device might enter recovery mode if it is empty.
The recovery flag might not be cleared after flashing the firmware, but the device should proceed as if it would boot normally.

The role of the recovery application is to perform an update of the main application firmware, which does not differ from the normal SUIT update process.
As soon as the update finishes succesfully, the recovery flag is cleared and the device proceeds with normal operation.

Default recovery firmware
*************************

Nordic provides a default recovery firmware that can be used in the recovery process.
This firmware uses Bluetooth LE and SMP as a transport.
It is optimized for memory usage, currently using around 164 kB of MRAM (72 kB of application core and 92 kB radio core).

.. note::
   The default recovery firmware does not support updating from external flash memory.

To use the firmware:

1. Create :file:`recovery.overlay` and :file:`recovery_hci_ipc.ovelay` files in the main application's :ref:`configuration_system_overview_sysbuild` directory.
   These files must define the ``cpuapp_recovery_partition`` and ``cpurad_recovery_partition`` nodes respectively.
   These partitions specify where the images for the recovery firmware are stored and cannot overlap with the main application partitions.
   For reference, see the files in the ``samples/suit/smp_transfer`` sample.

#. Set the ``SB_CONFIG_SUIT_BUILD_RECOVERY`` sysbuild configuration option in the main application.
   This will cause the recovery firmware to be built automatically as part of the main application build.

#. Flash the main application firmware to the device:

      .. code-block:: bash

         west flash

   This will automatically flash both the main application and the recovery firmware to the device.

To perform the recovery perform an update in the same way as described in :ref:`nrf54h_suit_sample`.

Further information about the default recovery firmware can be found in :ref:`suit_recovery`.
The code for the default recovery firmware can be found in the :file:`samples/suit/recovery` directory.

Updating the recovery firmware
******************************

To update the recovery firmware you can either use:

* The APP_RECOVERY envelope, found in :file:`<main_application_build_directory>/build/DFU/app_recovery.suit`
* The zip file, found in :file:`<main_application_build_directory>/build/zephyr/dfu_suit_recovery.zip``

These can be used to update the recovery application the same as :file:`root.suit` or :file:`dfu_suit.zip` are used to update the main application - see :ref:`nrf54h_suit_sample` as an example.

.. note::
   The recovery application can only be updated from the main application - not when running the recovery application itself.

Create custom recovery images
*****************************

To turn an application into a recovery application, the following steps have to be performed:

1. For each of the recovery firmware images, ensure the following configuration is present:

      .. code-block:: cfg

         CONFIG_SUIT_RECOVERY=y
         CONFIG_SUIT_MPI_GENERATE=n
         CONFIG_SUIT_ENVELOPE_OUTPUT_MPI_MERGE=n
         CONFIG_NRF_REGTOOL_GENERATE_UICR=n
         CONFIG_NRF_REGTOOL_GENERATE_BICR=n

#. Create the overlay files to be used by the recovery application.
   In this guide it is assumed that for the application core they are placed in the custom recovery application directory in the :file:`boards/nrf54h20dk_nrf54h20_cpuapp.overlay` file.

   The application core recovery image overlay should contain the following code:

      .. code-block:: dts

         / {
            chosen {
               zephyr,code-partition = &cpuapp_recovery_partition;
               nrf,tz-secure-image = &cpuapp_recovery_partition;
            };
         };

         &cpusec_cpuapp_ipc {
            status = "okay";
         };

         &cpusec_bellboard {
            status = "okay";
         };

   Optionally, if using the radio core recovery image, the radio core recovery image overlay should contain the following code:

      .. code-block:: dts

         / {
            chosen {
               zephyr,code-partition = &cpurad_recovery_partition;
               nrf,tz-secure-image = &cpurad_recovery_partition;
            };
         };

#. Add :file:`sysbuild.cmake` to the custom recovery application directory.
   In this file add the following code:

      .. code-block:: cmake

         add_overlay_dts(recovery ${CMAKE_CURRENT_LIST_DIR}/boards/nrf54h20dk_nrf54h20_cpuapp.overlay)

   This will ensure that when building from the main application directory the overlay file is attached to and not overwritten by the configuration comming from the main application.

#. If you want to add additional images to the recovery image, you can add it with code similar to the one from the default recovery firmware image:

      .. code-block:: cmake

         ExternalZephyrProject_Add(
            APPLICATION recovery_hci_ipc
            SOURCE_DIR  "${ZEPHYR_BASE}/samples/bluetooth/hci_ipc"
            BOARD       ${BOARD}/${SB_CONFIG_SOC}/${SB_CONFIG_NETCORE_REMOTE_BOARD_TARGET_CPUCLUSTER}
            BOARD_REVISION ${BOARD_REVISION}
         )

         add_overlay_config(recovery_hci_ipc ${CMAKE_CURRENT_LIST_DIR}/sysbuild/hci_ipc.conf)
         add_overlay_dts(recovery_hci_ipc ${CMAKE_CURRENT_LIST_DIR}/sysbuild/hci_ipc.overlay)

   Replace recovery_hci_ipc, hci_ipc and ``SOURCE_DIR`` with the appropriate values for your application.


#. Optionally - you can modify the recovery manifest templates.
   The manifest template defined by the `CONFIG_SUIT_ENVELOPE_TEMPLATE_FILENAME` is first searched for in :file:`suit/<soc>` in the main application directory.
   If it is not found, :file:`suit/<soc>` in the the recovery app is checked.
   If the manifest template is still not found, the default template directory in NCS is checked (:file:`config/suit/templates`).
