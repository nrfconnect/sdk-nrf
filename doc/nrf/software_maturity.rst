.. _software_maturity:

Software maturity levels
########################

.. contents::
   :local:
   :depth: 2

The |NCS| supports its various features and components at different levels of software maturity.
The tables on this page summarize the maturity level for each feature and component supported in the |NCS|.

Software maturity categories
****************************

The following categories are used in the tables to classify the software maturity of each feature and component:

Supported
   The feature or component is implemented and maintained, and is suitable for product development.

Not supported
   The feature or component is neither implemented nor maintained, and it does not work.

Experimental
   The feature can be used for development, but it is not recommended for production.
   This means that the feature is incomplete in functionality or verification and can be expected to change in future releases.
   The feature is made available in its current state, but the design and interfaces can change between release tags.
   The feature is also labeled as ``EXPERIMENTAL`` in Kconfig files to indicate this status.

   .. note::
      By default, the build system generates build warnings to indicate when features labeled ``EXPERIMENTAL`` are included in builds.
      To disable these warnings, disable the :kconfig:option:`CONFIG_WARN_EXPERIMENTAL` Kconfig option.
      See :ref:`app_build_additions` for details.

See the following table for more details:

.. _software_maturity_table:

.. list-table:: Software maturity
   :header-rows: 1
   :align: center
   :widths: auto

   * -
     - Supported
     - Experimental
     - Not supported
   * - **Technical support**
     - Customer issues raised for features supported for developing end products on tagged |NCS| releases are handled by technical support on DevZone.
     - Customer issues raised for experimental features on tagged |NCS| releases are handled by technical support on DevZone.
     - Not available.
   * - **Bug fixing**
     - Reported critical bugs and security fixes will be resolved in both ``main`` and the latest tagged versions.
     - Bug fixes, security fixes, and improvements are not guaranteed to be applied.
     - Not available.
   * - **Implementation completeness**
     - Complete implementation of the agreed features set.
     - Significant changes may be applied in upcoming versions.
     - A feature or component is available as an implementation, but is not compatible with (or tested on) a specific device or series of products.
   * - **API definition**
     - The API definition is stable.
     - The API definition is not stable and may change.
     - Not available.
   * - **Maturity**
     - Suitable for integration in end products.

       A feature or component that is either fully complete on first commit, or has previously been labelled *experimental* and is now ready for use in end products.

     - Suitable for prototyping or evaluation.
       Not recommended to be deployed in end products.

       A feature or component that is either not fully verified according to the existing test plans or currently being developed, meaning that it is incomplete or that it may change in the future.
     - Not applicable.

   * - **Verification**
     - Fully verified according to the existing test plans.
     - Incomplete verification
     - Not applicable.

.. _api_deprecation:

API deprecation
***************

The **Deprecated** status is assigned to API that has gone through all maturity levels, but is being phased out.
The deprecated API will be removed in one of future releases, no earlier than two releases after the deprecation is announced and only when the code has transitioned to not using the deprecated API.
The experimental API can be removed without deprecation notification.
Following :ref:`Zephyr's guidelines for API lifecycle <zephyr:api_lifecycle>`, the API documentation informs about the deprecation and attempts to use a deprecated API at build time will log a warning to the console.

.. _software_maturity_application:

Application support
*******************

The following subsections indicate the software maturity levels of the support for :ref:`applications <applications>`.

.. note::
    Features not listed are not supported.

.. _software_maturity_application_nrf5340audio:

nRF5340 Audio
=============

The following table indicates the software maturity levels of the support for the :ref:`nrf53_audio_app` application.

.. _software_maturity_application_nrf5340audio_table:

.. toggle::

   .. list-table:: nRF5340 Audio application feature support
      :header-rows: 1
      :align: center
      :widths: auto

      * - Feature
        - Description
        - Limitations
        - Maturity level
      * - **Broadcast source**
        - Transmitting broadcast audio using Broadcast Isochronous Stream (BIS) and Broadcast Isochronous Group (BIG).

          Play and pause emulated by disabling and enabling stream, respectively.
        - The following limitations apply:

          * Basic Audio Profile (BAP) broadcast, one BIG with two BIS streams.
          * Audio input: USB or I2S (Line in or using Pulse Density Modulation).
          * Configuration: 16 bit, several bit rates ranging from 24 kbps to 160 kbps.

        - Experimental
      * - **Broadcast sink**
        - Receiving broadcast audio using BIS and BIG.

          Synchronizes and unsynchronizes with the stream.
        - The following limitations apply:

          * BAP broadcast, one BIG, one of the two BIS streams (selectable).
          * Audio output: I2S/Analog headset output.
          * Configuration: 16 bit, several bit rates ranging from 24 kbps to 160 kbps.

        - Experimental
      * - **Unicast client**
        - BAP unicast, one Connected Isochronous Group (CIG) with two Connected Isochronous Streams (CIS).

          Transmitting unidirectional or transceiving bidirectional audio using CIG and CIS.
        - The following limitations apply:

          * BAP unicast, one CIG with two CIS.
          * Audio input: USB or I2S (Line in or using Pulse Density Modulation).
          * Audio output: USB or I2S/Analog headset output.
          * Configuration: 16 bit, several bit rates ranging from 24 kbps to 160 kbps.

        - Experimental
      * - **Unicast server**
        - BAP unicast, 1 CIG with 2 CIS streams.

          Receiving unidirectional or transceiving bidirectional audio using CIG and CIS.

          Coordinated Set Identification Service (CSIS) is implemented on the server side.
        - The following limitations apply:

          * BAP unicast, one CIG, one of the two CIS streams (selectable).
          * Audio output: I2S/Analog headset output.
          * Audio input: PDM microphone over I2S.
          * Configuration: 16 bit, several bit rates ranging from 24 kbps to 160 kbps.

        - Experimental

.. _software_maturity_protocol:

Protocol support
****************

The following table indicates the software maturity levels of the support for each :ref:`protocol <protocols>`:

+---------------+-----------+-----------+-----------+-----------+-----------+-----------+-----------+-----------+-----------+
|               | nRF52810  | nRF52811  | nRF52820  | nRF52832  | nRF52833  | nRF52840  | nRF5340   | nRF9160   | nRF9161   |
+===============+===========+===========+===========+===========+===========+===========+===========+===========+===========+
| **Bluetooth** | Supported | Supported | Supported | Supported | Supported | Supported | Supported | \-        | \-        |
+---------------+-----------+-----------+-----------+-----------+-----------+-----------+-----------+-----------+-----------+
| **Bluetooth** | \-        | -\        | -\        | Supported | Supported | Supported | Supported | \-        | \-        |
| **Mesh**      |           |           |           |           |           |           |           |           |           |
+---------------+-----------+-----------+-----------+-----------+-----------+-----------+-----------+-----------+-----------+
| **HomeKit**   | \-        | \-        | \-        | \-        | Supported | Supported | Supported | \-        | \-        |
+---------------+-----------+-----------+-----------+-----------+-----------+-----------+-----------+-----------+-----------+
| **LTE**       | \-        | \-        | \-        | \-        | \-        | \-        | \-        | Supported | Supported |
+---------------+-----------+-----------+-----------+-----------+-----------+-----------+-----------+-----------+-----------+
| **Matter**    | \-        | \-        | \-        | \-        | \-        | Supported | Supported | \-        | \-        |
+---------------+-----------+-----------+-----------+-----------+-----------+-----------+-----------+-----------+-----------+
| **Thread**    | \-        | \-        | \-        | \-        | Supported | Supported | Supported | \-        | \-        |
+---------------+-----------+-----------+-----------+-----------+-----------+-----------+-----------+-----------+-----------+
| **Wi-Fi**     | \-        | \-        | \-        | \-        | \-        | \-        | \-        | \-        | \-        |
+---------------+-----------+-----------+-----------+-----------+-----------+-----------+-----------+-----------+-----------+
| **Zigbee**    | \-        | \-        | \-        | \-        | Supported | Supported | Supported | \-        | \-        |
+---------------+-----------+-----------+-----------+-----------+-----------+-----------+-----------+-----------+-----------+

Bluetooth features support
**************************

The following table indicates the software maturity levels of the support for each Bluetooth feature:

.. toggle::

   +----------------------------------------------+----------+----------+----------+----------+----------+----------+----------+---------+---------+
   |                                              | nRF52810 | nRF52811 | nRF52820 | nRF52832 | nRF52833 | nRF52840 | nRF5340  | nRF9160 | nRF9161 |
   +==============================================+==========+==========+==========+==========+==========+==========+==========+=========+=========+
   | **Bluetooth LE Peripheral/Central**          | Supported| Supported| Supported| Supported| Supported| Supported| Supported| \-      | \-      |
   +----------------------------------------------+----------+----------+----------+----------+----------+----------+----------+---------+---------+
   | **Connectionless/Connected CTE Transmitter** | \-       | Supported| Supported| \-       | Supported| \-       | Supported| \-      | \-      |
   +----------------------------------------------+----------+----------+----------+----------+----------+----------+----------+---------+---------+
   | **LE Coded PHY**                             | \-       | Supported| Supported| \-       | Supported| Supported| Supported| \-      | \-      |
   +----------------------------------------------+----------+----------+----------+----------+----------+----------+----------+---------+---------+
   | **LLPM**                                     | \-       | \-       | \-       | Supported| Supported| Supported| \-       | \-      | \-      |
   +----------------------------------------------+----------+----------+----------+----------+----------+----------+----------+---------+---------+

HomeKit features support
************************

The following table indicates the software maturity levels of the support for each HomeKit feature:

.. toggle::

   .. list-table::
      :widths: 20 10 10 10 10 10 10 10 10 10
      :header-rows: 1
      :align: center

      * - Feature
        - nRF52810
        - nRF52811
        - nRF52820
        - nRF52832
        - nRF52833
        - nRF52840
        - nRF5340
        - nRF9160
        - nRF9161
      * - **HomeKit - OTA DFU over Bluetooth LE**
        -  --
        -  --
        -  --
        -  --
        -  --
        - Supported
        - Supported
        -  --
        -  --
      * - **HomeKit - OTA DFU over HomeKit**
        - --
        - --
        - --
        - --
        - --
        - Supported
        - Supported
        - --
        - --
      * - **HomeKit commissioning over Bluetooth LE with NFC**
        - --
        - --
        - --
        - --
        - Supported
        - Supported
        - Supported
        - --
        - --
      * - **HomeKit commissioning over Bluetooth LE with QR code**
        - --
        - --
        - --
        - --
        - Supported
        - Supported
        - Supported
        - --
        - --
      * - **HomeKit over Bluetooth LE**
        - --
        - --
        - --
        - --
        - Supported
        - Supported
        - Supported
        - --
        - --
      * - **HomeKit over Thread FTD**
        - --
        - --
        - --
        - --
        - --
        - Supported
        - Supported
        - --
        - --
      * - **HomeKit over Thread MTD SED**
        - --
        - --
        - --
        - --
        - --
        - Supported
        - Supported
        - --
        - --

Thread features support
***********************

The following table indicates the software maturity levels of the support for each Thread feature:

.. toggle::

   +------------------------------------------------+----------+----------+----------+----------+----------+--------------+--------------+---------+---------+
   |                                                | nRF52810 | nRF52811 | nRF52820 | nRF52832 | nRF52833 | nRF52840     | nRF5340      | nRF9160 | nRF9161 |
   +================================================+==========+==========+==========+==========+==========+==============+==============+=========+=========+
   | **Thread + nRF21540 (GPIO)**                   | \-       | \-       | \-       | \-       | \-       | Supported    | \-           | \-      | \-      |
   +------------------------------------------------+----------+----------+----------+----------+----------+--------------+--------------+---------+---------+
   | **Thread - Full Thread Device (FTD)**          | \-       | \-       | \-       | \-       | Supported| Supported    | Supported    | \-      | \-      |
   +------------------------------------------------+----------+----------+----------+----------+----------+--------------+--------------+---------+---------+
   | **Thread - Minimal Thread Device (MTD)**       | \-       | \-       | \-       | \-       | \-       | Supported    | Supported    | \-      | \-      |
   +------------------------------------------------+----------+----------+----------+----------+----------+--------------+--------------+---------+---------+
   | **Thread 1.1**                                 | \-       | \-       | \-       | \-       | Supported| Supported    | Supported    | \-      | \-      |
   +------------------------------------------------+----------+----------+----------+----------+----------+--------------+--------------+---------+---------+
   | **Thread 1.2 - CSL Receiver**                  | \-       | \-       | \-       | \-       | Supported| Supported    | Supported    | \-      | \-      |
   +------------------------------------------------+----------+----------+----------+----------+----------+--------------+--------------+---------+---------+
   | **Thread 1.2 - Core**                          | \-       | \-       | \-       | \-       | Supported| Supported    | Supported    | \-      | \-      |
   +------------------------------------------------+----------+----------+----------+----------+----------+--------------+--------------+---------+---------+
   | **Thread 1.2 - Link Metrics**                  | \-       | \-       | \-       | \-       | Supported| Supported    | Supported    | \-      | \-      |
   +------------------------------------------------+----------+----------+----------+----------+----------+--------------+--------------+---------+---------+
   | **Thread 1.3 - Core**                          | \-       | \-       | \-       | \-       | Supported| Supported    | Supported    | \-      | \-      |
   +------------------------------------------------+----------+----------+----------+----------+----------+--------------+--------------+---------+---------+
   | **Thread FTD + Bluetooth LE multiprotocol**    | \-       | \-       | \-       | \-       | \-       | Experimental | Experimental | \-      | \-      |
   +------------------------------------------------+----------+----------+----------+----------+----------+--------------+--------------+---------+---------+
   | **Thread MTD + Bluetooth LE multiprotocol**    | \-       | \-       | \-       | \-       | \-       | Supported    | Supported    | \-      | \-      |
   +------------------------------------------------+----------+----------+----------+----------+----------+--------------+--------------+---------+---------+
   | **Thread Radio Co-Processor (RCP)**            | \-       | \-       | \-       | \-       | Supported| Supported    | \-           | \-      | \-      |
   +------------------------------------------------+----------+----------+----------+----------+----------+--------------+--------------+---------+---------+
   | **Thread TCP**                                 | \-       | \-       | \-       | \-       | \-       | Experimental | Experimental | \-      | \-      |
   +------------------------------------------------+----------+----------+----------+----------+----------+--------------+--------------+---------+---------+

.. _software_maturity_protocol_matter:

Matter features support
***********************

The following table indicates the software maturity levels of the support for each Matter feature:

.. toggle::

   +-----------------------------------------------------------------------+----------+----------+----------+----------+----------+--------------+--------------+---------+---------+
   |                                                                       | nRF52810 | nRF52811 | nRF52820 | nRF52832 | nRF52833 | nRF52840     | nRF5340      | nRF9160 | nRF9161 |
   +=======================================================================+==========+==========+==========+==========+==========+==============+==============+=========+=========+
   | **Matter - OTA DFU over Bluetooth LE**                                | \-       | \-       | \-       | \-       | \-       | Experimental | Supported    | \-      | \-      |
   +-----------------------------------------------------------------------+----------+----------+----------+----------+----------+--------------+--------------+---------+---------+
   | **Matter Intermittently Connected Device**                            | \-       | \-       | \-       | \-       | \-       | Experimental | Experimental | \-      | \-      |
   +-----------------------------------------------------------------------+----------+----------+----------+----------+----------+--------------+--------------+---------+---------+
   | **Matter commissioning over Bluetooth LE with NFC onboarding**        | \-       | \-       | \-       | \-       | \-       | Experimental | Supported    | \-      | \-      |
   +-----------------------------------------------------------------------+----------+----------+----------+----------+----------+--------------+--------------+---------+---------+
   | **Matter commissioning over Bluetooth LE with QR code onboarding**    | \-       | \-       | \-       | \-       | \-       | Experimental | Supported    | \-      | \-      |
   +-----------------------------------------------------------------------+----------+----------+----------+----------+----------+--------------+--------------+---------+---------+
   | **Matter commissioning over IP**                                      | \-       | \-       | \-       | \-       | \-       | Experimental | Supported    | \-      | \-      |
   +-----------------------------------------------------------------------+----------+----------+----------+----------+----------+--------------+--------------+---------+---------+
   | **Matter over Thread**                                                | \-       | \-       | \-       | \-       | \-       | Experimental | Experimental | \-      | \-      |
   +-----------------------------------------------------------------------+----------+----------+----------+----------+----------+--------------+--------------+---------+---------+
   | **Matter over Wi-Fi**                                                 | \-       | \-       | \-       | \-       | \-       | \-           | Supported    | \-      | \-      |
   +-----------------------------------------------------------------------+----------+----------+----------+----------+----------+--------------+--------------+---------+---------+
   | **OTA DFU over Matter**                                               | \-       | \-       | \-       | \-       | \-       | Experimental | Supported    | \-      | \-      |
   +-----------------------------------------------------------------------+----------+----------+----------+----------+----------+--------------+--------------+---------+---------+

Zigbee feature support
**********************

The following table indicates the software maturity levels of the support for each Zigbee feature:

.. toggle::

   +-------------------------------------------+----------+----------+----------+----------+--------------+--------------+--------------+---------+---------+
   |                                           | nRF52810 | nRF52811 | nRF52820 | nRF52832 |   nRF52833   |   nRF52840   |   nRF5340    | nRF9160 | nRF9161 |
   +===========================================+==========+==========+==========+==========+==============+==============+==============+=========+=========+
   | **OTA DFU over Zigbee**                   |    \-    |    \-    |    \-    |    \-    |      \-      | Experimental | Experimental |   \-    |   \-    |
   +-------------------------------------------+----------+----------+----------+----------+--------------+--------------+--------------+---------+---------+
   | **Zigbee (Sleepy) End Device**            |    \-    |    \-    |    \-    |    \-    | Experimental | Experimental | Experimental |   \-    |   \-    |
   +-------------------------------------------+----------+----------+----------+----------+--------------+--------------+--------------+---------+---------+
   | **Zigbee + Bluetooth LE multiprotocol**   |    \-    |    \-    |    \-    |    \-    | Experimental | Experimental | Experimental |   \-    |   \-    |
   +-------------------------------------------+----------+----------+----------+----------+--------------+--------------+--------------+---------+---------+
   | **Zigbee + nRF21540 (GPIO)**              |    \-    |    \-    |    \-    |    \-    |      \-      | Experimental |      \-      |   \-    |   \-    |
   +-------------------------------------------+----------+----------+----------+----------+--------------+--------------+--------------+---------+---------+
   | **Zigbee Coordinator**                    |    \-    |    \-    |    \-    |    \-    | Experimental | Experimental | Experimental |   \-    |   \-    |
   +-------------------------------------------+----------+----------+----------+----------+--------------+--------------+--------------+---------+---------+
   | **Zigbee Network Co-Processor (NCP)**     |    \-    |    \-    |    \-    |    \-    | Experimental | Experimental | Experimental |   \-    |   \-    |
   +-------------------------------------------+----------+----------+----------+----------+--------------+--------------+--------------+---------+---------+
   | **Zigbee Router**                         |    \-    |    \-    |    \-    |    \-    | Experimental | Experimental | Experimental |   \-    |   \-    |
   +-------------------------------------------+----------+----------+----------+----------+--------------+--------------+--------------+---------+---------+

Wi-Fi feature support
**********************

The following table indicates the software maturity levels of the support for each Wi-Fi feature:

.. toggle::

   +-----------------------------------------------+----------+----------+----------+----------+----------+----------+---------+---------+---------+
   |                                               | nRF52810 | nRF52811 | nRF52820 | nRF52832 | nRF52833 | nRF52840 | nRF5340 | nRF9160 | nRF9161 |
   +===============================================+==========+==========+==========+==========+==========+==========+=========+=========+=========+
   | **Bluetooth LE Co-existence**                 | \-       | \-       | \-       | \-       | \-       | \-       | \-      | \-      | \-      |
   +-----------------------------------------------+----------+----------+----------+----------+----------+----------+---------+---------+---------+
   | **STA Mode**                                  | \-       | \-       | \-       | \-       | \-       | \-       | \-      | \-      | \-      |
   +-----------------------------------------------+----------+----------+----------+----------+----------+----------+---------+---------+---------+
   | **Scan only (for location accuracy)**         | \-       | \-       | \-       | \-       | \-       | \-       | \-      | \-      | \-      |
   +-----------------------------------------------+----------+----------+----------+----------+----------+----------+---------+---------+---------+
   | **SoftAP Mode**                               | \-       | \-       | \-       | \-       | \-       | \-       | \-      | \-      | \-      |
   +-----------------------------------------------+----------+----------+----------+----------+----------+----------+---------+---------+---------+

Security Feature Support
************************

The following sections contain the tables indicating the software maturity levels of the support for the following security features:

* Trusted Firmware-M
* PSA Crypto
* |NSIB|
* Hardware Unique Key

Trusted Firmware-M support
==========================

.. toggle::

   +------------------+----------+----------+----------+----------+----------+----------+--------------+--------------+--------------+
   |                  | nRF52810 | nRF52811 | nRF52820 | nRF52832 | nRF52833 | nRF52840 | nRF5340      | nRF9160      | nRF9161      |
   +==================+==========+==========+==========+==========+==========+==========+==============+==============+==============+
   | **Full build**   | \-       | \-       | \-       | \-       | \-       | \-       | Experimental | Experimental | Experimental |
   +------------------+----------+----------+----------+----------+----------+----------+--------------+--------------+--------------+
   | **Minimal Build**| \-       | \-       | \-       | \-       | \-       | \-       | Supported    | Supported    | Supported    |
   +------------------+----------+----------+----------+----------+----------+----------+--------------+--------------+--------------+

PSA Crypto support
==================

.. toggle::

   +---------------------+----------+----------+----------+----------+----------+----------+----------+----------+----------+
   |                     | nRF52810 | nRF52811 | nRF52820 | nRF52832 | nRF52833 | nRF52840 | nRF5340  | nRF9160  | nRF9161  |
   +=====================+==========+==========+==========+==========+==========+==========+==========+==========+==========+
   | **PSA Crypto APIs** | \-       | \-       | \-       | Supported| Supported| Supported| Supported| Supported| Supported|
   +---------------------+----------+----------+----------+----------+----------+----------+----------+----------+----------+

|NSIB|
======

.. toggle::

   +-----------------------------------------------+----------+----------+----------+----------+----------+----------+----------+----------+----------+
   |                                               | nRF52810 | nRF52811 | nRF52820 | nRF52832 | nRF52833 | nRF52840 | nRF5340  | nRF9160  | nRF9161  |
   +===============================================+==========+==========+==========+==========+==========+==========+==========+==========+==========+
   | **Immutable Bootloader as part of build**     | \-       | \-       | \-       | Supported| Supported| Supported| Supported| Supported| Supported|
   +-----------------------------------------------+----------+----------+----------+----------+----------+----------+----------+----------+----------+


Hardware Unique Key
===================

.. toggle::

   +-----------------------------------------------+----------+----------+----------+----------+----------+----------+----------+--------------+--------------+
   |                                               | nRF52810 | nRF52811 | nRF52820 | nRF52832 | nRF52833 | nRF52840 | nRF5340  | nRF9160      | nRF9161      |
   +===============================================+==========+==========+==========+==========+==========+==========+==========+==============+==============+
   | **Key Derivation from Hardware Unique Key**   | \-       | \-       | \-       | \-       | \-       | Supported| Supported| Supported    | Experimental |
   +-----------------------------------------------+----------+----------+----------+----------+----------+----------+----------+--------------+--------------+
