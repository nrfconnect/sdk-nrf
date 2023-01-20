.. _ug_nrf5340_gs:

Getting started with nRF5340 DK
###############################

.. contents::
   :local:
   :depth: 2

This section gets you started with your nRF5340 :term:`Development Kit (DK)` using the |NCS|.
It tells you how to install the :ref:`peripheral_uart` sample and perform a quick test of your DK.

If you have already set up your nRF5340 DK and want to learn more, see the following documentation:

* :ref:`ug_nrf5340` for more advanced topics related to the nRF5340 DK if you are already familiar with the |NCS|.
* The :ref:`introductory documentation <getting_started>` for more information on the |NCS| and the development environment.

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

On your computer:

* Microsoft Windows 10
* macOS X, latest version
* Ubuntu Linux, latest Long Term Support (LTS) version

On your mobile device:

* Android
* iOS

.. _nrf5340_gs_installing_software:

Installing the required software
********************************

On your computer, install `nRF Connect for Desktop`_.
After installing and starting the application, install the Programmer app.

You must also install a terminal emulator, such as :ref:`PuTTY <putty>` or the nRF Terminal, which is part of the `nRF Connect for Visual Studio Code`_ extension.
The steps detailed in :ref:`nrf5340_gs_connecting` use PuTTY, but any terminal emulator will work.

On your mobile device, install the `nRF Connect for Mobile`_ application from the corresponding application store.

.. _nrf5340_gs_programming_sample:

Programming the sample
**********************

You must program and run a precompiled version of the :ref:`peripheral_uart` sample on your development kit to test the functions.
Download the precompiled version of the sample from the `nRF5340 DK Downloads`_ page.

After downloading the zip archive, extract it to a folder of your choice.
The archive contains the HEX file used to program the sample to your DK.

.. |DK| replace:: nRF5340 DK

.. include:: /working_with_nrf/nrf52/gs.rst
   :start-after: program_dk_sample_start
   :end-before: program_dk_sample_end

After you have programmed the sample to the DK, you can connect to it and test the functions.
If you connect to the sample now, you can go directly to Step 2 of :ref:`nrf5340_gs_connecting`.

.. _nrf5340_gs_connecting:

Connecting to the sample
************************

.. include:: /working_with_nrf/nrf52/gs.rst
   :start-after: uart_dk_connect_start
   :end-before: uart_dk_connect_end

The connection has now been established.
If you test the sample now, you can go directly to Step 2 of :ref:`nrf5340_gs_testing`.

.. _nrf5340_gs_testing:

Testing the sample
******************

You can test the :ref:`peripheral_uart` sample on your |DK| using the `nRF Connect for Mobile`_ application.
The test requires that you have :ref:`connected to the sample <nrf5340_gs_connecting>` and have the connected terminal emulator open.

.. include:: /working_with_nrf/nrf52/gs.rst
   :start-after: testing_dk_start
   :end-before: testing_dk_end

Next steps
**********

You have now completed getting started with the nRF5340 DK.
See the following links for where to go next:

* :ref:`ug_nrf5340` documentation for more advanced topics related to the nRF5340 DK.
* The :ref:`introductory documentation <getting_started>` for more information on the |NCS| and the development environment.
