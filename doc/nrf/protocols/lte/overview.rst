.. _ug_cellular_overview:

Cellular overview
#################

.. contents::
   :local:
   :depth: 2

Nordic Semiconductor offers a wide range of cellular products that support cellular connectivity, particularly focusing on the LTE-M and NB-IoT standards.
For more information on the products, see `Cellular IoT SiPs`_.
The following are main key features of cellular:

* Purely packet-based.
* Provides significantly high data rates.
* Improves latency.
* It uses the spectrum more efficiently.
* It supports scalable carrier bandwidths.
* It includes robust security features.
* It is designed to be backward compatible.

LTE-M and NB-IoT
****************

LTE-M is a low-power wide-area network (LPWAN) communication standard, introduced in 3GPP release 13.
It offers higher bandwidth and lower latency compared to other similar technologies.
This makes it suited for applications such as asset tracking, health monitors, alarm panels, environmental monitoring, and smart parking.

NB-IoT, or Narrowband IoT, is another LPWAN communication standard that was introduced in 3GPP release 13.
It is designed for indoor coverage, low cost, and long battery life.
NB-IoT operates in a small radio frequency band of 200 kHz.
It is ideal for applications like smart metering, environmental monitoring, and smart lighting.

nRF91 Series devices such as nRF9151 DK, nRF9161 DK, nRF9160 DK, Thingy:91, and Thingy:91 X support both LTE-M and NB-IoT technologies.

Comparison between LTE-M and NB-IoT
===================================

The following table shows a comparison between LTE-M and NB-IoT in terms of advantages in using any of these standards:

+------------------+--------------------------------+--------------------------------+
|                  | LTE-M                          | NB-IoT                         |
+==================+================================+================================+
|                  | Higher throughput              | Longer range                   |
+                  +--------------------------------+--------------------------------+
|                  | Lower latency                  | Better penetration             |
+                  +--------------------------------+--------------------------------+
|   Strengths      | Better roaming agreements      | Power efficient at lower data  |
|                  |                                | rate                           |
+                  +--------------------------------+--------------------------------+
|                  | Power efficient at medium data |                                |
|                  | rate                           |                                |
+                  +--------------------------------+--------------------------------+
|                  | Suitable for TCP/TLS secure    |                                |
|                  | connection                     |                                |
+------------------+--------------------------------+--------------------------------+

You can find the key feature comparison between LTE-M and NB-IoT in the `nWP044 - Best practices for cellular IoT development LTE technology`_ documentation.

Factors to be considered for cellular IoT development
*****************************************************

Following are the key considerations to ensure cellular support for your SiP when using Nordic Semiconductor's nRF91 Series devices.

LTE bands
=========

There are 56 LTE bands from 400 MHz to 6 GHz.
Some bands use :term:`Frequency Division Duplex (FDD)` and some use :term:`Time Division Duplex (TDD)`.
The different bands are licensed by different Mobile Network Operators (MNO) around the world.
Half-duplex operation opens for much simpler multiband support and enables :term:`User equipment (UE)` to operate in a larger region.
For LTE-M and NB-IoT, the only known deployment is FDD.

nRF91 Series SiPs support up to 18 LTE bands for global operation.
For details of LTE band support in an nRF91 Series SiP for LTE-M and NB-IoT, see `nRF9151 Product Specification`_, `nRF9161 Product Specification`_, or `nRF9160 Product Specification`_, depending on the SiP you are using.

Physical layer parameters
-------------------------

The following table provides physical layer parameters for LTE-M and NB-IoT:

+----------------------------------+----------------------------------+----------------------------------+
|                                  | LTE-M                            | NB-IoT                           |
+==================================+==================================+==================================+
| Bandwidth                        | 1.4 MHz                          | 200 kHz                          |
+----------------------------------+----------------------------------+----------------------------------+
| Antenna Techniques               | Single antenna                   |  Single antenna                  |
+----------------------------------+----------------------------------+----------------------------------+
| OFDM subcarriers                 |  72 subcarriers                  | 12 subcarriers                   |
+----------------------------------+----------------------------------+----------------------------------+
| Duplexing                        | Full-duplex or half-duplex,      | Half-duplex                      |
|                                  | but networks use only            |                                  |
|                                  | half-duplex FDD                  |                                  |
+----------------------------------+----------------------------------+----------------------------------+
| Modulation in :term:`Uplink (UL)`| QPSK or 16QAM in both UL and DL  | BPSK or QPSK in UL depending on  |
| and :term:`Downlink (DL)`        | depending on signal quality      | signal quality,                  |
|                                  |                                  | only QPSK in DL                  |
+----------------------------------+----------------------------------+----------------------------------+

Network support
================

A factor governing the selection of the access technology is the MNO support in the areas where you wish to deploy your product.
Most cellular operators support both LTE-M and NB-IoT, but there are geographic locations where only one of these is supported.
See `Mobile IoT deployment map`_ for more information.

Certifications
==============

Some MNOs, such as Verizon and Vodafone, have their own certification requirements.
In such cases, it is advised to engage with the MNO certification programs and contact them at the earliest.
This is to receive certification-related requirements and better understand the potential certification costs, processes, and timelines.
But many operators only require GCF (Global Certification Forum) or PTCRB certifications that devices work as per standard, and regulatory certifications.

For more information about certification related to the nRF91 Series SiP, refer to the following pages:

* `nRF91 Series certifications`_
* `Certifying a cellular IoT device`_

SIM card support
================

SIMs that are used with the nRF91 Series devices must support LTE-M, NB-IoT, or both.
The iBasis SIM is bundled with the nRF9160 DK and Thingy:91, the Onomondo SIM with an nRF91x1 DK and Thingy:91 X, and the Wireless Logic SIM card with the nRF9151 DK and Thingy:91 X.
Check the `iBasis IoT network coverage`_, `Onomondo LTE-M coverage`_, `Onomondo NB-IoT coverage`_, or `Wireless Logic LTE-M/NB-IoT network coverage`_ pages to see the network coverage for different countries, depending on the SIM card you are using.

Software SIM
------------

The nRF91 Series supports software SIM, which allows the usage of software SIM-based solutions to reduce energy consumption associated with physical SIMs.
The following are some of the key advantages of using software SIM:

* Cost savings and simplified design by excluding the SIM hardware layer.
* Power savings, especially in idle mode using :term:`Extended Discontinuous Reception (eDRX)`.

The traditional carriers (not MVNOs like iBasis or Onomondo) do not allow their profiles to be provisioned or uploaded into SoftSIMs.
These carriers need to use profiles from SoftSIM providers like Onomondo.
See the software SIM support section of the `Cellular IoT unique features`_ documentation for information on software SIM.

The software SIM support is available in the |NCS| from the v2.5.0 release.
The `Onomondo SoftSIM integration with the nRF91 Series`_ guide describes the integration of software SIM into |NCS|, and the :ref:`nrf_modem_softsim` guide demonstrates the transfer of SIM data between the modem and the application.

Power consumption
=================

Both LTE-M and NB-IoT support :term:`Power Saving Mode (PSM)`, eDRX, and :term:`Release Assistance Indication (RAI)` to minimize power consumption.
For more information on power-saving techniques, refer to the DevAcademy's `Power saving techniques`_ documentation.

Security
========

The nRF91 Series devices include a range of security features, such as Arm TrustZone and Arm CryptoCell for secure application and data handling.
For more information, refer to the :ref:`app_boards_spe_nspe` documentation.

Security best practices are also implemented to protect data transmitted over the cellular network.
This includes using Transport Layer Security (TLS) for Transmission Control Protocol (TCP) and Datagram Transport Layer Security (DTLS) for User Datagram Protocol (UDP).
For more information, refer to the `Security protocol for cellular IoT`_ documentation.

References
**********

* `nWP044 - Best practices for cellular IoT development`_ whitepaper - Describes the guidelines that you need to consider when you start the development for a low power cellular IoT product.
* `How nRF9160 enables cellular IoT asset tracking`_ webinar.
* `Accelerate cellular product development`_ webinar.
