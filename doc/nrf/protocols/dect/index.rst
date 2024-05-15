.. _ug_dect:

DECT NR+
########

.. contents::
   :local:
   :depth: 2

DECT NR+ (NR+) is a new radio technology (ETSI TS 103 636) standard, considered the world's first non-cellular 5G.
It builds upon the legacy of DECT (Digital Enhanced Cordless Telecommunications), but offers significant advancements across various aspects, including radio frequency (RF) characteristics.

Differences between DECT and DECT NR+
*************************************

While DECT NR+ leverages the foundation laid by DECT, it is a completely new standard with substantial improvements.
The following table shows a comparison between  DECT and DECT NR+ across various technical aspects:

+---------------------+------------------------------------------------+-------------------------------------------------------------------------------------------+
| Feature             | DECT                                           | NR+                                                                                       |
+=====================+================================================+===========================================================================================+
| Technology          | Uses Time Division Multiple Access (TDMA)      | Employs :term:`Orthogonal Frequency Division Multiplexing (OFDM)`                         |
+---------------------+------------------------------------------------+-------------------------------------------------------------------------------------------+
| Focus               | Targets cordless phone applications            | Caters to a broader range, including massive Machine Type Communication (mMTC) and        |
|                     |                                                | Ultra-Reliable Low-Latency Communication (URLLC)                                          |
+---------------------+------------------------------------------------+-------------------------------------------------------------------------------------------+
| Latency             | Moderate, suitable for voice calls             | Significantly lower, ideal for real-time industrial automation and sensor data collection |
+---------------------+------------------------------------------------+-------------------------------------------------------------------------------------------+
| Data Rates          | Up to 1.3 Mbps                                 | Up to 3.4 Mbps, depending on the modulation scheme                                        |
+---------------------+------------------------------------------------+-------------------------------------------------------------------------------------------+
| Network Scalability | Limited to a few dozen devices                 | Supports massive deployments of sensors and devices                                       |
+---------------------+------------------------------------------------+-------------------------------------------------------------------------------------------+
| Security            | Basic security features                        | Stronger encryption (AES-128) and network hiding capabilities                             |
+---------------------+------------------------------------------------+-------------------------------------------------------------------------------------------+

Key Features of DECT NR+
************************

DECT NR+ presents a compelling option for low-power embedded devices, especially in applications requiring mMTC and URLLC.
Its focus on low power consumption, reliable data transfer, flexible network configurations, and strong security make it ideal for large sensor networks and real-time industrial applications.
Additionally, the utilization of the license-exempt 1.9 GHz band with specific channel bandwidth and modulation schemes contributes to its deployment advantages.

Building upon the improvements over DECT, NR+ offers the following key features:

License-Exempt Global Band
   DECT NR+ utilizes the globally available 1.9 GHz band, which is license-free in most regions.
   This reduces setup costs and complexity compared to LTE, which often requires licenses.

Supported Channels
   DECT NR+ operates within the 1.9 GHz band, using a specific channel bandwidth of 125 kHz.
   This allows for denser network deployments compared to wider-bandwidth technologies.

Modulation Schemes
   DECT NR+ supports various modulation schemes like gaussian frequency-shift keying (GFSK) and differential quadrature phase shift keying (DQPSK), which allow selection based on the desired balance between data rate, range, and robustness in different operating conditions.

Built-in Coexistence
   DECT NR+ features the built-in coexistence of multiple networks in the same location, meaning that multiple DECT NR+ networks can operate in the same area without interference, enabling flexible deployment in dense environments.

Flexible and Low-Latency network topologies
   DECT NR+ supports various network configurations, like star networks or mesh networks, adapting to specific applications.

High Reliability
   DECT NR+ employs Automatic Repeat Request (ARQ) to correct data transmission errors, ensuring reliable communication for mMTC applications.

Security
   Possibility of hiding the network using AES-128 encryption and integrity protection.
   DECT NR+ communication can be encrypted and the network itself hidden for enhanced security.

High-speed
   DECT NR+ allows selecting the modulation scheme for high-speed communication suitable for various applications.

Comparison of DECT NR+ and LTE
******************************

While both DECT NR+ and LTE address different needs, DECT NR+ offers distinct advantages in specific areas, as summarized by the following table:

.. table:: Feature comparison of DECT NR+ and LTE

   +---------------------------------------+-----------------------------------+----------------------------+
   | Feature                               | DECT NR+                          | LTE                        |
   +=======================================+===================================+============================+
   | License                               | License-exempt 1.9 GHz            | Requires licenses (varies) |
   +---------------------------------------+-----------------------------------+----------------------------+
   | Coverage                              | Shorter, up to 3 km               | Wider, tens of kilometers  |
   +---------------------------------------+-----------------------------------+----------------------------+
   | Performance in congested environments | Better                            | Poor                       |
   +---------------------------------------+-----------------------------------+----------------------------+
   | Network flexibility                   | Star                              | Primarily carrier-provided |
   +---------------------------------------+-----------------------------------+----------------------------+
   | Network Hiding                        | Possible                          | Not available              |
   +---------------------------------------+-----------------------------------+----------------------------+
   | Focus                                 | mMTC, URLLC                       | General purpose            |
   +---------------------------------------+-----------------------------------+----------------------------+
   | SIM card                              | Not required                      | Required                   |
   +---------------------------------------+-----------------------------------+----------------------------+

Getting started with DECT NR+
*****************************

The nRF9161 and nRF9151 SiPs are low power SiPs with an integrated DECT NR+ modem along with 3GPP Release 14 LTE-M/NB-IoT with GNSS.
For getting started with these DECT NR+ products, you can refer to the :ref:`ug_nrf91` documentation, which guides through the features, getting started steps, and advanced usage of these boards.

For details about the key hardware capabilities of the nRF9161 and nRF9151 with regards to the DECT NR+ standard, refer to the `nRF9161 DECT NR+ specification`_ and `nRF9151 DECT NR+ specification`_, respectively.

DECT NR+ in |NCS|
*****************

The following sections explain the different applications, libraries, and samples that use DECT NR+.

Modem library
=============

.. include:: ../../device_guides/nrf91/nrf91_features.rst
    :start-after: nrf91_modem_lib_start
    :end-before: nrf91_modem_lib_end

Library support
===============

A subset of the following |NCS| libraries are used for DECT NR+:

* :ref:`lib_modem`

Applications and samples
========================

The following sample use DECT NR+ in the |NCS|:

* :ref:`nrf_modem_dect_phy_hello`

Power optimization
==================

For optimizing the power consumption of your application, you can use the `Online Power Profiler (OPP)`_ tool.
See :ref:`app_power_opt_nRF91` for more information.
