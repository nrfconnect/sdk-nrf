.. _ug_nrf93m1_features:

Features of nRF93M1
###################

.. contents::
   :local:
   :depth: 2

The nRF93M1 is an LTE Cat 1 bis module.
It contains the cellular modem, the RF front end, the protocol stacks, and the nRF Cloud client, and it presents all of that to a host processor over serial interfaces.

The nRF93M1 DK is used for development with module variants described in :ref:`ug_nrf93m1_variants`.

Board hardware
**************

The nRF93M1 board contains the following:

* nRF54L15 host MCU, with an Arm Cortex-M33 application core and a RISC-V FLPR core
* nRF93M1 LTE Cat 1 bis module, connected to the host over UART
* nPM1300 PMIC with battery charger
* External SPI NOR flash
* USB-C for programming, debugging, and power

This page describes what the module provides, so you can decide what your host application still has to implement.

Module
******

The module owns the following:

* The LTE Cat 1 bis protocol stack, 3GPP Release 14 compliant.
* IPv4 and IPv6, TCP, and UDP.
* TLS and DTLS, including credential storage.
* CoAP, HTTP and HTTPS, and MQTT.
* A BSD-style socket API over AT commands, so a host without its own IP stack can still use TCP and
  UDP sockets.
* A file system, used for credentials and for firmware and observability payloads.
* SMS.
* The nRF Cloud client, covering provisioning, location, firmware updates over the air, and modem
  observability.
* SIM handling for up to two (e)UICC interfaces and the SoftSIM framework.
* Its own power state machine.

Host application
****************

Your host application owns the following:

* Application logic and sensor handling.
* The AT command interface, or a PPP network interface if you run the host IP stack.
* Any host-side security, such as protecting credentials that you pass to the module.
* Power management of the host itself.

Because the protocol stacks run in the module, a minimal host does not need an IP stack, a TLS library, or a cloud library.
This is the main footprint difference between an nRF93M1 design and an nRF91 Series design.

.. note::
   MQTT is supported from module firmware v1.5.x and higher on the nRF93M1 device.
   See the ``%MQTT*`` command in the `nRF93M1 AT Commands Reference Guide`_ document for more details.

.. important::
   The module boots at ``AT+CFUN=0`` for minimum functionality and does not attach to the network automatically.
   Your host owns the attached sequence.
   Change the boot level with ``AT%MODEMCFG="CfunInit",<fun>,<max_delay>`` if you need the module to attach without host intervention.

.. _ug_nrf93m1_getting_started:

Getting started
***************

To get started with the nRF93M1 DK, complete the following steps:

1. Follow the `Quick Start app`_ available from `nRF Connect for Desktop`_ to connect the DK to a network and to nRF Cloud using the preloaded SIM and the AT command interface.
   You do not need to build any firmware for this step.
#. Build and run the :ref:`nrf93m1dk_modem_bypass` sample to get direct AT command access to the module from your PC.
#. Build and run the :ref:`nrf93m1dk_ppp_shell` sample to bring up IP networking on the nRF54L15 host and test throughput.

.. _ug_nrf93m1_architecture:

Host and modem architecture
***************************

An nRF93M1 design always has two software domains.

Modem module
   The nRF93M1 runs Nordic-maintained modem firmware that owns the LTE protocol stack, the IP stack, the TLS stack, and the nRF Cloud client.
   You do not build or flash this firmware from the |NCS|.
   You configure it and drive it through AT commands.

Host processor
   The host runs your application.
   It sends AT commands, handles unsolicited result codes, and either uses the offloaded IP stack in the module or runs its own IP stack over PPP.

This distinction is important when you plan your application, because it determines where the networking functionality is implemented and how much flash and RAM the host application requires.
Three integration models are available.

.. list-table::
   :header-rows: 1

   * - Model
     - Host interface
     - Where the IP stack runs
     - Use when
   * - AT commands
     - UART
     - Module
     - The host is small, or you want the lowest host footprint.
   * - PPP over CMUX
     - UART
     - Host
     - You need Zephyr sockets, or existing host networking code.
   * - USB
     - USB RNDIS or CDC-ECM
     - Host OS
     - The host runs Linux, Windows, or another rich OS.

For the AT command model, no |NCS| networking libraries are required on the host.
For the PPP model, the host uses Zephyr modem cellular driver, which multiplexes a single UART into an AT command channel and a PPP data channel using CMUX.
The nRF93M1 is supported by that driver through the ``nordic,nrf93m1`` devicetree binding.

.. _ug_nrf93m1_features_radio:

LTE radio
*********

The radio implements 3GPP Release 14 LTE Category 1 bis on FDD and TDD bands, at LTE Power Class 3, up to 23 dBm.

Cat 1 bis uses a single receive antenna.
The module has one 50-ohm antenna pin, **ANT**, used for both transmit and receive across all supported bands.
This removes receive diversity routing from your layout while preserving Cat 1 peak data rates.

.. list-table:: Radio characteristics
   :header-rows: 1

   * - Parameter
     - Value
   * - 3GPP release
     - Release 14, field upgradable
   * - Power class
     - Class 3, up to 23 dBm
   * - Channel bandwidths
     - 1.4, 3, 5, 10, 15, and 20 MHz
   * - Peak downlink
     - 10 Mbps
   * - Peak uplink
     - 5 Mbps
   * - Antenna
     - Single 50-ohm pin, no receive diversity

.. _ug_nrf93m1_variants:

Module variants
***************

The nRF93M1 is available in two variants that differ only in band support and certification coverage:

.. list-table::
   :header-rows: 1

   * - Variant
     - Coverage
     - Bands and certifications
   * - nRF93M1-LABA
     - Multi-regional
     - LTE bands for Europe, Asia, the Middle East, and Africa, with corresponding certifications.
   * - nRF93M1-LACA
     - Global
     - Comprehensive set of global LTE bands and certifications for worldwide deployments.

Both variants are pin, size, and software compatible.
They share the same board target, interface, and AT commands, so moving between them requires no PCB redesign and no application changes.
Develop and qualify on nRF93M1-LABA, then place nRF93M1-LACA for markets that need the wider band set, with no PCB change.

See the `nRF93M1 Datasheet`_ for the supported bands of each variant.

You can restrict the enabled band set at runtime with the ``AT%BAND`` command, which takes a list of band numbers to enable.
The ``AT%BAND=?`` command returns the bands supported by the module and firmware you are running, allowing you to confirm the module variant.

.. _ug_nrf93m1_features_location:

Location
********

The module provides location without a GNSS receiver.
Two methods are available, and both use nRF Cloud to resolve raw measurements into a position:

Wi-Fi® scan
   The module scans 2.4 GHz Wi-Fi access points using DSSS and reports the results.
   nRF Cloud resolves the access point list to a position.

Cellular eCID
   The module reports serving and neighbor cell information.
   nRF Cloud resolves the cell data to a position.

Both are driven with AT commands.
Neither requires a GNSS antenna, which removes an antenna, a keep-out area, and a sky-view requirement from your mechanical design.

.. note::
   Accuracy from Wi-Fi and cellular location is lower than from GNSS and depends on the density of surrounding infrastructure.

.. _ug_nrf93m1_features_sim:

SIM support
***********

The SIMs that are used with the nRF93M1 device have the following features:

.. list-table::
   :header-rows: 1

   * - Feature
     - Support
   * - SIM interfaces
     - Two. Both support Class B (3.0 V) and Class C (1.8 V); the voltage is auto-selected per inserted UICC. If two external SIMs are used, both must be the same class.
   * - GSMA remote SIM provisioning
     - SGP.02 M2M eSIM, SGP.22 consumer eSIM, SGP.32 IoT eSIM
   * - SoftSIM
     - Supported

SGP.32 is the relevant profile for most IoT deployments, because it allows profile changes without a device-initiated user flow.
SoftSIM removes the physical SIM.

.. _ug_nrf93m1_features_security:

Security
********

The module supports secure boot and firmware rollback protection, so unauthorized or downgraded modem firmware does not execute.
Firmware updates are signed by Nordic and delivered either over the air through nRF Cloud or locally over serial.
See :ref:`ug_nrf93m1_updating_modem_fw`.

TLS and DTLS run in the module, and credentials are stored in the module.
TLS and DTLS run in the module, and credentials are stored in the module.
Your host sends credentials over the AT interface during provisioning and does not need to store them afterwards.

.. _ug_nrf93m1_samples:

Samples and applications
************************

The following samples use the nRF93M1 DK in the |NCS|:

* :ref:`nrf93m1dk_modem_bypass`, located under the :file:`samples/nrf93m1dk/modem_bypass` folder, configures the modem UART switch in bypass mode and forwards it to the USB CDC-ACM VCOM port, so a host PC can send AT commands straight to the module.
  It also handles modem power sequencing at startup.
  **Button 1** triggers ``POWER_KEY`` and **Button 2** triggers ``RESET``.
  Use the sample for bring-up, AT command exploration, and modem firmware work.
* :ref:`nrf93m1dk_ppp_shell`, located under the :file:`samples/nrf93m1dk/modem_bypass` folder, establishes a PPP connection between the nRF54L15 host and the module over a CMUX-multiplexed UART, and exposes the AT channel through the AT command shell.
  IPv4, IPv6, DNS, TCP, UDP, and POSIX sockets are enabled, and zperf is included for throughput testing.

Both samples are built for the ``nrf93m1dk/nrf54l15/cpuapp`` build target and run the UART at 115200 baud.
These samples do not require any firmware preparation on the module side because the host controls the module.
