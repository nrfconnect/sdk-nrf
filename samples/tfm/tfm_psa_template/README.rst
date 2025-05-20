.. _tfm_psa_template:

TF-M: PSA template
##################

.. contents::
   :local:
   :depth: 2

This sample provides a template for Arm's `Platform Security Architecture (PSA)`_ best practices on Nordic Semiconductor devices.

Requirements
************

The sample supports the following development kits:

.. table-from-sample-yaml::

.. include:: /includes/tfm.txt

Overview
********

This sample uses :ref:`Trusted Firmware-M <ug_tfm>`, :ref:`MCUboot and nRF Secure Immutable Bootloader (NSIB)<ug_bootloader_mcuboot_nsib>` to demonstrate how to implement the best practices that comply with the Arm PSA requirements.
It includes provisioning the device with keys and being able to perform a device firmware update.
The sample prints information about the identity of the device and the firmware versions that are currently running.

On the nRF5340 devices, this sample also includes the :ref:`B0n bootloader <nc_bootloader>` and the :ref:`empty_net_core <nrf5340_empty_net_core>` image for demonstrating the network core firmware update process.

Building and running
********************

.. |sample path| replace:: :file:`samples/tfm/tfm_psa_template`

.. include:: /includes/build_and_run_ns.txt

The sample requires that the device is provisioned using the :ref:`provisioning image <provisioning_image>` sample.
Build and flash the provisioning image sample to provision the device with the PSA root-of-trust security parameters.

.. code-block:: console

    west build -b nrf5340dk/nrf5340/cpuapp nrf/samples/tfm/provisioning_image -d build_provisioning_image
    west flash --erase --recover -d build_provisioning_image

Build and flash the TF-M PSA template sample.
Do not flash with ``--erase`` as this will erase the PSA platform security parameters and they will be lost.

.. code-block:: console

    west build -b nrf5340dk/nrf5340/cpuapp/ns nrf/samples/tfm/tfm_psa_template
    west flash

Testing
=======

After programming the sample, the following output is displayed in the console:

.. code-block:: console

    *** Booting nRF Connect SDK v2.9.99-ad7a4f0e68fa ***
    *** Using Zephyr OS v3.7.99-f5efb381b8af ***
    Attempting to boot slot 0.
    Attempting to boot from address 0x8200.
    I: Trying to get Firmware version
    I: Verifying signature against key 0.
    I: Hash: 0xf0...99
    I: Firmware signature verified.
    Firmware version 1
    *** Booting My Application v2.1.0-dev-b82206c15fff ***
    *** Using nRF Connect SDK v2.9.99-ad7a4f0e68fa ***
    *** Using Zephyr OS v3.7.99-f5efb381b8af ***
    I: Starting bootloader
    I: Image index: 0, Swap type: none
    I: Image index: 1, Swap type: none
    I: Image index: 2, Swap type: none
    I: Bootloader chainload address offset: 0x20000
    *** Booting nRF Connect SDK v2.9.99-ad7a4f0e68fa ***
    *** Using Zephyr OS v3.7.99-f5efb381b8af ***
    build time: Dec 19 2024 08:31:12

    FW info S0:
    Magic: 0x281ee6de8fcebb4c00003502
    Total Size: 140
    Size: 0x0000805c
    Version: 1
    Address: 0x00008200
    Boot address: 0x00008200
    Valid: 0x9102ffff (CONFIG_FW_INFO_VALID_VAL=0x9102ffff)

    FW info S1:
    Magic: 0x281ee6de8fcebb4c00003502
    Total Size: 140
    Size: 0x0000805c
    Version: 1
    Address: 0x00014200
    Boot address: 0x00014200
    Valid: 0x9102ffff (CONFIG_FW_INFO_VALID_VAL=0x9102ffff)

    Active slot: S0

    Requesting initial attestation token with 64 byte challenge.
    Received initial attestation token of 360 bytes.

    0  1  2  3  4  5  6  7   8  9  A  B  C  D  E  F
    D2 84 43 A1 01 26 A0 59  01 1C AA 3A 00 01 24 FF  |  ..C..&.Y...:..$.
    58 40 00 11 22 33 44 55  66 77 88 99 AA BB CC DD  |  X@.."3DUfw......
    EE FF 00 11 22 33 44 55  66 77 88 99 AA BB CC DD  |  ...."3DUfw......
    EE FF 00 11 22 33 44 55  66 77 88 99 AA BB CC DD  |  ...."3DUfw......
    EE FF 00 11 22 33 44 55  66 77 88 99 AA BB CC DD  |  ...."3DUfw......
    EE FF 3A 00 01 24 FB 58  20 20 E7 18 5E 6F F1 0A  |  ..:..$.X  ..^o..
    F7 C0 04 63 A9 6C 73 78  65 92 11 02 EC DF 09 EF  |  ...c.lsxe.......
    4C BD 13 AD 69 A3 33 AD  AE 3A 00 01 25 00 58 21  |  L...i.3..:..%.X!
    01 69 48 1A 3C 87 E2 6E  48 D3 15 8A 45 F1 D8 E4  |  .iH.<..nH...E...
    6A 00 A0 14 0C 87 E5 5E  3F B3 B6 98 45 8F 36 5E  |  j......^?...E.6^
    BB 3A 00 01 24 FA 58 20  AA AA AA AA AA AA AA AA  |  .:..$.X ........
    BB BB BB BB BB BB BB BB  CC CC CC CC CC CC CC CC  |  ................
    DD DD DD DD DD DD DD DD  3A 00 01 24 F8 3A 3B FF  |  ........:..$.:;.
    FF FF 3A 00 01 24 F9 19  30 00 3A 00 01 24 FE 01  |  ..:..$..0.:..$..
    3A 00 01 24 F7 71 50 53  41 5F 49 4F 54 5F 50 52  |  :..$.qPSA_IOT_PR
    4F 46 49 4C 45 5F 31 3A  00 01 25 01 70 76 65 72  |  OFILE_1:..%.pver
    69 66 69 63 61 74 69 6F  6E 5F 75 72 6C 3A 00 01  |  ification_url:..
    24 FC 73 30 36 33 32 37  39 33 35 31 39 35 33 39  |  $.s0632793519539
    2D 31 30 31 30 30 58 40  F9 04 F8 56 4F 89 A0 67  |  -10100X@...VO..g
    2C 21 31 6C 88 EF D6 34  D1 02 4F 6D EA 17 54 9F  |  ,!1l...4..Om..T.
    B6 90 E4 6E D6 55 4E D3  62 8C 9D 6A 0F 67 8D 9E  |  ...n.UN.b..j.g..
    D7 05 45 3C 89 BE C2 9B  2B D0 ED 05 F1 AC 42 21  |  ..E<....+.....B!
    F0 05 00 CE B7 B9 47 E9                           |  ......G.

.. _tfm_psa_template_fw_update:

Configuring firmware update
===========================

This sample supports firmware update of both the application and TF-M, and the second stage bootloader.

The firmware update process requires :ref:`signature verification keys <ug_fw_update_keys>` to sign the images used in the process.
The nRF Secure Immutable bootloader and MCUboot will use signing keys that should not be used in production.
For signing and verifying images, use ECDSA with secp256r1-sha256, which is supported by the |NCS| cryptographic libraries :ref:`nrf_oberon_readme` and :ref:`crypto_api_nrf_cc310_bl`.

The following steps are an example of how to generate the keys and update the configuration files with them:

1. Generate security keys if you have not already done so.
   The following example generates keys using :ref:`Imgtool <ug_fw_update_keys_imgtool>`, but you can also use :ref:`OpenSSL <ug_fw_update_keys_openssl>`.

   .. code-block:: console

      mkdir _keys
      python3 bootloader/mcuboot/scripts/imgtool.py keygen -t ecdsa-p256 -k ~/ncs/_keys/mcuboot_priv.pem
      python3 bootloader/mcuboot/scripts/imgtool.py keygen -t ecdsa-p256 -k ~/ncs/_keys/nsib_priv.pem

#. Update the :file:`sysbuild.conf` file to set the private signing keys for MCUboot and NSIB:

   .. note::
        Use only absolute paths for ``SB_CONFIG_BOOT_SIGNATURE_KEY_FILE`` and ``SB_CONFIG_SECURE_BOOT_SIGNING_KEY_FILE``.

   .. code-block:: console

      SB_CONFIG_BOOT_SIGNATURE_KEY_FILE="/home/user/ncs/_keys/mcuboot_priv.pem"
      SB_CONFIG_SECURE_BOOT_SIGNING_KEY_FILE="/home/user/ncs/_keys/nsib_priv.pem"

#. Rebuild the sample with the updated keys before proceeding with the firmware update as described in the next sections.

The bootloader and the application can be updated using the :file:`mcumgr` command-line tool.
See :zephyr:code-sample:`smp-svr` for installation and usage instructions.

Updating application and TF-M firmware
--------------------------------------

You can use firmware update to update the application and TF-M firmware.
For the image to be updatable, the firmware image version :kconfig:option:`CONFIG_MCUBOOT_IMGTOOL_SIGN_VERSION` must be updated to a higher version.
See :ref:`ug_fw_update_image_versions_mcuboot_downgrade` for information on downgrade protection in MCUboot.

To upload a new application image, complete the following steps:

1. Build an application with an updated image version:

   .. code-block:: console

      west build -b nrf5340dk/nrf5340/cpuapp/ns nrf/samples/tfm/tfm_psa_template -d build_update \
      -DCONFIG_MCUBOOT_IMGTOOL_SIGN_VERSION=\"1.2.3\"

#. |mcumgr_installation_step|
#. Upload the new application image to the device using mcumgr:

   .. code-block:: console

      mcumgr --conntype serial --connstring dev=/dev/ttyACM1,baud=115200,mtu=512 image list
      mcumgr --conntype serial --connstring dev=/dev/ttyACM1,baud=115200,mtu=512 image upload \
      build_update/tfm_psa_template/zephyr/zephyr.signed.bin

   Once the new application image is uploaded, the hash of the image is shown in the image list.
#. Flag the image to be tested on next reboot using its hash:

   .. code-block:: console

      mcumgr --conntype serial --connstring dev=/dev/ttyACM1,baud=115200,mtu=512 image list
      mcumgr --conntype serial --connstring dev=/dev/ttyACM1,baud=115200,mtu=512 image test <hash>

#. Trigger the application update by initiating a reset.
   The verification of the image will happen during the update process.

   .. code-block:: console

      mcumgr --conntype serial --connstring dev=/dev/ttyACM1,baud=115200,mtu=512 reset

Updating bootloader firmware
----------------------------

To upload a new bootloader image, complete the following steps:

1. Build a bootloader targeting the correct bootloader slot with an updated firmware image version:

   .. code-block:: console

      west build -b nrf5340dk/nrf5340/cpuapp/ns nrf/samples/tfm/tfm_psa_template -d build_update \
      -Dmcuboot_CONFIG_FW_INFO_FIRMWARE_VERSION=2

#. |mcumgr_installation_step|
#. List the current firmware images and upload a bootloader image that targets the non-active bootloader slot.

   .. code-block:: console

      mcumgr --conntype serial --connstring dev=/dev/ttyACM1,baud=115200,mtu=512 image list
      mcumgr --conntype serial --connstring dev=/dev/ttyACM1,baud=115200,mtu=512 image upload \
      build_update/signed_by_mcuboot_and_b0_s1_image.bin

   Once the new bootloader image is uploaded, the hash of the image is shown in the image list.
#. Flag the image to be tested on next reboot using its hash:

   .. code-block:: console

      mcumgr --conntype serial --connstring dev=/dev/ttyACM1,baud=115200,mtu=512 image test <hash>

#. Trigger the bootloader update by initiating a reset.
   The verification of the image will happen during the update process.

   .. code-block:: console

      mcumgr --conntype serial --connstring dev=/dev/ttyACM1,baud=115200,mtu=512 reset

Updating the network core (nRF5340 only)
----------------------------------------

To upload a new network core image, complete the following steps:

1. Build the ``empty_net_core`` image with an updated firmware image version:

   .. code-block:: console

      west build -b nrf5340dk/nrf5340/cpuapp/ns nrf/samples/tfm/tfm_psa_template -d build_update \
      -Dempty_net_core_CONFIG_FW_INFO_FIRMWARE_VERSION=2

#. |mcumgr_installation_step|
#. Upload the new network core image to the device:

   .. note::

      The image is uploaded to the network core slot.

   .. code-block:: console

      mcumgr --conntype serial --connstring dev=/dev/ttyACM1,baud=115200,mtu=512 image upload \
      build_update/signed_by_mcuboot_and_b0_empty_net_core.bin -e -n 1

      mcumgr --conntype serial --connstring dev=/dev/ttyACM1,baud=115200,mtu=512 image list

   Once the network core image is uploaded, the hash of the image is shown in the image list as image 1 in slot 1.
#. Flag the image to be tested on next reboot using its hash:

   .. code-block:: console

      mcumgr --conntype serial --connstring dev=/dev/ttyACM1,baud=115200,mtu=512 image test <hash>

#. Trigger the network core update by initiating a reset.
   The verification of the image will happen during the update process.

   .. code-block:: console

      mcumgr --conntype serial --connstring dev=/dev/ttyACM1,baud=115200,mtu=512 reset

Alternatively, you can conduct a manual reset to trigger the network core update.
This allows you to observe the update process in the application and network core console outputs.

Updating application and network cores simultaneously (nRF5340 only)
--------------------------------------------------------------------

When the interface between the application core and the network core is updated, both the application and network core images must be updated simultaneously.
To do this, complete the following steps:

1. Build the application image with an updated image version and the network core image with an updated firmware image version:

   .. code-block:: console

      west build -b nrf5340dk/nrf5340/cpuapp/ns nrf/samples/tfm/tfm_psa_template -d build_update \
      -DCONFIG_MCUBOOT_IMGTOOL_SIGN_VERSION=\"1.2.4\" -Dempty_net_core_CONFIG_FW_INFO_FIRMWARE_VERSION=3

#. |mcumgr_installation_step|
#. Upload the new application and network core images to the device.

   .. note::
      The application image is uploaded to the application slot, and the network core image is uploaded to the network core slot.

   .. code-block:: console

      mcumgr --conntype serial --connstring dev=/dev/ttyACM1,baud=115200,mtu=512 image upload \
      build_update/tfm_psa_template/zephyr/zephyr.signed.bin -e -n 0

      mcumgr --conntype serial --connstring dev=/dev/ttyACM1,baud=115200,mtu=512 image upload \
      build_update/signed_by_mcuboot_and_b0_empty_net_core.bin -e -n 1

      mcumgr --conntype serial --connstring dev=/dev/ttyACM1,baud=115200,mtu=512 image list

   Once the images are uploaded, the hash of the images is shown in the image list.
   The application image is image 1 in slot 0, and the network core image is image 1 in slot 1.
#. To allow the application and network core images to be updated simultaneously, first confirm the network core image and then the application image:

   .. code-block:: console

      mcumgr --conntype serial --connstring dev=/dev/ttyACM1,baud=115200,mtu=512 image confirm <network core image hash>

      mcumgr --conntype serial --connstring dev=/dev/ttyACM1,baud=115200,mtu=512 image confirm <application core image hash>

#. Trigger the core updates by initiating a reset.
   The verification of the images will happen during the update process.

   .. code-block:: console

      mcumgr --conntype serial --connstring dev=/dev/ttyACM1,baud=115200,mtu=512 reset

Alternatively, you can conduct a manual reset to trigger the core updates.
This allows you to observe the update process in the application and network core console outputs.

Dependencies
************

* This sample uses the TF-M module found in the :file:`modules/tee/tfm/` folder of the |NCS|.
* This sample uses the :ref:`lib_tfm_ioctl_api` library.
* On the nRF5340 devices, this sample uses the :ref:`subsys_pcd` library.

.. |mcumgr_installation_step| replace:: Install the :file:`mcumgr` command-line tool if you have not already done so.
   See :ref:`dfu_tools_mcumgr_cli` for installation and usage instructions.
