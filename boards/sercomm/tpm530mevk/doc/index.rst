.. zephyr:board:: tpm530mevk

Overview
********

The TPM530M EVK is a single-board development kit for evaluation and
development on the TPM530M module for LTE-M/NB-IoT with GNSS. The ``tpm530mevk/tpm530m``
board configuration provides support for the Nordic Semiconductor nRF9120 ARM
Cortex-M33F CPU with ARMv8-M Security Extension and the following devices:

* :abbr:`ADC (Analog to Digital Converter)`
* CLOCK
* FLASH
* :abbr:`GPIO (General Purpose Input Output)`
* :abbr:`I2C (Inter-Integrated Circuit)`
* :abbr:`MPU (Memory Protection Unit)`
* :abbr:`NVIC (Nested Vectored Interrupt Controller)`
* :abbr:`PWM (Pulse Width Modulation)`
* :abbr:`RTC (nRF RTC System Clock)`
* Segger RTT (RTT Console)
* :abbr:`SPI (Serial Peripheral Interface)`
* :abbr:`UARTE (Universal asynchronous receiver-transmitter with EasyDMA)`
* :abbr:`WDT (Watchdog Timer)`
* :abbr:`IDAU (Implementation Defined Attribution Unit)`

Hardware
********

TPM530M EVK has two external oscillators. The frequency of
the slow clock is 32.768 kHz. The frequency of the main clock
is 32 MHz.

Supported Features
==================

.. zephyr:board-supported-hw::

Connections and IOs
===================

LED
---

* D8 PWR (green) = power
* D9 MODULE (green) = P0.31
* D10 RF (green) = COEX2

Push buttons and Switches
-------------------------

* SW1 RESET = reset
* SW2 WAKE UP = P0.12

Security components
===================

- Implementation Defined Attribution Unit (`IDAU`_).  The IDAU is implemented
  with the System Protection Unit and is used to define secure and non-secure
  memory maps. By default, all of the memory space  (Flash, SRAM, and
  peripheral address space) is defined to be secure accessible only.
- Secure boot.

Programming and Debugging
*************************

.. zephyr:board-supported-runners::

``tpm530mevk/tpm530m`` supports the Armv8m Security Extension, and by default boots
in the Secure state.

Building Secure/Non-Secure Zephyr applications with Arm |reg| TrustZone |reg|
=============================================================================

The process requires the following steps:

1. Build the Secure Zephyr application using ``-DBOARD=tpm530mevk/tpm530m`` and
   ``CONFIG_TRUSTED_EXECUTION_SECURE=y`` in the application project configuration file.
2. Build the Non-Secure Zephyr application using ``-DBOARD=tpm530mevk/tpm530m/ns``.
3. Merge the two binaries together.

When building a Secure/Non-Secure application, the Secure application will
have to set the IDAU (SPU) configuration to allow Non-Secure access to all
CPU resources utilized by the Non-Secure application firmware. SPU
configuration shall take place before jumping to the Non-Secure application.

Building a Secure only application
==================================

Build the Zephyr app in the usual way (see :ref:`build_an_application`
and :ref:`application_run`), using ``-DBOARD=tpm530mevk/tpm530m``.

Flashing
========

Flashing the board requires an external J-Link debug probe. The probe is
attached to the JP17 SWD header.

Follow the instructions in the :ref:`nordic_segger` page to install
and configure all the necessary software. Further information can be
found in :ref:`nordic_segger_flashing`. Then build and flash
applications as usual (see :ref:`build_an_application` and
:ref:`application_run` for more details).

Here is an example for the :zephyr:code-sample:`hello_world` application.

First, run your favorite terminal program to listen for output.

.. code-block:: console

   $ minicom -D <tty_device> -b 115200

Replace :code:`<tty_device>` with the port where the TPM530M EVK
can be found. For example, under Linux, :code:`/dev/ttyACM0`.

Then build and flash the application in the usual way.

.. zephyr-app-commands::
   :zephyr-app: samples/hello_world
   :board: tpm530mevk/tpm530m
   :goals: build flash

Debugging
=========

The board does not have an on-board Segger IC, however, instructions from the
:ref:`nordic_segger` page also apply to this board, with the additional step
of connecting an external J-Link debug probe.

References
**********

.. target-notes::

.. _IDAU:
   https://developer.arm.com/docs/100690/latest/attribution-units-sau-and-idau
.. _Trusted Firmware M: https://www.trustedfirmware.org/projects/tf-m/
