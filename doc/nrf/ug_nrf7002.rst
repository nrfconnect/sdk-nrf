.. _nrf7002dk_nrf5340:

Getting started with nRF9160 DK
###############################

The nRF7002 DK (PCA10143) is a single-board development kit for evaluation and development on the Nordic nRF7002 Wi-Fi companion chip to Nordic nRF5340 System-on-Chip (SoC) host processor.

Overview
********

The nRF7002 is a IEEE 802.11ax (Wi-Fi 6) complaint SoC which implements the Wi-Fi physical layer and MAC layer protocols.
It implements the Wi-Fi driver software on the nRF5340 host processor communicating via QSPI bus.

The nRF5340 host is a dual-core SoC based on the Arm® Cortex®-M33 architecture.
It has the following features:

* a full-featured Arm Cortex-M33F core with DSP instructions, FPU, and Armv8-M Security Extension, running at up to 128 MHz, referred to as the *application core*.
* a secondary Arm Cortex-M33 core, with a reduced feature set, running at a fixed 64 MHz, referred to as the *network core*.

The ``nrf7002dk_nrf5340_cpuapp`` build target provides support for the application core on the nRF5340 SoC.
The ``nrf7002dk_nrf5340_cpunet`` build target provides support for the network core on the nRF5340 SoC.

The nRF5340 SoC provides support for the following devices:

* :abbr:`ADC (Analog to Digital Converter)`
* CLOCK
* FLASH
* :abbr:`GPIO (General Purpose Input Output)`
* :abbr:`IDAU (Implementation Defined Attribution Unit)`
* :abbr:`I2C (Inter-Integrated Circuit)`
* :abbr:`MPU (Memory Protection Unit)`
* :abbr:`NVIC (Nested Vectored Interrupt Controller)`
* :abbr:`PWM (Pulse Width Modulation)`
* Wi-Fi (Supporting up to 802.11ax in 2.4GHz and 5GHz bands)
* RADIO (Bluetooth Low Energy and 802.15.4)
* :abbr:`RTC (nRF RTC System Clock)`
* Segger RTT (RTT Console)
* :abbr:`SPI (Serial Peripheral Interface)`
* :abbr:`UARTE (Universal asynchronous receiver-transmitter)`
* :abbr:`USB (Universal Serial Bus)`
* :abbr:`WDT (Watchdog Timer)`

.. figure:: images/nrf7002dk.jpg
     :width: 711px
     :align: center
     :alt: nRF7002 DK

     nRF7002 DK (Credit: Nordic Semiconductor)

Visit the `Nordic Semiconductor WiFi products`_ page for more information on the development kit.
Visit the `Infocenter <Nordic Semiconductor Infocenter>`_ for the processor's information and the datasheet.

Hardware
********

nRF7002 DK has two external oscillators.
The frequency of the slow clock is 32.768 kHz.
The frequency of the main clock is 32 MHz.

Supported Features
==================

The ``nrf7002dk_nrf5340_cpuapp`` board configuration supports the following hardware features:

+-----------+------------+----------------------+
| Interface | Controller | Driver/Component     |
+===========+============+======================+
| ADC       | on-chip    | adc                  |
+-----------+------------+----------------------+
| CLOCK     | on-chip    | clock_control        |
+-----------+------------+----------------------+
| FLASH     | on-chip    | flash                |
+-----------+------------+----------------------+
| GPIO      | on-chip    | gpio                 |
+-----------+------------+----------------------+
| I2C(M)    | on-chip    | i2c                  |
+-----------+------------+----------------------+
| MPU       | on-chip    | arch/arm             |
+-----------+------------+----------------------+
| NVIC      | on-chip    | arch/arm             |
+-----------+------------+----------------------+
| PWM       | on-chip    | pwm                  |
+-----------+------------+----------------------+
| RTC       | on-chip    | system clock         |
+-----------+------------+----------------------+
| RTT       | Segger     | console              |
+-----------+------------+----------------------+
| RADIO     | nrf7002    | Wi-Fi 6 (802.11ax)   |
+-----------+------------+----------------------+
| QSPI      | on-chip    | qspi                 |
+-----------+------------+----------------------+
| SPI(M/S)  | on-chip    | spi                  |
+-----------+------------+----------------------+
| SPU       | on-chip    | system protection    |
+-----------+------------+----------------------+
| UARTE     | on-chip    | serial               |
+-----------+------------+----------------------+
| USB       | on-chip    | usb                  |
+-----------+------------+----------------------+
| WDT       | on-chip    | watchdog             |
+-----------+------------+----------------------+

The ``nrf7002dk_nrf5340_cpunet`` board configuration supports the following hardware features:

+-----------+------------+----------------------+
| Interface | Controller | Driver/Component     |
+===========+============+======================+
| CLOCK     | on-chip    | clock_control        |
+-----------+------------+----------------------+
| FLASH     | on-chip    | flash                |
+-----------+------------+----------------------+
| GPIO      | on-chip    | gpio                 |
+-----------+------------+----------------------+
| I2C(M)    | on-chip    | i2c                  |
+-----------+------------+----------------------+
| MPU       | on-chip    | arch/arm             |
+-----------+------------+----------------------+
| NVIC      | on-chip    | arch/arm             |
+-----------+------------+----------------------+
| RADIO     | on-chip    | Bluetooth,           |
|           |            | ieee802154           |
+-----------+------------+----------------------+
| RTC       | on-chip    | system clock         |
+-----------+------------+----------------------+
| RTT       | Segger     | console              |
+-----------+------------+----------------------+
| QSPI      | on-chip    | qspi                 |
+-----------+------------+----------------------+
| SPI(M/S)  | on-chip    | spi                  |
+-----------+------------+----------------------+
| UARTE     | on-chip    | serial               |
+-----------+------------+----------------------+
| WDT       | on-chip    | watchdog             |
+-----------+------------+----------------------+

Other hardware features are not supported by the |NCS| kernel.

Connections and IOs
===================

The following are the connections and IOs supported by the development kit.

LED
---

* LED1 (green) = P1.06
* LED2 (green) = P1.07

Push buttons
------------

* BUTTON1 = SW1 = P1.08
* BUTTON2 = SW2 = P1.09
* BOOT = SW5 = boot/reset

Wi-Fi Control
-------------

* BUCKEN = P0.12
* IOVDD CONTROL = P0.31
* HOST IRQ = P0.23
* COEX_REQ = P0.28
* COEX_STATUS0 = P0.30
* COEX_STATUS1 = P0.29
* COEX_GRANT = P0.24

Security components
===================

- Implementation Defined Attribution Unit (`IDAU`_) on the application core.
  The IDAU is implemented with the System Protection Unit and is used to define secure and non-secure memory maps.
  By default, all of the memory space (Flash, SRAM, and peripheral address space) is defined to be secure accessible only.
- Secure boot.

Programming and Debugging
*************************

The nRF5340 application core supports the Armv8-M Security Extension.
Applications built for the ``nrf7002dk_nrf5340_cpuapp`` board boot by default in the Secure state.

The nRF5340 network core does not support the Armv8-M Security Extension.
nRF5340 IDAU can configure bus accesses by the nRF5340 network core to have Secure attribute set.
This allows to build and run secure-only applications on the nRF5340 SoC.

Building Secure/Non-Secure |NCS| applications with Arm TrustZone
=================================================================

Applications on the nRF5340 can contain a Secure and a Non-Secure firmware image for the application core.
You can build the secure image using either |NCS| or `Trusted Firmware M`_ (TF-M).
You must always build the non-secure firmware images using |NCS|.

.. note::
   By default, the Secure image for nRF5340 application core is built using TF-M.

Building the Secure firmware with TF-M
--------------------------------------

To build the Secure firmware image using TF-M and the Non-Secure firmware image using |NCS|, follow these steps:

1. Build the Non-Secure |NCS| application for the application core using ``-DBOARD=nrf7002dk_nrf5340_cpuapp_ns``.
   To invoke the building of TF-M, the |NCS| build system requires the Kconfig option ``BUILD_WITH_TFM`` to be enabled, which is done by default when building |NCS| as a Non-Secure application.

   The |NCS| build system performs the following steps automatically:

      a. Build the Non-Secure firmware image as a regular |NCS| application.
      #. Build a TF-M (secure) firmware image.
      #. Merge the output image binaries.
      #. Optionally build a bootloader image (MCUboot).

   .. note::
      Depending on the TF-M configuration, an application DTS overlay can be required, to adjust the Non-Secure image flash memory partition and SRAM starting address and sizes.

2. Build the application firmware for the network core using ``-DBOARD=nrf7002dk_nrf5340_cpunet``.


Building the Secure firmware using |NCS|
-----------------------------------------

To build the Secure and the Non-Secure firmware images using |NCS|, follow these steps:

1. Build the Secure |NCS| application for the application core using ``-DBOARD=nrf7002dk_nrf5340_cpuapp``.
   Also set ``CONFIG_TRUSTED_EXECUTION_SECURE=y`` and ``CONFIG_BUILD_WITH_TFM=n`` in the application project configuration file.
2. Build the Non-Secure |NCS| application for the application core using ``-DBOARD=nrf7002dk_nrf5340_cpuapp_ns``.
3. Merge the two binaries.
4. Build the application firmware for the network core using ``-DBOARD=nrf7002dk_nrf5340_cpunet``.


When building a Secure and a Non-Secure application for the nRF5340 application core, the Secure application has to set the IDAU (SPU) configuration to allow Non-Secure access to all CPU resources utilized by the Non-Secure application firmware.
SPU configuration shall take place before jumping to the Non-Secure application.

Building a Secure only application
==================================

Build the |NCS| app following the standard process (see :ref:`build_an_application` and :ref:`application_run`), using ``-DBOARD=nrf7002dk_nrf5340_cpuapp`` for the firmware running on the nRF5340 application core and ``-DBOARD=nrf7002dk_nrf5340_cpunet`` for the firmware running on the nRF5340 network core.

Programming the firmware to the DK
==================================

Follow the instructions in the :ref:`nordic_segger` page to install and configure all the necessary software.
Further information can be found in :ref:`nordic_segger_flashing`.
Then you can build and flash applications as usual (:ref:`build_an_application` and :ref:`application_run` for more details).

.. note::

   The nRF5340 has a flash read-back protection feature.
   When flash read-back protection is active, you will need to recover the chip before reflashing.
   If you are flashing with :ref:`west <west-build-flash-debug>`, run this command for more details on the related ``--recover`` option:

   .. code-block:: console

      west flash -H -r nrfjprog --skip-rebuild

.. note::
   Flashing and debugging applications on the nRF7002 DK require upgrading the nRF Command Line Tools to version 10.12.0.
   Further information on how to install the nRF Command Line Tools can be found in :ref:`nordic_segger_flashing`.

Follow the steps in this example to run the :ref:`hello_world` application on the nRF5340 application core:

1. Run your favorite terminal program to listen for output.

   .. code-block:: console

      $ minicom -D <tty_device> -b 115200

2. Replace :code:`<tty_device>` with the port where the board nRF7002 DK can be found.
   For example, under Linux, :code:`/dev/ttyACM0`.

3. Build and flash the application.

   .. code-block:: console

      zephyr-app: samples/hello_world
      board: nrf7002dk_nrf5340_cpuapp
      goals: build flash

Debugging
=========

Refer to the :ref:`nordic_segger` page to learn about debugging Nordic Semiconductor development kits with a Segger IC.


Testing the LEDs and buttons in the nRF7002 DK
**********************************************

There are 2 samples that allow you to test if the buttons (switches) and LEDs on the board are working properly with |NCS|:

* :ref:`blinky-sample`
* :ref:`button-sample`

You can build and flash the examples to make sure |NCS| is running correctly on your board.
The button and LED definitions can be found in :file:`boards/arm/nrf7002dk_nrf5340/nrf5340_cpuapp_common.dts`.
