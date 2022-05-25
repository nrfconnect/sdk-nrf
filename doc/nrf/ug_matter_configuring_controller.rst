.. _ug_matter_configuring_controller:

Configuring Matter controller
#############################

.. contents::
   :local:
   :depth: 2

.. matter_controller_start

The Matter controller is a role within the :ref:`Matter development environment <ug_matter_configuring>`.
The controller device is used to pair and control the Matter accessory device remotely over a network, interacting with it using Bluetooth LE and the regular IPv6 communication.

The following figure shows the available Matter controllers in the |NCS|.

.. figure:: images/matter_protocols_controllers.svg
   :width: 600
   :alt: Controllers used by Matter

   Controllers used by Matter

.. matter_controller_end

The following Matter controllers can be used for testing Matter applications based on the |NCS|:

* **Recommended:** CHIP Tool for Linux or macOS
* CHIP Tool for Android

These controller types are compatible with the |NCS| implementation of Matter.
In the Matter upstream repository, you can find information and resources for implementing `other controller setups`_ (for example, mobile Matter controller for iOS).

.. _ug_matter_configuring_controller_chip_tool:

CHIP Tool for Linux or macOS
****************************

CHIP Tool for Linux or macOS is the default implementation of the Matter controller role, recommended for the nRF Connect platform.
You can use it either on a separate device from Thread Border Router or on the same device as Thread Border Router, depending on which :ref:`development environment setup option <ug_matter_configuring>` you choose.
CHIP Tool for Linux or macOS is available for both amd64 and aarch64 architectures.
This implies that the tool can also be run on a Raspberry Pi with a 64-bit OS.

To use this controller type, choose one of the following options:

* For Linux only: use the prebuilt tool package from the `Matter nRF Connect releases`_ GitHub page.
  Make sure that the package is compatible with your |NCS| version.
* For both Linux and macOS: Build it manually from the source files available in the :file:`modules/lib/matter/examples/chip-tool` directory and using the building instructions from the :doc:`matter:chip_tool_guide` page in the Matter documentation.

For more information about how to test using the CHIP Tool, see the :doc:`matter:chip_tool_guide` page in the Matter documentation.

.. _ug_matter_configuring_controller_mobile:

CHIP Tool for Android
*********************

CHIP Tool for Android (also known as Android CHIPTool) is the recommended Mobile controller for mobile, which allows you to test Matter applications using an Android smartphone.
You can use it in the :ref:`development environment setup option <ug_matter_configuring>` where Thread Border Router and Matter controller are running on separate devices.

To use CHIP Tool for Android, choose one of the following options:

* Use the prebuilt tool package from the `Matter nRF Connect releases`_ GitHub page.
  Make sure that the package is compatible with your |NCS| version.
* Build it manually from the source files available in the :file:`modules/lib/matter/src/android/CHIPTool` directory and using the building instructions from the :doc:`matter:android_building` page in the Matter documentation.

For instructions about how to test using the CHIP Tool for Android, see the :doc:`matter:nrfconnect_android_commissioning` page in the Matter documentation.
