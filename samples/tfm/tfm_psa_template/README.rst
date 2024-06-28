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

    west build -b nrf5340dk/nrf5340/cpuapp nrf/samples/tfm/provisioning_image -d build_provisioning_image
    west flash --erase -d build_provisioning_image

Build and flash the TF-M PSA template sample.
Do not flash with ``--erase`` as this will erase the PSA platform security parameters and they will be lost.

.. code-block:: console

    west build -b nrf5340dk/nrf5340/cpuapp/ns nrf/samples/tfm/tfm_psa_template
    west flash

Testing
=======

After programming the sample, the following output is displayed in the console:

.. code-block:: console

    *** Booting nRF Connect SDK v2.6.99-27b5931e8742 ***
    *** Using Zephyr OS v3.6.99-b5cf30402984 ***
    Attempting to boot slot 0.
    Attempting to boot from address 0x8200.
    Verifying signature against key 0.
    Hash: 0x43...7f
    Firmware signature verified.
    Firmware version 1
    *** Booting My Application v2.1.0-dev-0b5810de95eb ***
    *** Using nRF Connect SDK v2.6.99-27b5931e8742 ***
    *** Using Zephyr OS v3.6.99-b5cf30402984 ***
    *** Booting nRF Connect SDK v2.6.99-27b5931e8742 ***
    *** Using Zephyr OS v3.6.99-b5cf30402984 ***
    build time: May 13 2024 17:13:28

    FW info S0:
    Magic: 0x281ee6de8fcebb4c00003502
    Total Size: 60
    Size: 0x000081c8
    Version: 1
    Address: 0x00008200
    Boot address: 0x00008200
    Valid: 0x9102ffff (CONFIG_FW_INFO_VALID_VAL=0x9102ffff)

    FW info S1:
    Magic: 0x281ee6de8fcebb4c00003502
    Total Size: 60
    Size: 0x000081c8
    Version: 1
    Address: 0x00014200
    Boot address: 0x00014200
    Valid: 0x9102ffff (CONFIG_FW_INFO_VALID_VAL=0x9102ffff)

    Active slot: S0

    Requesting initial attestation token with 64 byte challenge.
    Received initial attestation token of 303 bytes.

    0  1  2  3  4  5  6  7   8  9  A  B  C  D  E  F
    D2 84 43 A1 01 26 A0 58  E4 AA 3A 00 01 24 FF 58  |  ..C..&.X..:..$.X
    40 00 11 22 33 44 55 66  77 88 99 AA BB CC DD EE  |  @.."3DUfw.......
    FF 00 11 22 33 44 55 66  77 88 99 AA BB CC DD EE  |  ..."3DUfw.......
    FF 00 11 22 33 44 55 66  77 88 99 AA BB CC DD EE  |  ..."3DUfw.......
    FF 00 11 22 33 44 55 66  77 88 99 AA BB CC DD EE  |  ..."3DUfw.......
    FF 3A 00 01 24 FB 58 20  3B 32 D3 C6 4F E0 9E 44  |  .:..$.X ;2..O..D
    85 33 05 C0 7E 1B 14 39  C0 73 22 2E AE 63 68 8C  |  .3..~..9.s"..ch.
    26 66 52 D2 84 50 EF 56  3A 00 01 25 00 58 21 01  |  &fR..P.V:..%.X!.
    70 7D AE 14 A9 68 1F 77  99 6F F8 02 CF 69 C2 47  |  p}...h.w.o...i.G
    BF 3A 10 DD EA C3 74 6E  0B F7 45 CF 2A 25 A9 DA  |  .:....tn..E.*%..
    3A 00 01 24 FA 58 20 AA  AA AA AA AA AA AA AA BB  |  :..$.X .........
    BB BB BB BB BB BB BB CC  CC CC CC CC CC CC CC DD  |  ................
    DD DD DD DD DD DD DD 3A  00 01 24 F8 20 3A 00 01  |  .......:..$. :..
    24 F9 19 30 00 3A 00 01  24 FE 01 3A 00 01 24 F7  |  $..0.:..$..:..$.
    60 3A 00 01 25 01 60 3A  00 01 24 FC 60 58 40 00  |  `:..%.`:..$.`X@.
    E0 AB 97 6B C1 24 8D AF  C9 E1 1C 77 E4 5E 1D 8E  |  ...k.$.....w.^..
    0D 44 61 76 28 A5 0D A1  BE A3 2D B2 A0 35 77 0E  |  .Dav(.....-..5w.
    78 72 7D E6 BE A1 10 A2  DC 7C ED 87 76 C7 33 E4  |  xr}......|..v.3.
    4E 8C 39 3D AA EC 40 EB  31 4B D5 68 80 53 77     |  N.9=..@.1K.h.Sw

Firmware update
***************

This sample supports firmware update of both the application and TF-M, and the second stage bootloader.

The firmware update process requires signature verification keys in order to sign the images used in the process.
The nRF Secure Immutable bootloader and MCUboot will use signing keys that should not be used in production.
For signing and verifying images, use ECDSA with secp256r1-sha256, which is supported by the |NCS| cryptographic libraries :ref:`nrf_oberon_readme` and :ref:`crypto_api_nrf_cc310_bl`.

Below is an example of how to generate and use the keys.

Generate security keys if needed:

.. code-block:: console

    mkdir _keys
    python3 bootloader/mcuboot/scripts/imgtool.py keygen -t ecdsa-p256 -k ~/ncs/_keys/mcuboot_priv.pem
    python3 bootloader/mcuboot/scripts/imgtool.py keygen -t ecdsa-p256 -k ~/ncs/_keys/nsib_priv.pem

Update the :file:`sysbuild.conf` file to set the private signing keys for MCUboot and NSIB:

.. code-block:: console

    SB_CONFIG_BOOT_SIGNATURE_KEY_FILE="/home/user/ncs/_keys/mcuboot_priv.pem"
    SB_CONFIG_SECURE_BOOT_SIGNING_KEY_FILE="/home/user/ncs/_keys/nsib_priv.pem"

See :ref:`ug_fw_update_keys` for more information on how to generate and use keys for a project.

The bootloader and the application can be updated using the :file:`mcumgr` command-line tool.
See :zephyr:code-sample:`smp-svr` for installation and usage instructions.

Application and TF-M firmware update
====================================

Use firmware update to update the application and TF-M firmware.
For the image to be updatable, the firmware image version :kconfig:option:`CONFIG_MCUBOOT_IMGTOOL_SIGN_VERSION` has to be updated to a higher version.
See :ref:`ug_fw_update_image_versions_mcuboot_downgrade` for information on downgrade protection in MCUboot.

To upload a new application image, build an application with an updated image version.

.. code-block:: console

    west build -b nrf5340dk/nrf5340/cpuapp/ns nrf/samples/tfm/tfm_psa_template -d build_update \
    -DCONFIG_MCUBOOT_IMGTOOL_SIGN_VERSION=\"1.2.3\"

Then upload the new application image to the device.

.. code-block:: console

    mcumgr --conntype serial --connstring dev=/dev/ttyACM1,baud=115200,mtu=512 image list
    mcumgr --conntype serial --connstring dev=/dev/ttyACM1,baud=115200,mtu=512 image upload \
    build_update/tfm_psa_template/zephyr/zephyr.signed.bin

Once the new application image is uploaded, the hash of the image is shown in the image list.
Flag the image to be tested on next reboot using its hash.

.. code-block:: console

    mcumgr --conntype serial --connstring dev=/dev/ttyACM1,baud=115200,mtu=512 image list
    mcumgr --conntype serial --connstring dev=/dev/ttyACM1,baud=115200,mtu=512 image test <hash>

Trigger the application update by initiating a reset.
The verification of the image will happen during the update process.

.. code-block:: console

    mcumgr --conntype serial --connstring dev=/dev/ttyACM1,baud=115200,mtu=512 reset

Bootloader firmware update
==========================

To upload a new bootloader image, build a bootloader targeting the correct bootloader slot with an updated firmware image version.

.. code-block:: console

    west build -b nrf5340dk/nrf5340/cpuapp/ns nrf/samples/tfm/tfm_psa_template \
    -Dmcuboot_CONFIG_FW_INFO_FIRMWARE_VERSION=2

List the current firmware images and upload a bootloader image that targets the non-active bootloader slot.

.. code-block:: console

    mcumgr --conntype serial --connstring dev=/dev/ttyACM1,baud=115200,mtu=512 image list
    mcumgr --conntype serial --connstring dev=/dev/ttyACM1,baud=115200,mtu=512 image upload \
    build/signed_by_mcuboot_and_b0_s1_image.bin

Once the new bootloader image is uploaded, the hash of the image is shown in the image list.
Flag the image to be tested on next reboot using its hash.

.. code-block:: console

    mcumgr --conntype serial --connstring dev=/dev/ttyACM1,baud=115200,mtu=512 image list
    mcumgr --conntype serial --connstring dev=/dev/ttyACM1,baud=115200,mtu=512 image test <hash>

Trigger the bootloader update by initiating a reset.
The verification of the image will happen during the update process.

.. code-block:: console

    mcumgr --conntype serial --connstring dev=/dev/ttyACM1,baud=115200,mtu=512 reset

Dependencies
*************

* This sample uses the TF-M module found in the :file:`modules/tee/tfm/` folder of the |NCS|.
* This sample uses the :ref:`lib_tfm_ioctl_api` library.
