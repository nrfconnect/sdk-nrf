.. _tfm_psa_template:

TF-M: PSA template
##################

.. contents::
   :local:
   :depth: 2

This sample provides a template for Arm Platform Security Architecture (PSA) best practices on nRF devices.

Requirements
************

The sample supports the following development kits:

.. table-from-sample-yaml::

.. include:: /includes/tfm.txt

Overview
********

This sample uses Trusted Firmware-M, nRF Secure Immutable bootloader and MCUboot to demonstrate how to implement the best practices that comply with the Arm PSA requirements.
It includes provisioning the device with keys and being able to perform a device firmware update.
The sample prints information about the identity of the device and the firmware versions that are currently running.

Building and running
********************

.. |sample path| replace:: :file:`samples/tfm/tfm_psa_template`

.. include:: /includes/build_and_run_ns.txt

The sample requires that the device is provisioned using the :ref:`provisioning image <provisioning_image>` sample.
Build and flash the provisioning image sample to provision the device with the PSA root-of-trust security parameters.

.. code-block:: console

    west build -b nrf5340dk_nrf5340_cpuapp nrf/samples/tfm/provisioning_image -d build_provisioning_image
    west flash --erase -d build_provisioning_image

Build and flash the TF-M PSA template sample.
Do not flash with ``--erase`` as this will erase the PSA platform security parameters and they will be lost.

.. code-block:: console

    west build -b nrf5340dk_nrf5340_cpuapp_ns nrf/samples/tfm/tfm_psa_template
    west flash

Testing
=======

After programming the sample, the following output is displayed in the console:

.. code-block:: console

    *** Booting Zephyr OS build v3.2.0-rc3-510-g7c71945d42f6  ***
    Attempting to boot slot 0.
    Attempting to boot from address 0x8200.
    Verifying signature against key 0.
    Hash: 0xde...4b
    Firmware signature verified.
    Firmware version 1
    *** Booting Zephyr OS build v3.2.0-rc3-510-g7c71945d42f6  ***
    I: Starting bootloader
    I: Swap type: none
    I: Swap type: none
    I: Bootloader chainload address offset: 0x28000
    *** Booting Zephyr OS build v3.2.0-rc3-510-g7c71945d42f6  ***
    build time: Nov  2 2022 16:26:33

    FW info S0:
    Magic: 0x281ee6de8fcebb4c00003502
    Total Size: 60
    Size: 0x00009658
    Version: 1
    address: 0x00008200
    boot address: 0x00008200
    Valid: 0x9102ffff (CONFIG_FW_INFO_VALID_VAL=0x9102ffff)

    FW info S1:
    Failed to retrieve fw_info for address 0x00018000

    Active slot: S0

    Requesting initial attestation token with 64 byte challenge
    Received initial attestation token of 360 bytes.

    0  1  2  3  4  5  6  7   8  9  A  B  C  D  E  F
    D2 84 43 A1 01 26 A0 59  01 1C AA 3A 00 01 24 FF  |  ..C..&.Y...:..$.
    58 40 00 11 22 33 44 55  66 77 88 99 AA BB CC DD  |  X@.."3DUfw......
    EE FF 00 11 22 33 44 55  66 77 88 99 AA BB CC DD  |  ...."3DUfw......
    EE FF 00 11 22 33 44 55  66 77 88 99 AA BB CC DD  |  ...."3DUfw......
    EE FF 00 11 22 33 44 55  66 77 88 99 AA BB CC DD  |  ...."3DUfw......
    EE FF 3A 00 01 24 FB 58  20 26 0A 95 20 6B F8 17  |  ..:..$.X &.. k..
    3C 65 E9 12 C1 7F 12 57  C2 26 85 56 0E 27 FE 37  |  <e.....W.&.V.'.7
    90 B7 67 FD 07 86 D8 E5  3A 3A 00 01 25 00 58 21  |  ..g.....::..%.X!
    01 8B A0 C3 7E 96 62 29  C3 90 14 D9 E1 CF B6 6F  |  ....~.b).......o
    71 07 37 76 BF B8 E3 96  94 7D 09 1E 37 07 21 DB  |  q.7v.....}..7.!.
    84 3A 00 01 24 FA 58 20  AA AA AA AA AA AA AA AA  |  .:..$.X ........
    BB BB BB BB BB BB BB BB  CC CC CC CC CC CC CC CC  |  ................
    DD DD DD DD DD DD DD DD  3A 00 01 24 F8 20 3A 00  |  ........:..$. :.
    01 24 F9 19 30 00 3A 00  01 24 FE 01 3A 00 01 25  |  .$..0.:..$..:..%
    01 77 77 77 77 2E 74 72  75 73 74 65 64 66 69 72  |  .wwww.trustedfir
    6D 77 61 72 65 2E 6F 72  67 3A 00 01 24 F7 71 50  |  mware.org:..$.qP
    53 41 5F 49 4F 54 5F 50  52 4F 46 49 4C 45 5F 31  |  SA_IOT_PROFILE_1
    3A 00 01 24 FC 70 30 36  30 34 35 36 35 32 37 32  |  :..$.p0604565272
    38 32 39 31 30 30 58 40  BD 2F 65 4C 56 5A 9F 01  |  829100X@./eLVZ..
    F2 D4 9C FB F4 25 9D C5  11 D0 57 B6 23 F1 D9 99  |  .....%....W.#...
    2D A0 AC 39 F8 D8 39 A6  81 A7 E0 DC 8B BA A6 9F  |  -..9..9.........
    EB AE 50 55 C3 DD 1F 6A  FF E3 AB 98 5D CC 2F E9  |  ..PU...j....]./.
    54 77 40 BE D9 FF AE 7F                           |  Tw@.....

Firmware update
***************

This sample supports firmware update of both the application and TF-M, and the second stage bootloader.

The firmware update process requires signature verification keys in order to sign the images used in the process.
The nRF Secure Immutable bootloader and MCUboot will use signing keys that should not be used in production.
For signing and verifying images, use ECDSA with secp256r1-sha256, which is supported by the |NCS| cryptographic libraries :ref:`nrf_oberon_readme` and :ref:`crypto_api_nrf_cc310_bl`.

Below is an example of how to generate and use the keys.

Generate security keys if needed:

.. code-block:: console

    python3 bootloader/mcuboot/scripts/imgtool.py keygen -t ecdsa-p256 -k /home/user/ncs/_keys/mcuboot_priv.pem
    python3 bootloader/mcuboot/scripts/imgtool.py keygen -t ecdsa-p256 -k /home/user/ncs/_keys/nsib_priv.pem

Update the ``child_image/mcuboot/prj.conf`` file to set the private signing key for MCUBoot:

.. code-block:: console

    CONFIG_BOOT_SIGNATURE_KEY_FILE="/home/user/ncs/_keys/mcuboot_priv.pem"
    CONFIG_BOOT_SIGNATURE_TYPE_RSA=n
    CONFIG_BOOT_SIGNATURE_TYPE_ECDSA_P256=y

Update the ``prj.conf`` file to set the private signing key for NSIB:

.. code-block:: console

    CONFIG_SB_SIGNING_KEY_FILE="/home/user/ncs/_keys/nsib_priv.pem"

See :ref:`ug_fw_update_keys` for more information on how to generate and use keys for a project.

The bootloader and the application can be updated using the :file:`mcumgr` command-line tool.
See :ref:`smp_svr_sample` for installation and usage instructions.

Application and TF-M firmware update
====================================

Use firmware update to update the application and TF-M firmware.
For the image to be updatable, the firmware image version :kconfig:option:`CONFIG_MCUBOOT_IMAGE_VERSION` has to be updated to a higher version.
See :ref:`ug_fw_update_image_versions_mcuboot_downgrade` for information on downgrade protection in MCUboot.

To upload a new application image, build an application with an updated image version.

.. code-block:: console

    west build -b nrf5340dk_nrf5340_cpuapp_ns nrf/samples/tfm/tfm_psa_template -d build_update \
    -DCONFIG_MCUBOOT_IMAGE_VERSION=\"1.2.3\"

Then upload the new application image to the device.

.. code-block:: console

    mcumgr --conntype serial --connstring dev=/dev/ttyACM2,baud=115200,mtu=512 image list
    mcumgr --conntype serial --connstring dev=/dev/ttyACM2,baud=115200,mtu=512 image upload \
    build_update/zephyr/app_update.bin

Once the new application image is uploaded, the hash of the image is shown in the image list.
Flag the image to be tested on next reboot using its hash.

.. code-block:: console

    mcumgr --conntype serial --connstring dev=/dev/ttyACM2,baud=115200,mtu=512 image list
    mcumgr --conntype serial --connstring dev=/dev/ttyACM2,baud=115200,mtu=512 image test <hash>

Trigger the application update by initiating a reset.
The verification of the image will happen during the update process.

.. code-block:: console

    mcumgr --conntype serial --connstring dev=/dev/ttyACM2,baud=115200,mtu=512 reset

Bootloader firmware update
==========================

To upload a new bootloader image, build a bootloader targeting the correct bootloader slot with an updated firmware image version.
The bootloader is placed in slot 0 by default, so enable building of the slot 1 bootloader.

    west build -b nrf5340dk_nrf5340_cpuapp_ns nrf/samples/tfm/tfm_psa_template \
    -DCONFIG_BUILD_S1_VARIANT=y \
    -Dmcuboot_CONFIG_FW_INFO_FIRMWARE_VERSION=2

List the current firmware images and upload a bootloader image that targets the non-active bootloader slot.

.. code-block:: console

    mcumgr --conntype serial --connstring dev=/dev/ttyACM2,baud=115200,mtu=512 image list
    mcumgr --conntype serial --connstring dev=/dev/ttyACM2,baud=115200,mtu=512 image upload \
    build/zephyr/signed_by_mcuboot_and_b0_s1_image_update.bin

Once the new bootloader image is uploaded, the hash of the image is shown in the image list.
Flag the image to be tested on next reboot using its hash.

.. code-block:: console

    mcumgr --conntype serial --connstring dev=/dev/ttyACM2,baud=115200,mtu=512 image list
    mcumgr --conntype serial --connstring dev=/dev/ttyACM2,baud=115200,mtu=512 image test <hash>

Trigger the bootloader update by initiating a reset.
The verification of the image will happen during the update process.

.. code-block:: console

    mcumgr --conntype serial --connstring dev=/dev/ttyACM2,baud=115200,mtu=512 reset


Dependencies
*************

This sample uses the TF-M module that can be found in the following location in the |NCS| folder structure:

* :file:`modules/tee/tfm/`

This sample uses the following libraries:

* :ref:`lib_tfm_ioctl_api`
