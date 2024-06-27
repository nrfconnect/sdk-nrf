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
   The feature is also labeled as :ref:`experimental in Kconfig files <app_build_additions_experimental>` and a build warning is generated to indicate this status.

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

For the certification status of different features in a specific SoC, see its Compatibility Matrix in the `Nordic Semiconductor TechDocs`_.

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

The following table indicates the software maturity levels of the support for the :ref:`nrf53_audio_app`.

.. toggle::

   .. _software_maturity_application_nrf5340audio_table:

   .. list-table:: nRF5340 Audio application feature support
      :header-rows: 1
      :align: center
      :widths: auto

      * - Application
        - Description
        - Limitations
        - Maturity level
      * - :ref:`Broadcast source <nrf53_audio_broadcast_source_app>`
        - Broadcasting audio using Broadcast Isochronous Stream (BIS) and Broadcast Isochronous Group (BIG).

          Play and pause emulated by disabling and enabling stream, respectively.
        - The following limitations apply:

          * One BIG with two BIS streams.
          * Audio input: USB or I2S (Line in or using Pulse Density Modulation).
          * Configuration: 16 bit, several bit rates ranging from 32 kbps to 124 kbps.

        - Experimental
      * - :ref:`Broadcast sink <nrf53_audio_broadcast_sink_app>`
        - Receiving broadcast audio using BIS and BIG.

          Synchronizes and unsynchronizes with the stream.
        - The following limitations apply:

          * One BIG, one of the two BIS streams (selectable).
          * Audio output: I2S/Analog headset output.
          * Configuration: 16 bit, several bit rates ranging from 32 kbps to 124 kbps.

        - Experimental
      * - :ref:`Unicast client <nrf53_audio_unicast_client_app>`
        - One Connected Isochronous Group (CIG) with two Connected Isochronous Streams (CIS).

          Transmitting unidirectional or transceiving bidirectional audio using CIG and CIS.
        - The following limitations apply:

          * One CIG with two CIS.
          * Audio input: USB or I2S (Line in or using Pulse Density Modulation).
          * Audio output: USB or I2S/Analog headset output.
          * Configuration: 16 bit, several bit rates ranging from 32 kbps to 124 kbps.

        - Experimental
      * - :ref:`Unicast server <nrf53_audio_unicast_server_app>`
        - One CIG with one CIS stream.

          Receiving unidirectional or transceiving bidirectional audio using CIG and CIS.

          Coordinated Set Identification Service (CSIS) is implemented on the server side.
        - The following limitations apply:

          * One CIG, one of the two CIS streams (selectable).
          * Audio output: I2S/Analog headset output.
          * Audio input: PDM microphone over I2S.
          * Configuration: 16 bit, several bit rates ranging from 32 kbps to 124 kbps.

        - Experimental

.. _software_maturity_protocol:

Protocol support
****************

The following table indicates the software maturity levels of the support for each :ref:`protocol <protocols>`:

+--------------------+-----------+-----------+-----------+-----------+-----------+---------------------+---------------------+--------------+--------------+--------------+---------------------+---------------------+
|                    | nRF52810  | nRF52811  | nRF52820  | nRF52832  | nRF52833  | nRF52840            | nRF5340             | nRF54H20     | nRF54L15     | nRF9151      | nRF9160             | nRF9161             |
+====================+===========+===========+===========+===========+===========+=====================+=====================+==============+==============+==============+=====================+=====================+
| **Bluetooth**      | Supported | Supported | Supported | Supported | Supported | Supported           | Supported           | Experimental | Experimental | --           | --                  | --                  |
+--------------------+-----------+-----------+-----------+-----------+-----------+---------------------+---------------------+--------------+--------------+--------------+---------------------+---------------------+
| **Bluetooth Mesh** | --        | --        | --        | Supported | Supported | Supported           | Supported           | --           | Experimental | --           | --                  | --                  |
+--------------------+-----------+-----------+-----------+-----------+-----------+---------------------+---------------------+--------------+--------------+--------------+---------------------+---------------------+
| **DECT NR+ PHY**   | --        | --        | --        | --        | --        | --                  | --                  | --           | --           | Experimental | --                  | Experimental        |
+--------------------+-----------+-----------+-----------+-----------+-----------+---------------------+---------------------+--------------+--------------+--------------+---------------------+---------------------+
| **LTE**            | --        | --        | --        | --        | --        | --                  | --                  | --           | --           | Supported    | Supported           | Supported           |
+--------------------+-----------+-----------+-----------+-----------+-----------+---------------------+---------------------+--------------+--------------+--------------+---------------------+---------------------+
| **Matter**         | --        | --        | --        | --        | --        | Supported           | Supported           | Experimental | Experimental | --           | --                  | --                  |
+--------------------+-----------+-----------+-----------+-----------+-----------+---------------------+---------------------+--------------+--------------+--------------+---------------------+---------------------+
| **Sidewalk**       | --        | --        | --        | --        | --        | Supported           | Supported           | --           | Experimental | --           | --                  | --                  |
+--------------------+-----------+-----------+-----------+-----------+-----------+---------------------+---------------------+--------------+--------------+--------------+---------------------+---------------------+
| **Thread**         | --        | --        | --        | --        | Supported | Supported           | Supported           | Experimental | Experimental | --           | --                  | --                  |
+--------------------+-----------+-----------+-----------+-----------+-----------+---------------------+---------------------+--------------+--------------+--------------+---------------------+---------------------+
| **Wi-Fi**          | --        | --        | --        | --        | --        | Supported\ :sup:`1` | Supported\ :sup:`2` | Experimental | Experimental | Supported    | Supported\ :sup:`1` | Supported\ :sup:`1` |
+--------------------+-----------+-----------+-----------+-----------+-----------+---------------------+---------------------+--------------+--------------+--------------+---------------------+---------------------+
| **Zigbee**         | --        | --        | --        | --        | Supported | Supported           | Supported           | --           | --           | --           | --                  | --                  |
+--------------------+-----------+-----------+-----------+-----------+-----------+---------------------+---------------------+--------------+--------------+--------------+---------------------+---------------------+

| [1]: Only with nRF7002 EK, nRF7002 EK in nRF7000 emulation mode or nRF7002 EK in nRF7001 emulation mode
| [2]: Only with nRF7002 DK, nRF7002 DK in nRF7001 emulation mode, nRF7002 EB, nRF7002 EK, nRF7002 EK in nRF7000 emulation mode or nRF7002 EK in nRF7001 emulation mode

Amazon Sidewalk features support
********************************

The following table indicates the software maturity levels of the support for each Amazon Sidewalk feature:

.. toggle::

  .. sml-table:: sidewalk
     :insert-values: [("Sidewalk - OTA DFU over Bluetooth LE","nRF54L15","Experimental"), ("Sidewalk File Transfer (FUOTA)","nRF54L15","Experimental"), ("Sidewalk Multi-link + Auto-connect","nRF54L15","Experimental"), ("Sidewalk on-device certification","nRF54L15","Experimental"),("Sidewalk over BLE","nRF54L15","Experimental"), ("Sidewalk over FSK","nRF54L15","Experimental"), ("Sidewalk over LORA","nRF54L15","Experimental")]

Bluetooth features support
**************************

The following table indicates the software maturity levels of the support for each Bluetooth feature:

.. toggle::

  .. sml-table:: bluetooth
     :insert-values: [("Bluetooth LE Peripheral/Central","nRF54H20","Experimental"), ("Connectionless/Connected CTE Transmitter","nRF54H20","Experimental"), ("LE Coded PHY","nRF54H20","Experimental"), ("LLPM","nRF54H20","Experimental"),("Bluetooth LE Peripheral/Central","nRF54L15","Experimental"), ("Connectionless/Connected CTE Transmitter","nRF54L15","Experimental"), ("LE Coded PHY","nRF54L15","Experimental"), ("LLPM","nRF54L15","Experimental")]

Thread features support
***********************

The following table indicates the software maturity levels of the support for each Thread feature:

.. toggle::

  .. sml-table:: thread
     :insert-values: [("Thread - Full Thread Device (FTD)","nRF54H20","Experimental"), ("Thread - Minimal Thread Device (MTD)","nRF54H20","Experimental"), ("Thread 1.1","nRF54H20","Experimental"), ("Thread 1.2 - CSL Receiver","nRF54H20","Experimental"),("Thread 1.2 - Core","nRF54H20","Experimental"), ("Thread 1.2 - Link Metrics","nRF54H20","Experimental"), ("Thread 1.3 - Core","nRF54H20","Experimental"), ("Thread FTD + Bluetooth LE multiprotocol","nRF54H20","Experimental"), ("Thread MTD + Bluetooth LE multiprotocol","nRF54H20","Experimental"), ("Thread TCP","nRF54H20","Experimental"), ("Thread - Full Thread Device (FTD)","nRF54L15","Experimental"), ("Thread - Minimal Thread Device (MTD)","nRF54L15","Experimental"), ("Thread 1.1","nRF54L15","Experimental"), ("Thread 1.2 - CSL Receiver","nRF54L15","Experimental"),("Thread 1.2 - Core","nRF54L15","Experimental"), ("Thread 1.2 - Link Metrics","nRF54L15","Experimental"), ("Thread 1.3 - Core","nRF54L15","Experimental"), ("Thread FTD + Bluetooth LE multiprotocol","nRF54L15","Experimental"), ("Thread MTD + Bluetooth LE multiprotocol","nRF54L15","Experimental"), ("Thread Radio Co-Processor (RCP)","nRF54L15","Experimental"), ("Thread TCP","nRF54L15","Experimental")]

.. _software_maturity_protocol_matter:

Matter features support
***********************

The following table indicates the software maturity levels of the support for each Matter feature:

.. toggle::

  .. sml-table:: matter
     :insert-values: [("Matter Intermittently Connected Device","nRF54H20","Experimental"), ("Matter commissioning over Bluetooth LE with NFC onboarding","nRF54H20","Experimental"), ("Matter commissioning over Bluetooth LE with QR code onboarding","nRF54H20","Experimental"), ("Matter commissioning over IP","nRF54H20","Experimental"),("Matter over Thread","nRF54H20","Experimental"), ("Matter - OTA DFU over Bluetooth LE","nRF54L15","Experimental"), ("Matter Intermittently Connected Device","nRF54L15","Experimental"), ("Matter commissioning over Bluetooth LE with NFC onboarding","nRF54L15","Experimental"), ("Matter commissioning over Bluetooth LE with QR code onboarding","nRF54L15","Experimental"),("Matter commissioning over IP","nRF54L15","Experimental"), ("Matter over Thread","nRF54L15","Experimental"), ("OTA DFU over Matter","nRF54L15","Experimental")]

Zigbee feature support
**********************

The following table indicates the software maturity levels of the support for each Zigbee feature:

.. toggle::

  .. sml-table:: zigbee

Wi-Fi feature support
**********************

The following table indicates the software maturity levels of the support for each Wi-Fi feature:

.. toggle::

   +------------------------------------------+------------+------------+------------+------------+------------+------------------------+------------------------+------------------------+------------------------+-----------+---------------------+---------------------+
   | Feature                                  | nRF52810   | nRF52811   | nRF52820   | nRF52832   | nRF52833   | nRF52840               | nRF5340                | nRF54H20               | nRF54L15               | nRF9151   | nRF9160             | nRF9161             |
   +==========================================+============+============+============+============+============+========================+========================+========================+========================+===========+=====================+=====================+
   | **Bluetooth LE Coexistence**             | --         | --         | --         | --         | --         | Experimental           | Supported\ :sup:`1`    | --                     | --                     | --        | --                  | --                  |
   +------------------------------------------+------------+------------+------------+------------+------------+------------------------+------------------------+------------------------+------------------------+-----------+---------------------+---------------------+
   | **Monitor Mode**                         | --         | --         | --         | --         | --         | --                     | Supported\ :sup:`1`    | --                     | --                     | --        | --                  | --                  |
   +------------------------------------------+------------+------------+------------+------------+------------+------------------------+------------------------+------------------------+------------------------+-----------+---------------------+---------------------+
   | **Promiscuous Mode**                     | --         | --         | --         | --         | --         | --                     | Experimental\ :sup:`2` | --                     | --                     | --        | --                  | --                  |
   +------------------------------------------+------------+------------+------------+------------+------------+------------------------+------------------------+------------------------+------------------------+-----------+---------------------+---------------------+
   | **STA Mode**                             | --         | --         | --         | --         | --         | Experimental\ :sup:`3` | Supported\ :sup:`1`    | Experimental\ :sup:`4` | Experimental\ :sup:`5` | --        | --                  | --                  |
   +------------------------------------------+------------+------------+------------+------------+------------+------------------------+------------------------+------------------------+------------------------+-----------+---------------------+---------------------+
   | **Scan only (for location accuracy)**    | --         | --         | --         | --         | --         | Experimental\ :sup:`6` | Supported\ :sup:`7`    | --                     | --                     | Supported | Supported\ :sup:`6` | Supported\ :sup:`6` |
   +------------------------------------------+------------+------------+------------+------------+------------+------------------------+------------------------+------------------------+------------------------+-----------+---------------------+---------------------+
   | **SoftAP Mode (for Wi-Fi provisioning)** | --         | --         | --         | --         | --         | --                     | Supported\ :sup:`1`    | --                     | --                     | --        | --                  | --                  |
   +------------------------------------------+------------+------------+------------+------------+------------+------------------------+------------------------+------------------------+------------------------+-----------+---------------------+---------------------+
   | **TX injection Mode**                    | --         | --         | --         | --         | --         | --                     | Supported\ :sup:`1`    | --                     | --                     | --        | --                  | --                  |
   +------------------------------------------+------------+------------+------------+------------+------------+------------------------+------------------------+------------------------+------------------------+-----------+---------------------+---------------------+
   | **Thread Coexistence**                   | --         | --         | --         | --         | --         | --                     | Experimental           | --                     | --                     | --        | --                  | --                  |
   +------------------------------------------+------------+------------+------------+------------+------------+------------------------+------------------------+------------------------+------------------------+-----------+---------------------+---------------------+

  | [1]: Only with nRF7002 DK, nRF7002 DK in nRF7001 emulation mode, nRF7002 EB, nRF7002 EK or nRF7002 EK in nRF7001 emulation mode
  | [2]: Only with nRF7002 DK, nRF7002 DK in nRF7001 emulation mode or nRF7002 EK
  | [3]: Only with nRF7002 EK or nRF7002 EK in nRF7001 emulation mode
  | [4]: Only with nRF700X_nRF54H20DK
  | [5]: Only with nRF700X_nRF54L15PDK
  | [6]: Only with nRF7002 EK, nRF7002 EK in nRF7000 emulation mode or nRF7002 EK in nRF7001 emulation mode
  | [7]: Only with nRF7002 DK, nRF7002 EB, nRF7002 EK, nRF7002 EK in nRF7000 emulation mode or nRF7002 EK in nRF7001 emulation mode

Ecosystem support
*****************

The following sections contain the tables indicating the software maturity levels of the support for the following ecosystems:

* Google Fast Pair

Google Fast Pair
================

The following table indicates the software maturity levels of the support for Google Fast Pair use cases integrated in the |NCS|:

.. toggle::

   .. _software_maturity_fast_pair_use_case:

   .. list-table:: Google Fast Pair use case support
      :header-rows: 1
      :align: center
      :widths: auto

      * - Use case
        - |NCS| sample demonstration
        - nRF52810
        - nRF52811
        - nRF52820
        - nRF52832
        - nRF52833
        - nRF52840
        - nRF5340
        - nRF54H20
        - nRF54L15
        - nRF9151
        - nRF9160
        - nRF9161
      * - **Input device**
        - :ref:`fast_pair_input_device`
        - --
        - --
        - --
        - Experimental
        - Experimental
        - Experimental
        - Experimental
        - --
        - Experimental
        - --
        - --
        - --
      * - **Locator tag**
        - :ref:`fast_pair_locator_tag`
        - --
        - --
        - --
        - Supported
        - Supported
        - Supported
        - Experimental
        - --
        - Experimental
        - --
        - --
        - --

The following table indicates the software maturity levels of the support for each Fast Pair feature:

.. toggle::

   .. _software_maturity_fast_pair_feature:

   .. list-table:: Google Fast Pair feature support
      :header-rows: 1
      :align: center
      :widths: auto

      * -
        - nRF52810
        - nRF52811
        - nRF52820
        - nRF52832
        - nRF52833
        - nRF52840
        - nRF5340
        - nRF54H20
        - nRF54L15
        - nRF9151
        - nRF9160
        - nRF9161
      * - **Initial pairing**
        - --
        - --
        - --
        - Supported
        - Supported
        - Supported
        - Experimental
        - --
        - Experimental
        - --
        - --
        - --
      * - **Subsequent pairing**
        - --
        - --
        - --
        - Experimental
        - Experimental
        - Experimental
        - Experimental
        - --
        - Experimental
        - --
        - --
        - --
      * - **Battery Notification extension**
        - --
        - --
        - --
        - Experimental
        - Experimental
        - Experimental
        - Experimental
        - --
        - Experimental
        - --
        - --
        - --
      * - **Personalized Name extension**
        - --
        - --
        - --
        - Experimental
        - Experimental
        - Experimental
        - Experimental
        - --
        - Experimental
        - --
        - --
        - --
      * - **Find My Device Network extension**
        - --
        - --
        - --
        - Supported
        - Supported
        - Supported
        - Experimental
        - --
        - Experimental
        - --
        - --
        - --

Security Feature Support
************************

The following sections contain the tables indicating the software maturity levels of the support for the following security features:

* Trusted Firmware-M
* PSA Crypto
* |NSIB|
* Hardware Unique Key
* Trusted storage

Trusted Firmware-M support
==========================

.. toggle::

  .. sml-table:: trusted_firmware_m

PSA Crypto support
==================

.. toggle::

  .. sml-table:: psa_crypto
     :insert-values: [("PSA Crypto APIs","nRF54H20","Experimental"), ("PSA Crypto APIs","nRF54L15","Experimental")]

|NSIB|
======

.. toggle::

  .. sml-table:: immutable_bootloader

Hardware Unique Key
===================

.. toggle::

  .. sml-table:: hw_unique_key
     :insert-values: [("Key Derivation from Hardware Unique Key","nRF54L15","Experimental")]

Trusted storage
===============

.. toggle::

  .. sml-table:: trusted_storage
     :insert-values: [("Trusted storage implements the PSA Certified Secure Storage APIs without TF-M","nRF54H20","Experimental"), ("Trusted storage implements the PSA Certified Secure Storage APIs without TF-M","nRF54L15","Experimental")]

Power management device support
*******************************

The following table indicates the software maturity levels of the support for each power management device:

.. toggle::

  .. sml-table:: power_management
