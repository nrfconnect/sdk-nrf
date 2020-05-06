.. _nc_bootloader:

nRF5340 Network core bootloader
################################

This bootloader sample implements an immutable first stage bootloader that has the capability to download update the application firmware on the network core of the nRF5340 SoC.
Here, 'application' implies the application on the network core.
It also performs flash protection of both itself and the application.

Overview
********

The network core bootloader sample provides a scheme for transporting an already verified and authenticated firmware upgrade from the application core flash to the network core flash, as well as performing flash protection.

This is accomplished by the following steps:

1. Lock the flash of the bootloader.
     The bootloader sample locks the flash that contains the sample bootloader and its configuration.
     Locking is done using the ACL peripheral.
     For details on locking, see the :ref:`fprotect_readme` driver.


#. Check if there is a pending firmware upgrade of the application.
     Invoke the :ref:`subsys_pcd` subsys to inspect a memory region shared with the application core.
     If the application core has configured a pending update, copy this to the application partition.
     Once the copy is done, compare the SHA of the data in the application partition against the SHA provisioned in the firmware hex.

#. Lock the flash of the application.
     Lock the flash that contains the application.
     Locking is done using the ACL peripheral.
     For details on locking, see the :ref:`fprotect_readme` driver.

#. Boot the application in the network core.
     After possibly performing a firmware update, and enabling flash protection, the network core bootloader uninitializes all peripherals that it used and boots the application.

Requirements
************

  * |nRF5340DK|

.. _bootloader_build_and_run:

Building and running
********************

The source code of the sample can be found under :file:`samples/nrf5340/netboot/` in the |NCS| folder structure.

The most common use case for the bootloader sample is to be included as a child image in a multi-image build, rather than being built stand-alone.
This sample is included automatically if the application in the nrf5340 application core has the :option:`CONFIG_BOOTLOADER_MCUBOOT` option set.
Alternatively you can set the :option:`CONFIG_SECURE_BOOT` for a sample being built for the nrf5340 network core explicitly.

Testing
=======

To test the network core bootloader sample, enable :option:`CONFIG_BOOTLOADER_MCUBOOT` in the application core :file:`prj.conf`.
Then test it by performing the following steps:

#. |connect_terminal|
#. Reset the board.
#. Observe that the application starts as expected.

Dependencies
************

This sample uses the following |NCS| libraries:

* :ref:`doc_fw_info`
* :ref:`fprotect_readme`
* ``include/bl_validation.h``
* ``include/bl_crypto.h``
* ``subsys/bootloader/include/provision.h``

The sample also uses drivers from nrfx.
