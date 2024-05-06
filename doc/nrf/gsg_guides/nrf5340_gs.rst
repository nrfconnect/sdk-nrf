.. _ug_nrf5340_gs:

Getting started with nRF5340 DK
###############################

.. contents::
   :local:
   :depth: 2

This guide lets you evaluate the |NCS|'s support for nRF5340 :term:`Development Kit (DK)` without the need of installing the SDK.
It tells you how to install the :ref:`peripheral_uart` sample and perform a quick test of your DK.

If you want to go through an online training course to familiarize yourself with Bluetooth Low Energy and the development of Bluetooth LE applications, enroll in the `Bluetooth LE Fundamentals course`_ in the `Nordic Developer Academy`_.

.. _nrf5340_gs_requirements:

Minimum requirements
********************

Make sure you have all the required hardware and that your computer has one of the supported operating systems.

Hardware
========

* nRF5340 DK
* Micro-USB 2.0 cable

Software
========

On your computer, one of the following operating systems:

* Microsoft Windows
* macOS
* Ubuntu Linux

|Supported OS|

On your mobile device:

* Android
* iOS

.. _nrf5340_gs_installing_software:

Installing the required software
********************************

On your computer, install `nRF Connect for Desktop`_.
After installing and starting the application, install the Programmer app.

You must also install a terminal emulator, such as `nRF Connect Serial Terminal`_, the nRF Terminal (part of the `nRF Connect for Visual Studio Code`_ extension), or PuTTY.
nRF Connect Serial Terminal is the recommended method for :ref:`nrf5340_gs_connecting`.

On your mobile device, install the `nRF Connect for Mobile`_ application from the corresponding application store.

.. _nrf5340_gs_programming_sample:

Programming the sample
**********************

You must program and run a precompiled version of the :ref:`peripheral_uart` sample on your development kit to test the functions.
Download the precompiled version of the sample from the `nRF5340 DK Downloads`_ page.

After downloading the zip archive, extract it to a folder of your choice.
The archive contains the HEX file used to program the sample to your DK.

.. |DK| replace:: nRF5340 DK

.. include:: /gsg_guides/nrf52_gs.rst
   :start-after: program_dk_sample_start
   :end-before: program_dk_sample_end

After you have programmed the sample to the DK, you can connect to it using a terminal emulator and test the functions.

.. _nrf5340_gs_connecting:

Connecting to the sample
************************

.. include:: /gsg_guides/nrf52_gs.rst
   :start-after: uart_dk_connect_start
   :end-before: uart_dk_connect_end

Once the connection has been established, continue to :ref:`nrf5340_gs_testing`.

.. _nrf5340_gs_testing:

Testing the sample
******************

You can test the :ref:`peripheral_uart` sample on your |DK| using the `nRF Connect for Mobile`_ application.
The test requires that you have :ref:`connected to the sample <nrf5340_gs_connecting>` and have the connected terminal emulator open.

.. include:: /gsg_guides/nrf52_gs.rst
   :start-after: testing_dk_start
   :end-before: testing_dk_end

Next steps
**********

You have now completed getting started with the nRF5340 DK.
See the following links for where to go next:

* :ref:`installation` and :ref:`configuration_and_build` documentation to install the |NCS| and learn more about its development environment.
* :ref:`ug_nrf5340` documentation for more advanced topics related to the nRF5340 DK.
