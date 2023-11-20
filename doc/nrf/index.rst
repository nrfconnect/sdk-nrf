.. _index:
.. _ncs_introduction:

Introduction
############

.. important::

   |NCS| v2.5.99-dev1 is a development tag that will be replaced by v2.6.0 in the future.

   This development tag includes the following major update:

   * Experimental support for the Matter 1.2.0 protocol, with updates including but not limited to:

     * Support for new device types such as refrigerators, room air conditioners, dishwashers, laundry washers, robotic vacuum cleaners, smoke CO alarms, air quality sensors, air purifiers, and fans.
     * Support for the Intermittently Connected Devices (ICD) Management cluster.
     * Support for the Product Appearance attribute in the Basic Information cluster.
     * Updated documentation to reflect changes in the Matter 1.2.0 protocol.

The |NCS| is a modern, unified software development kit for building low-power wireless applications based on the Nordic Semiconductor nRF52, nRF53, nRF70, and nRF91 Series wireless devices.
It supports :ref:`Microsoft Windows, Linux, and macOS <requirements>` for development.

The |NCS| has the following distinguishing features:

Based on Zephyr and open source
  The |NCS| integrates the :ref:`Zephyr™ real-time operating system (RTOS) <zephyr:introducing_zephyr>` and is compatible with most of Zephyr's applications and samples.
  Just like Zephyr, the |NCS| is open source and :ref:`based on proven open-source projects <dm_code_base>`.

Middleware and security
  The |NCS| includes middleware from Zephyr, such as MQTT or Trusted Firmware-M for security.
  In addition to that, the |NCS| adds extra libraries and drivers, such as CoAP, LwM2M, a secure bootloader (MCUboot), Mbed TLS, nRF 802.15.4 Radio Driver, nRF Security, nRF Profiler, nRF Remote procedure call libraries, and many more.

Pre-certified libraries
  The |NCS| provides pre-certified, optimized libraries, including for SoftDevice, Matter, and Thread.

Robust connectivity support
  The |NCS| supports a wide range of connectivity technologies.
  In addition to connectivity technologies :ref:`provided by Zephyr <zephyr:connectivity>`, such as Bluetooth® Low Energy, IPv6, TCP/IP, UDP, LoRa and LoRaWAN, the |NCS| supports ANT, Bluetooth mesh, Apple Find My, LTE-M/NB-IoT/GPS, Matter, Amazon Sidewalk, Thread, and Wi-Fi®, among others.

Scalable and extensible
  The |NCS| is out-of-tree ready and can be used for projects and applications of all sizes and levels of complexity.

Third-party integrations
  The |NCS| provides integrations with third-party and Nordic products within the SDK, such as AWS, nRF Cloud, Memfault, and more.

Varied reference designs
  The |NCS| comes with advanced hardware reference designs for different use cases, ranging from nRF Desktop for Human Interface Devices to nRF5340 Audio for audio devices based on Bluetooth LE Audio specifications.

.. toctree::
   :maxdepth: 2
   :caption: Contents
   :hidden:

   installation
   create_application
   config_and_build
   test_and_optimize
   device_guides
   security
   protocols
   applications
   samples
   drivers
   libraries/index
   scripts
   integrations
   releases_and_maturity
   documentation

..  templates/cheat_sheet
