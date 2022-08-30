.. _ug_matter_gs_testing_thread_separate_otbr:

Matter over Thread: Configuring Border Router and controller on separate devices
################################################################################

The recommended approach for Matter over Thread is to run the Thread Border Router and the Matter controller on separate devices.
With the Thread Border Router installed on Raspberry Pi, this approach provides support for most functionalities.
For example, it allows using a mobile controller by providing connectivity between a Wi-Fi network and a Thread network.

In this setup, Raspberry Pi runs the Thread Border Router, which provides communication between the Thread nodes and the Matter controller.
The controller can be installed on a PC or a mobile phone.
Both the Thread Border Router and the Matter controller must support IPv6 communication over backbone network, for example Wi-Fi or Ethernet.

.. _ug_matter_configuring_pc_chip_tool:

Configuring Thread Border Router and the CHIP Tool for Linux or macOS
*********************************************************************

In this setup, the Matter controller is installed on PC that is running either Linux or macOS, and a dedicated Wi-Fi Access Point and the CHIP Tool Matter controller are used.
This is the recommended setup.

.. figure:: images/matter_otbr_controller_separate_pc.svg
   :width: 600
   :alt: Setup with OpenThread Border Router and Matter controller on PC

Requirements
============

To use this setup, you need the following hardware:

* 1x PC with Ubuntu (20.04 or newer)
* 1x Raspberry Pi Model 3B+ or newer (along with an SD card with at least 8 GB of memory)
* 1x Wi-Fi Access Point supporting IPv6 (without the IPv6 Router Advertisement Guard enabled on the router)
* 1x nRF52840 DK or nRF52840 Dongle - for the Radio Co-Processor (RCP) device
* 1x compatible Nordic Semiconductor's DK - for the Matter accessory device (compatible and programmed with one of :ref:`matter_samples`)

Configuring the environment
===========================

To configure and use Thread Border Router and CHIP Tool for Linux or macOS on separate devices, complete the following steps:

1. Program the development kit for the Matter accessory device with one of available :ref:`matter_samples`.
#. Configure the Thread Border Router on a Raspberry Pi.
   See :ref:`ug_thread_tools_tbr` in the |NCS| documentation for details.
#. Configure the CHIP Tool for Linux or macOS by following the steps in the sections of the :doc:`matter:chip_tool_guide` in the Matter documentation:

   a. Build and run the CHIP Tool by completing the steps listed in "Building and running CHIP Tool".
   #. Prepare the environment for testing by completing the steps listed in "Using CHIP Tool for Matter device testing".

#. Depending on which Matter sample you programmed onto the development kit, go to this sample's documentation page and complete the steps from the Testing section.

.. _ug_matter_configuring_mobile:

Configuring Thread Border Router and CHIP Tool for Android
**********************************************************

In this setup, the Matter controller is installed on mobile, and a dedicated Wi-Fi Access Point and CHIP Tool for Android are used.

.. figure:: images/matter_otbr_controller_separate_mobile.svg
   :width: 600
   :alt: Setup with OpenThread Border Router and Matter controller on mobile

Requirements
============

To use this setup, you need the following hardware:

* 1x smartphone with Android 8+
* 1x Raspberry Pi Model 3B+ or newer (along with an SD card with at least 8 GB of memory)
* 1x Wi-Fi Access Point supporting IPv6 (without the IPv6 Router Advertisement Guard enabled on the router)
* 1x nRF52840 DK or nRF52840 Dongle - for the Radio Co-Processor (RCP) device
* 1x compatible Nordic Semiconductor's DK - for the Matter accessory device (compatible and programmed with one of :ref:`matter_samples`)

Configuring the environment
===========================

To configure and use Thread Border Router and CHIP Tool for Android on separate devices, complete the following steps:

1. Program the development kit for the Matter accessory device with one of available :ref:`matter_samples`.
#. Configure the Thread Border Router on a Raspberry Pi.
   See :ref:`ug_thread_tools_tbr` in the |NCS| documentation for details.
#. Configure the CHIP Tool for Android:

   a. Install the controller using one of the options described in :ref:`ug_matter_configuring_controller_mobile`.
   #. Complete the following steps from the :doc:`matter:nrfconnect_android_commissioning` user guide in the Matter documentation:

      * Building and installing CHIP Tool for Android - which prepares the controller for commissioning.
      * Preparing accessory device - which prepares your device programmed with the Matter sample for commissioning and provides you with the commissioning QR code.

        .. note::
            In the |NCS|, you can also use :ref:`NFC tag for Matter commissioning <ug_matter_configuring_optional_nfc>`.

      * Commissioning accessory device - which lets you commission your device into the network you created when configuring the Thread Border Router on Raspberry Pi.
      * Sending Matter commands - which checks the IPv6 connectivity.

#. Depending on which Matter sample you programmed onto the development kit, go to this sample's documentation page and complete the steps from the Testing section.
