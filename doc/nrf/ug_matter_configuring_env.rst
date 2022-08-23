.. _ug_matter_configuring_env:

Configuring Matter development environment
##########################################

.. contents::
   :local:
   :depth: 2

Setting up Matter development environment is all about setting up the Matter network and commissioning your device into it.
In Matter, the commissioning procedure takes place over Bluetooth LE between a Matter accessory device and the Matter controller, where the controller has the commissioner role.
When the procedure has completed, the device should be equipped with all information needed to securely operate in the Matter network.

During the last part of the commissioning procedure (the provisioning operation), the Matter controller sends the Thread or Wi-Fi network credentials to the Matter accessory device.
As a result, the device can join the IPv6 network and communicate with other IPv6 devices in the network.

To start the commissioning procedure, the controller must get the onboarding information from the Matter accessory device.
The data payload, which includes the device discriminator and setup PIN code, is encoded within a QR code, printed to the UART console, and can be shared using an NFC tag.

Matter development environment setup depends on how you choose to run the Matter controller.
When using Matter over Thread, you can either run it on the same device as the Thread Border Router or run the Matter controller and the Thread Border Router on separate devices.

Matter over Thread: Running Thread Border Router and Matter controller on separate devices
******************************************************************************************

The recommended approach for Matter over Thread is to run the Thread Border Router and the Matter controller on separate devices.
With the Thread Border Router installed on Raspberry Pi, this approach provides support for most functionalities.
For example, it allows using a mobile controller by providing connectivity between a Wi-Fi network and a Thread network.

In this setup, Raspberry Pi runs the Thread Border Router, which provides communication between the Thread nodes and the Matter controller.
The controller can be installed on a PC or a mobile phone.
Both the Thread Border Router and the Matter controller must support IPv6 communication over backbone network, for example Wi-Fi or Ethernet.

.. _ug_matter_configuring_pc_chip_tool:

Configuring Thread Border Router and the CHIP Tool for Linux or macOS
=====================================================================

In this setup, the Matter controller is installed on PC that is running either Linux or macOS, and a dedicated Wi-Fi Access Point and the CHIP Tool Matter controller are used.
This is the recommended setup.

.. matter_env_ctrl_pc_start

.. figure:: images/matter_otbr_controller_separate_pc.svg
   :width: 600
   :alt: Setup with OpenThread Border Router and Matter controller on PC

.. matter_env_ctrl_pc_end

Requirements
------------

To use this setup, you need the following hardware:

* 1x PC with Ubuntu (20.04 or newer)
* 1x Raspberry Pi Model 3B+ or newer (along with an SD card with at least 8 GB of memory)
* 1x Wi-Fi Access Point supporting IPv6 (without the IPv6 Router Advertisement Guard enabled on the router)
* 1x nRF52840 DK or nRF52840 Dongle - for the Radio Co-Processor (RCP) device
* 1x compatible Nordic Semiconductor's DK - for the Matter accessory device (compatible and programmed with one of :ref:`matter_samples`)

Configuring the environment
---------------------------

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
==========================================================

In this setup, the Matter controller is installed on mobile, and a dedicated Wi-Fi Access Point and CHIP Tool for Android are used.

.. matter_env_ctrl_mobile_start

.. figure:: images/matter_otbr_controller_separate_mobile.svg
   :width: 600
   :alt: Setup with OpenThread Border Router and Matter controller on mobile

.. matter_env_ctrl_mobile_end

Requirements
------------

To use this setup, you need the following hardware:

* 1x smartphone with Android 8+
* 1x Raspberry Pi Model 3B+ or newer (along with an SD card with at least 8 GB of memory)
* 1x Wi-Fi Access Point supporting IPv6 (without the IPv6 Router Advertisement Guard enabled on the router)
* 1x nRF52840 DK or nRF52840 Dongle - for the Radio Co-Processor (RCP) device
* 1x compatible Nordic Semiconductor's DK - for the Matter accessory device (compatible and programmed with one of :ref:`matter_samples`)

Configuring the environment
---------------------------

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

Matter over Thread: Running Thread Border Router and Matter controller on the same device
*****************************************************************************************

If you only have one device, be it a PC with Linux or a Raspberry Pi, you can set up and test the Matter over Thread development environment with both the Thread Border Router and the Matter controller running on this one device.

In this setup, a PC or a Raspberry Pi runs the Thread Border Router and the CHIP Tool for Linux or macOS simultaneously.
To simplify enabling the Thread communication with the Matter accessory device, use a Docker container with the OpenThread Border Router image instead of installing the OpenThread Border Router natively.

.. matter_env_ctrl_one_start

.. figure:: images/matter_otbr_controller_same_device.svg
   :width: 600
   :alt: Setup with OpenThread Border Router and Matter controller on the same device

   Setup with OpenThread Border Router and Matter controller on the same device

.. matter_env_ctrl_one_end

You can use this setup with the CHIP Tool controller.

Requirements
============

To use this setup, you need the following hardware:

* One of the following:

  * 1x PC with Ubuntu (20.04 or newer)
  * 1x Raspberry Pi Model 3B+ or newer with Ubuntu (20.04 or newer) instead of Raspbian OS

* 1x Bluetooth LE dongle (can be embedded inside the PC, like it is on Raspberry Pi)
* 1x nRF52840 DK or nRF52840 Dongle - for the Radio Co-Processor (RCP) device
* 1x nRF52840 DK or nRF5340 DK - for the Matter accessory device (programmed with one of :ref:`matter_samples`)

Configuring the environment
===========================

To configure and use Thread Border Router and Matter controller on the same device, complete the following steps:

1. Program the Matter accessory device with one of available :ref:`matter_samples`.
#. Configure the Thread Border Router on a PC or on a Raspberry Pi, depending on what hardware you are using.
   For detailed steps, see the Running OTBR using Docker section on the :ref:`ug_thread_tools_tbr` page in the |NCS| documentation.
#. Configure the CHIP Tool controller.
   Complete the following actions by following the steps in the :doc:`matter:chip_tool_guide` user guide in the Matter documentation:

   * Build and run the CHIP Tool by completing the steps listed in "Building and running CHIP Tool".
   * Prepare the environment for testing by completing the steps listed in "Using CHIP Tool for Matter device testing".

#. Depending on which Matter sample you programmed onto the development kit, go to this sample's documentation page and complete the steps from the Testing section.

.. _ug_matter_configuring_pc_chip_tool_wifi:

Matter over Wi-Fi: Configuring CHIP Tool for Linux or macOS
***********************************************************

In this setup, the Matter controller is installed on PC that is running either Linux or macOS, and a dedicated Wi-Fi Access Point is used.
This is the recommended setup.

Requirements
============

To use this setup, you need the following hardware:

* 1x PC with Ubuntu (20.04 or newer)
* 1x Wi-Fi Access Point supporting IPv6 (without the IPv6 Router Advertisement Guard enabled on the router)
* 1x compatible Nordic Semiconductor's DK - for the Matter accessory device (compatible and programmed with one of :ref:`matter_samples`)

Configuring the environment
===========================

To configure and use CHIP Tool for Linux or macOS, complete the following steps:

1. Program the development kit for the Matter accessory device with one of available :ref:`matter_samples`.
#. Configure the CHIP Tool for Linux or macOS by following the steps in the sections of the :doc:`matter:chip_tool_guide` in the Matter documentation:

   a. Build and run the CHIP Tool by completing the steps listed in "Building and running CHIP Tool".
   #. Prepare the environment for testing by completing the steps listed in "Using CHIP Tool for Matter device testing".

#. Depending on which Matter sample you programmed onto the development kit, go to this sample's documentation page and complete the steps from the Testing section.

.. _ug_matter_configuring_mobile_wifi:

Matter over Wi-Fi: Configuring CHIP Tool for Android
****************************************************

In this setup, the Matter controller is installed on mobile, and a dedicated Wi-Fi Access Point is used.

Requirements
============

To use this setup, you need the following hardware:

* 1x smartphone with Android 8+
* 1x Wi-Fi Access Point supporting IPv6 (without the IPv6 Router Advertisement Guard enabled on the router)
* 1x compatible Nordic Semiconductor's DK - for the Matter accessory device (compatible and programmed with one of :ref:`matter_samples`)

Configuring the environment
===========================

To configure and use CHIP Tool for Android, complete the following steps:

1. Program the development kit for the Matter accessory device with one of available :ref:`matter_samples`.
#. Configure the CHIP Tool for Android:

   a. Install the controller using one of the options described in :ref:`ug_matter_configuring_controller_mobile`.
   #. Complete the following steps from the :doc:`matter:nrfconnect_android_commissioning` user guide in the Matter documentation:

      * Building and installing CHIP Tool for Android - which prepares the controller for commissioning.
      * Preparing accessory device - which prepares your device programmed with the Matter sample for commissioning and provides you with the commissioning QR code.

        .. note::
            In the |NCS|, you can also use :ref:`NFC tag for Matter commissioning <ug_matter_configuring_optional_nfc>`.

      * Commissioning accessory device - which lets you commission your device into the Wi-Fi network (Wi-Fi Access Point).
      * Sending Matter commands - which checks the IPv6 connectivity.

#. Depending on which Matter sample you programmed onto the development kit, go to this sample's documentation page and complete the steps from the Testing section.

