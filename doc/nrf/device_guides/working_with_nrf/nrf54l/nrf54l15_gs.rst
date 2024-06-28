.. _ug_nrf54l15_gs:

Getting started with nRF54L15 PDK
#################################

.. contents::
   :local:
   :depth: 2

This page will get you started with your nRF54L15 PDK using the |NCS|.
First, you need to install the required software and prepare the environment.
Once completed, test if your PDK is working correctly by running the preflashed :zephyr:code-sample:`blinky` sample.
The instructions will then guide you through how to configure, build, and program the :ref:`Hello World <zephyr:hello_world>` sample to the development kit, and how to read its logs.

.. _ug_nrf54l15_gs_requirements:

Minimum requirements
********************

Make sure you have all the required hardware and that your computer has one of the supported operating systems.

Hardware
========

* nRF54L15 PDK

   .. note::

      For commands, use the correct board target depending on your PDK version:

      * For the PDK revision v0.2.1, AB0-ES7 (Engineering A), use the ``nrf54l15pdk@0.2.1/nrf54l15/cpuapp`` board target.
      * For the PDK revisions v0.3.0 and v0.7.0 (Engineering A), use the ``nrf54l15pdk/nrf54l15/cpuapp`` board target.

* USB-C cable

Software
========

On your computer, one of the following operating systems:

* macOS
* Microsoft Windows
* Ubuntu Linux

|supported OS|
Make sure to install the v2.6.0 of :ref:`the nRF Connect SDK and the nRF Connect SDK toolchain <install_ncs>`.
You also need to install `Git`_ or `Git for Windows`_ (on Linux and Mac, or Windows, respectively).

Downloading the code
********************

Once you have installed the software, you need to update the code separately to be able to work with the nRF54L15 PDK.

Go to the :file:`ncs/v2.6.0/nrf` folder and run the following commands:

.. parsed-literal::
   :class: highlight

   git fetch
   git checkout v2.6.99-cs1
   west update

.. _ug_nrf54l15_gs_test_sample:

Testing with the Blinky sample
******************************

The nRF54L15 PDK comes preprogrammed with the Blinky sample.

Test if the PDK works correctly:

1. Connect the USB-C end of the USB-C cable to the **IMCU USB** port the nRF54L15 PDK.
#. Connect the other end of the USB-C cable to your PC.
#. Move the **POWER** switch to **On** to turn the nRF54L15 PDK on.

**LED1** will turn on and start to blink.

If something does not work as expected, contact Nordic Semiconductor support.

.. _ug_nrf54l15_gs_installing_software:

Additional software
********************

You will need a :ref:`terminal emulator <test_and_optimize>` to program the sample and read the logs.
The recommended emulators are `nRF Connect Serial Terminal`_ or the nRF Terminal (part of the `nRF Connect for Visual Studio Code`_ extension).

To make sure the device sees the environment, all the commands related to the |NCS|, building, and flashing need to be run from the nRF Connect terminal.

.. _ug_nrf54l15_gs_sample:

Programming the Hello World! sample
***********************************

The :ref:`zephyr:hello_world_user` Zephyr sample uses the ``nrf54l15pdk/nrf54l15/cpuapp`` board target.

To build and program the sample to the nRF54L15 PDK, complete the following steps:

1. Connect the nRF54L15 PDK to your computer using the IMCU USB port on the PDK.
#. Navigate to the :file:`zephyr/samples/hello_world` folder containing the sample.
#. Build the sample by running the following command:

   .. code-block:: console

      west build -b nrf54l15pdk/nrf54l15/cpuapp

#. Program the sample by running the standard |NCS| command:

   .. code-block:: console

      west flash

   If you have multiple Nordic Semiconductor devices, make sure that only the nRF54L15 PDK is connected.

   .. note::

      When programming the device, you might get an error mentioning the readback protection of the device.
      To get around the error, :ref:`program the device <programming_params>` with the ``--recover`` parameter.

.. _ug_nrf54l15_sample_reading_logs:

Reading the logs
****************

With the :ref:`zephyr:hello_world_user` sample programmed, the nRF54L15 PDK outputs logs over UART 30.

To read the logs from the :ref:`zephyr:hello_world_user` sample programmed to the nRF54L15 PDK, complete the following steps:

1. Connect to the PDK with a terminal emulator (for example, `nRF Connect Serial Terminal`_) using the :ref:`default serial port connection settings <test_and_optimize>`.
#. Press the **Reset** button on the PCB to reset the PDK.
#. Observe the console output:

   .. code-block:: console

    *** Booting Zephyr OS build 06af494ba663  ***
    Hello world! nrf54l15dk/nrf54l15/cpuapp

   .. note::
      If no output is shown when using the nRF Serial Terminal, select a different serial port in the terminal application.

Next steps
**********

You have now completed getting started with the nRF54L15 PDK.
See the following links for where to go next:

* :ref:`configuration_and_build` documentation to learn more about the |NCS| development environment.
