.. _ug_bootloader:

Secure bootloader chain
#######################

The |NCS| provides a secure bootloader solution based on the chain of trust concept.

By using this bootloader chain, you can easily ensure that all code that is executed is authorized and your application is protected against running altered code.
If, for example, an attacker attempts to modify your application, or you have a bug in your code that overwrites parts of the application flash, the secure bootloader chain detects that the flash has been altered and your application does not start up.

Chain of trust
**************

A secure system depends on building and maintaining a chain of trust through all layers in the system.
Each step in this chain will guarantee that the next step can be trusted to have certain properties, because any illegal modification of a following step will be detected and the process halted.
The trustworthiness of each layer is guaranteed by the previous layer, all the way back to a property in the system referred to as a root of trust (RoT).

You can compare a chain of trust to the concept of a door.
You trust a door because you trust the lock because you trust the key because the key is in your pocket.
If you lose this key the chain of trust unravels and you no longer trust this door.

It is important to note that a key alone is not a RoT, although a RoT may include a key.
A RoT will consist of both hardware, software, and data components that must always behave in an expected manner, because any misbehavior cannot be detected.

.. _ug_bootloader_architecture:

Architecture
************

The secure bootloader chain consists of a sequence of images that are booted one after the other.
To establish a root of trust, the first image verifies the signature of the next image (which can be the application or another bootloader image).
If the next image is another bootloader image, that one must verify the image following it to maintain the chain of trust.
After all images in the bootloader chain have been verified successfully, the application starts.

In the current implementation, only the first stage in the chain, the immutable bootloader, is implemented.

The following image shows an abstract representation of the memory layout, assuming that there are two bootloader images (one immutable, one upgradable) and one application:

.. figure:: images/bootloader_memory_layout.svg
   :alt: Memory layout

   Memory layout

For detailed information about the memory layout, see the partition configuration in the DTS overlay file for the board that you are using.
This file is located in ``subsys\bootloader\dts``.

Immutable bootloader
====================

The first step in the chain of trust is an immutable first stage bootloader, in the following referred to as immutable bootloader.
The code abbreviates this bootloader as SB (for Secure Boot) or B0.

The immutable bootloader runs after every reset and establishes the root of trust by verifying the signature and metadata of the next image in the boot sequence.
If verification fails, the boot process stops.

This way, the immutable bootloader can guarantee that if the flash of the next image in the boot sequence (another bootloader or the application) has been tampered with in any way, that image will not start up.
So if an attacker attempts to take over the device by altering the flash, the device will not boot and not run the infected code.

The immutable bootloader cannot be updated or deleted from the device unless you erase the device.

There is no need to modify the immutable bootloader in any way before you program it. The default verification is recommended and suitable for all common user scenarios and includes the following checks:

Signature verification
   Verifies that the key used for signing the next image in the boot sequence matches one of the provided public keys.

Metadata verification
   Checks that the images are compatible.


Upgradable bootloader
=====================

Unlike the immutable bootloader, the upgradable bootloader can be updated through, for example, a Device Firmware Update (DFU).
This bootloader is therefore more flexible than the immutable bootloader.
It is protected by the root of trust in form of the immutable bootloader, and it must continue the chain of trust by verifying the next image in the boot sequence.

The upgradable bootloader should carry out the same signature and metadata verification as the immutable bootloader.
In addition, it can provide functionality for upgrading both itself and the following image in the boot sequence (in most cases, the application).

A default implementation of an upgradable bootloader is not available yet.


Adding a bootloader chain to your application
*********************************************

Complete the following steps to add a secure bootloader chain to your application:

1. Create a private key in PEM format.
   To do so, run the following command, which stores your private key in a file name ``priv.pem`` in the current folder::

       openssl ecparam -name prime256v1 -genkey -noout -out priv.pem

   OpenSSL is installed with GIT, so it should be available in your GIT bash.
   See `openSSL`_ for more information.

   .. note::
      This step is optional for testing the bootloader chain.
      If you do not provide your own keys, debug keys are created automatically.
      However, you should never go into production with an application that is not protected by secure keys.

#. Run ``menuconfig`` to enable Secure Boot:

   a. Select **Project** > **Configure nRF Connect SDK project**.
   #. Go to **Nordic nRF Connect** and select **Secure Boot** to enable :option:`CONFIG_SECURE_BOOT`.
   #. Under **Private key PEM file** (:option:`CONFIG_SB_SIGNING_KEY_FILE`), enter the path to the private key that you created.
      If you choose to run the sample with default debug keys, you can skip this step.

      There are additional configuration options that you can modify, but it is not recommended to do so.
      The default settings are suitable for most use cases.

   .. note::
      If you need more flexibility with signing, or you don't want the build system to handle your private key, choose CONFIG_SB_SIGNING_CUSTOM.
      When choosing CONFIG_SB_SIGNING_CUSTOM, you must also specify CONFIG_SB_SIGNING_COMMAND and CONFIG_SB_SIGNING_PUBLIC_KEY.

   #. Click **Configure**.

#. Select **Build** > **Build Solution** to compile your application.
   The build process creates two images, one for the bootloader and one for the application, and merges them together.
#.  Select **Build** > **Build and Run** to program the resulting image to your device.
