.. _bootloader:

Immutable bootloader
####################

The bootloader sample implements an immutable first stage bootloader that has the capability to verify and boot a second stage bootloader.
If the second stage bootloader is upgradable, it can reside in one of two slots.
In this case, the sample chooses the second stage bootloader with the highest version number.

See :ref:`ug_bootloader` for more information about the full bootloader chain.
For information about creating a second stage bootloader, see :doc:`mcuboot:index`.

Overview
********

The bootloader sample provides a simple root of trust (RoT) as well as support for an upgradable second stage bootloader.

This is accomplished by the following steps:

1. Lock the flash.
     To enable the RoT, the bootloader sample locks the flash that contains the sample bootloader and its configuration.
     Locking is done using the hardware that is available on the given architecture.
     For details on locking, see the :ref:`fprotect_readme` driver.

#. Select the next stage in the boot chain.
     The next stage in the boot chain can be another bootloader or the application.
     When the bootloader sample is enabled and MCUboot is used as second stage bootloader, there are two slots in which the second stage bootloader can reside.
     The second stage bootloader in each slot has a version number associated with it, and the bootloader sample selects the second stage bootloader that has the highest version number.

#. Verify the next stage in the boot chain.
     After selecting the image to be booted next, the bootloader sample verifies its validity using one of the provisioned public keys hashes.
     The image for the next boot stage has a set of metadata associated with it.
     This metadata contains the full public key corresponding to the private key that was used to sign the firmware.
     The bootloader sample checks the public key against a set of provisioned keys.
     Note that to save space, only the hashes of the provisioned keys are stored, and only the hashes of the keys are compared.
     If the public key in the metadata matches one of the valid provisioned public key hashes, the image is considered valid.
     All public key hashes at lower indices than the matching hash are permanently invalidated at this point, which means that images can no longer be validated with those public keys.
     For example, if an image is successfully validated with the public key at index 2, the public keys 0 and 1 are invalidated.
     This mechanism can be used to decommission broken keys.
     If the public key does not match any of the still valid provisioned hashes, validation fails.

#. Boot the next stage in the boot chain.
    After verifying the next boot stage, the bootloader sample uninitializes all peripherals that it used and boots the next boot stage.

#. Share the cryptographic library over EXT_API.
     The bootloader shares some of its functionality through an external API (EXT_API, see :ref:`doc_fw_info_ext_api`).
     For more information, see :file:`bl_crypto.h`.


Flash layout
============

The flash layout is defined by :file:`samples/bootloader/pm.yml`.

The bootloader sample defines four main areas:

1. **B0** - Contains the bootloader sample.
#. **Provision** - Stores the provisioned data.
#. **S0** - One of two potential storage areas for the second stage bootloader.
#. **S1** - One of two potential storage areas for the second stage bootloader.

Provisioning
============

The public key hashes are not compiled in with the source code of the bootloader sample.
Instead, they must be stored in a separate memory region through a process called *provisioning*.

By default, the bootloader sample will automatically generate and provision public key hashes directly into the bootloader HEX file, based on the specified private key and additional public keys.
Alternatively, to facilitate the manufacturing process of a device with the bootloader sample, it is possible to decouple this process and program the sample HEX file and the HEX file containing the public key hashes separately.
If you choose to do so, use the Python scripts in ``scripts\bootloader`` to create and provision the keys manually.

   .. note::
      On nRF9160, the provisioning data is held in the OTP region in UICR.
      Because of this, you must erase the UICR before programming the bootloader.
      On nRF9160, the UICR can only be erased by erasing the whole chip.
      To do so on the command line, call ``west flash`` with the ``--erase`` option.
      This will erase the whole chip before programming the new image.
      In |SES|, choose :guilabel:`Target` > :guilabel:`Connect J-Link` and then :guilabel:`Target` > :guilabel:`Erase All` to erase the whole chip.


Requirements
************

* One of the following development boards:

  * |nRF9160DK|
  * |nRF52840DK|
  * |nRF52DK|
  * |nRF51DK|

.. _bootloader_build_and_run:

Building and running
********************

The source code of the sample can be found under :file:`samples/bootloader/` in the |NCS| folder structure.

The most common use case for the bootloader sample is to be included as a child image in a multi-image build, rather than being built stand-alone.
Complete the following steps to add the bootloader sample as child image to your application:

1. Create a private key in PEM format.
   To do so, run the following command, which stores your private key in a file name ``priv.pem`` in the current folder::

       openssl ecparam -name prime256v1 -genkey -noout -out priv.pem

   OpenSSL is installed with GIT, so it should be available in your GIT bash.
   See `openSSL`_ for more information.

   .. note::
      This step is optional for testing the bootloader chain.
      If you do not provide your own keys, debug keys are created automatically.
      However, you should never go into production with an application that is not protected by secure keys.

#. Run ``menuconfig`` on your application to enable Secure Boot:

   a. Select **Project** > **Configure nRF Connect SDK project**.
   #. Go to **Modules** > **Nordic nRF Connect** and select **Use Secure Bootloader** to enable :option:`CONFIG_SECURE_BOOT`.
   #. Under **Private key PEM file** (:option:`CONFIG_SB_SIGNING_KEY_FILE`), enter the path to the private key that you created.
      If you choose to run the sample with default debug keys, you can skip this step.

      There are additional configuration options that you can modify, but it is not recommended to do so.
      The default settings are suitable for most use cases.

      .. note::
         If you need more flexibility with signing, or if you do not want the build system to handle your private key, choose :option:`CONFIG_SB_SIGNING_CUSTOM`.
         This option allows you to define the signing command.
         In this case, you must also specify :option:`CONFIG_SB_SIGNING_COMMAND` and :option:`CONFIG_SB_SIGNING_PUBLIC_KEY`.

   #. Click **Configure**.

#. Select **Build** > **Build Solution** to compile your application.
   The build process creates two images, one for the bootloader and one for the application, and merges them together.
#.  Select **Build** > **Build and Run** to program the resulting image to your device.


Testing
=======

To test the bootloader sample, add it to any other sample and build and program that sample it as described above.
Then test it by performing the following steps:

#. |connect_terminal|
#. Reset the board.
#. Observe that the kit prints the following information::

      Attempting to boot from address 0x8000.

      Verifying signature against key 0.

      Signature verified.

      Booting (0x8000).

Dependencies
************

This sample uses the following |NCS| libraries:

* :ref:`partition_manager`
* :ref:`doc_fw_info`
* :ref:`fprotect_readme`
* ``include/bl_validation.h``
* ``include/bl_crypto.h``
* ``subsys/bootloader/include/provision.h``

The sample also uses drivers from nrfx.
