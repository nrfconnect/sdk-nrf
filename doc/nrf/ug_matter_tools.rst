.. _ug_matter_tools:

Matter tools
############

.. contents::
   :local:
   :depth: 2

Use tools listed on this page to test :ref:`matter_samples` and develop Matter applications in the |NCS|.

GN tool
*******

To build and develop Matter applications, you need the `GN`_ meta-build system.
This system generates the Ninja files that the |NCS| uses.

If you are updating from the |NCS| version earlier than v1.5.0, see the :ref:`GN installation instructions <gs_installing_gn>`.

Matter controller tools
***********************

The Matter controller is a role within the :ref:`Matter development environment <ug_matter_configuring>`.
The controller device is used to pair and control the Matter accessory device remotely over a network, interacting with it using Bluetooth LE and the regular IPv6 communication.

The following figure shows the available Matter controllers in the |NCS|.

.. figure:: images/matter_protocols_controllers.svg
   :width: 600
   :alt: Controllers used by Matter

   Controllers used by Matter

Python controller for Linux
===========================

The Python Matter controller allows you to test Matter applications on a PC running Linux.
You can use it either on a separate device from Thread Border Router or on the same device as Thread Border Router, depending on which :ref:`development environment setup option <ug_matter_configuring>` you choose.

To use this controller type, you need to build it first using one of the following options:

* Use the prebuilt tool package from the `Matter nRF Connect releases`_ GitHub page.
  Make sure that the package is compatible with your |NCS| version.
* Build it manually from the source files available in the :file:`modules/lib/matter/src/controller/python` directory and using the building instructions from the :doc:`matter:python_chip_controller_building` page in the Matter documentation.

For instructions about how to test using the Python Controller for Linux, see the :doc:`matter:python_chip_controller_building` page in the Matter documentation.

Mobile controller for Android
=============================

The Mobile Controller for Android allows you to test Matter applications using an Android smartphone.
You can use it in the :ref:`development environment setup option <ug_matter_configuring>` where Thread Border Router and Matter controller are running on separate devices.

To use this controller type, you need to build it first using one of the following options:

* Use the prebuilt tool package from the `Matter nRF Connect releases`_ GitHub page.
  Make sure that the package is compatible with your |NCS| version.
* Build it manually from the source files available in the :file:`modules/lib/matter/src/android/CHIPTool` directory and using the building instructions from the :doc:`matter:android_building` page in the Matter documentation.

For instructions about how to test using the Mobile Controller for Android, see the :doc:`matter:nrfconnect_android_commissioning` page in the Matter documentation.

Thread tools
************

Because Matter is based on the Thread network, you can use the following :ref:`ug_thread_tools` when working with Matter in the |NCS|.

Thread Border Router
====================

.. include:: ug_thread_tools.rst
    :start-after: tbr_shortdesc_start
    :end-before: tbr_shortdesc_end

See the :ref:`ug_thread_tools_tbr` documentation for configuration instructions.

nRF Sniffer for 802.15.4
========================

.. include:: ug_thread_tools.rst
    :start-after: sniffer_shortdesc_start
    :end-before: sniffer_shortdesc_end

nRF Thread Topology Monitor
===========================

.. include:: ug_thread_tools.rst
    :start-after: ttm_shortdesc_start
    :end-before: ttm_shortdesc_end
