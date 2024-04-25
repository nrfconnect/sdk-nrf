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

+---------------+-----------+-----------+-----------+-----------+--------------+---------------------+---------------------+--------------+-----------------+---------------------+---------------------+
|               | nRF52810  | nRF52811  | nRF52820  | nRF52832  | nRF52833     | nRF52840            | nRF5340             | nRF54        | nRF9151         | nRF9160             | nRF9161             |
+===============+===========+===========+===========+===========+==============+=====================+=====================+==============+=================+=====================+=====================+
| **Bluetooth** | Supported | Supported | Supported | Supported | Supported    | Supported           | Supported           | --           | --              | --                  | --                  |
+---------------+-----------+-----------+-----------+-----------+--------------+---------------------+---------------------+--------------+-----------------+---------------------+---------------------+
| **Bluetooth   | --        | --        | --        | Supported | Supported    | Supported           | Supported           | --           | --              | --                  | --                  |
| Mesh**        |           |           |           |           |              |                     |                     |              |                 |                     |                     |
+---------------+-----------+-----------+-----------+-----------+--------------+---------------------+---------------------+--------------+-----------------+---------------------+---------------------+
| **LTE**       | --        | --        | --        | --        | --           | --                  | --                  | --           | Supported       | Supported           | Supported           |
+---------------+-----------+-----------+-----------+-----------+--------------+---------------------+---------------------+--------------+-----------------+---------------------+---------------------+
| **Matter**    | --        | --        | --        | --        | --           | Supported           | Supported           | --           | --              | --                  | --                  |
+---------------+-----------+-----------+-----------+-----------+--------------+---------------------+---------------------+--------------+-----------------+---------------------+---------------------+
| **Sidewalk**  | --        | --        | --        | --        | --           | Supported           | Supported           | --           | --              | --                  | --                  |
+---------------+-----------+-----------+-----------+-----------+--------------+---------------------+---------------------+--------------+-----------------+---------------------+---------------------+
| **Thread**    | --        | --        | --        | --        | Supported    | Supported           | Supported           | --           | --              | --                  | --                  |
+---------------+-----------+-----------+-----------+-----------+--------------+---------------------+---------------------+--------------+-----------------+---------------------+---------------------+
| **Wi-Fi**     | --        | --        | --        | --        | --           | Supported\ :sup:`1` | Supported\ :sup:`2` | --           | Supported       | Supported\ :sup:`3` | Supported\ :sup:`3` |
+---------------+-----------+-----------+-----------+-----------+--------------+---------------------+---------------------+--------------+-----------------+---------------------+---------------------+
| **Zigbee**    | --        | --        | --        | --        | Supported    | Supported           | Supported           | --           | --              | --                  | --                  |
+---------------+-----------+-----------+-----------+-----------+--------------+---------------------+---------------------+--------------+-----------------+---------------------+---------------------+

| [1]: Only with nRF7002 EK or nRF7002 EK in nRF7001 emulation mode
| [2]: Only with nRF7002 DK, nRF7002 DK in nRF7001 emulation mode, nRF7002 EB, nRF7002 EK, nRF7002 EK in nRF7000 emulation mode or nRF7002 EK in nRF7001 emulation mode
| [3]: Only with nRF7002 EK, nRF7002 EK in nRF7000 emulation mode or nRF7002 EK in nRF7001 emulation mode

Amazon Sidewalk features support
********************************

The following table indicates the software maturity levels of the support for each Amazon Sidewalk feature:

.. toggle::

   +-----------------------------------------+----------+----------+----------+----------+----------+--------------+--------------+------+----------+----------+----------+
   |                                         | nRF52810 | nRF52811 | nRF52820 | nRF52832 | nRF52833 | nRF52840     | nRF5340      | nRF54| nRF9151  | nRF9160  | nRF9161  |
   +=========================================+==========+==========+==========+==========+==========+==============+==============+======+==========+==========+==========+
   | **Sidewalk - OTA DFU over Bluetooth LE**|    --    |    --    |    --    |    --    |    --    | Supported    | Supported    |  --  |    --    |    --    |    --    |
   +-----------------------------------------+----------+----------+----------+----------+----------+--------------+--------------+------+----------+----------+----------+
   | **Sidewalk File Transfer (FUOTA)**      |    --    |    --    |    --    |    --    |    --    | Experimental | Experimental |  --  |    --    |    --    |    --    |
   +-----------------------------------------+----------+----------+----------+----------+----------+--------------+--------------+------+----------+----------+----------+
   | **Sidewalk Multi-link + Auto-connect**  |    --    |    --    |    --    |    --    |    --    | Supported    | Supported    |  --  |    --    |    --    |    --    |
   +-----------------------------------------+----------+----------+----------+----------+----------+--------------+--------------+------+----------+----------+----------+
   | **Sidewalk on-device certification**    |    --    |    --    |    --    |    --    |    --    | Supported    | Supported    |  --  |    --    |    --    |    --    |
   +-----------------------------------------+----------+----------+----------+----------+----------+--------------+--------------+------+----------+----------+----------+
   | **Sidewalk over BLE**                   |    --    |    --    |    --    |    --    |    --    | Supported    | Supported    |  --  |    --    |    --    |    --    |
   +-----------------------------------------+----------+----------+----------+----------+----------+--------------+--------------+------+----------+----------+----------+
   | **Sidewalk over FSK**                   |    --    |    --    |    --    |    --    |    --    | Supported    | Supported    |  --  |    --    |    --    |    --    |
   +-----------------------------------------+----------+----------+----------+----------+----------+--------------+--------------+------+----------+----------+----------+
   | **Sidewalk over LORA**                  |    --    |    --    |    --    |    --    |    --    | Supported    | Supported    |  --  |    --    |    --    |    --    |
   +-----------------------------------------+----------+----------+----------+----------+----------+--------------+--------------+------+----------+----------+----------+

Bluetooth features support
**************************

The following table indicates the software maturity levels of the support for each Bluetooth feature:

.. toggle::

   +----------------------------------------------+-----------+-----------+-----------+-----------+-----------+-----------+-----------+------+-----------+-----------+-----------+
   |                                              | nRF52810  | nRF52811  | nRF52820  | nRF52832  | nRF52833  | nRF52840  | nRF5340   | nRF54| nRF9151   | nRF9160   | nRF9161   |
   +==============================================+===========+===========+===========+===========+===========+===========+===========+======+===========+===========+===========+
   | **Bluetooth LE Peripheral/Central**          | Supported | Supported | Supported | Supported | Supported | Supported | Supported |  --  |    --     |    --     |    --     |
   +----------------------------------------------+-----------+-----------+-----------+-----------+-----------+-----------+-----------+------+-----------+-----------+-----------+
   | **Connectionless/Connected CTE Transmitter** |    --     | Supported | Supported |    --     | Supported |    --     | Supported |  --  |    --     |    --     |    --     |
   +----------------------------------------------+-----------+-----------+-----------+-----------+-----------+-----------+-----------+------+-----------+-----------+-----------+
   | **LE Coded PHY**                             |    --     | Supported | Supported |    --     | Supported | Supported | Supported |  --  |    --     |    --     |    --     |
   +----------------------------------------------+-----------+-----------+-----------+-----------+-----------+-----------+-----------+------+-----------+-----------+-----------+
   | **LLPM**                                     |    --     |    --     |    --     | Supported | Supported | Supported |    --     |  --  |    --     |    --     |    --     |
   +----------------------------------------------+-----------+-----------+-----------+-----------+-----------+-----------+-----------+------+-----------+-----------+-----------+

Thread features support
***********************

The following table indicates the software maturity levels of the support for each Thread feature:

.. toggle::

   +--------------------------------------------+----------+----------+----------+----------+--------------+-------------+-------------+--------+----------+----------+----------+
   |                                            | nRF52810 | nRF52811 | nRF52820 | nRF52832 | nRF52833     | nRF52840    | nRF5340     | nRF54  | nRF9151  | nRF9160  | nRF9161  |
   +============================================+==========+==========+==========+==========+==============+=============+=============+========+==========+==========+==========+
   | **Thread + nRF21540 (GPIO)**               |    --    |    --    |   --     |   --     |   --         | Supported   |  --         | --     |  --      |  --      |  --      |
   +--------------------------------------------+----------+----------+----------+----------+--------------+-------------+-------------+--------+----------+----------+----------+
   | **Thread - Full Thread Device (FTD)**      |    --    |    --    |   --     |   --     | Supported    | Supported   | Supported   | --     |  --      |  --      |  --      |
   +--------------------------------------------+----------+----------+----------+----------+--------------+-------------+-------------+--------+----------+----------+----------+
   | **Thread - Minimal Thread Device (MTD)**   |    --    |    --    |   --     |   --     |    --        | Supported   | Supported   | --     |  --      |  --      |  --      |
   +--------------------------------------------+----------+----------+----------+----------+--------------+-------------+-------------+--------+----------+----------+----------+
   | **Thread 1.1**                             |    --    |    --    |   --     |   --     | Supported    | Supported   | Supported   | --     |  --      |  --      |  --      |
   +--------------------------------------------+----------+----------+----------+----------+--------------+-------------+-------------+--------+----------+----------+----------+
   | **Thread 1.2 - CSL Receiver**              |    --    |    --    |   --     |   --     | Supported    | Supported   | Supported   | --     |  --      |  --      | --       |
   +--------------------------------------------+----------+----------+----------+----------+--------------+-------------+-------------+--------+----------+----------+----------+
   | **Thread 1.2 - Core**                      |    --    |   --     |   --     |   --     | Supported    | Supported   | Supported   | --     |  --      |  --      | --       |
   +--------------------------------------------+----------+----------+----------+----------+--------------+-------------+-------------+--------+----------+----------+----------+
   | **Thread 1.2 - Link Metrics**              |    --    |   --     |   --     |   --     | Supported    | Experimental| Experimental| --     |  --      |  --      |  --      |
   +--------------------------------------------+----------+----------+----------+----------+--------------+-------------+-------------+--------+----------+----------+----------+
   | **Thread 1.3 - Core**                      |    --    |   --     |   --     |   --     | Supported    | Supported   | Supported   | --     |  --      |  --      |  --      |
   +--------------------------------------------+----------+----------+----------+----------+--------------+-------------+-------------+--------+----------+----------+----------+
   | **Thread FTD + Bluetooth LE multiprotocol**|    --    |    --    |   --     |   --     |    --        | Supported   | Supported   | --     |  --      |  --      |  --      |
   +--------------------------------------------+----------+----------+----------+----------+--------------+-------------+-------------+--------+----------+----------+----------+
   | **Thread MTD + Bluetooth LE multiprotocol**|    --    |   --     |   --     |   --     |    --        | Supported   | Supported   | --     |  --      |  --      |   --     |
   +--------------------------------------------+----------+----------+----------+----------+--------------+-------------+-------------+--------+----------+----------+----------+
   | **Thread Radio Co-Processor (RCP)**        |    --    |   --     |   --     |   --     | Supported    | Experimental|  --         | --     |  --      |  --      |  --      |
   +--------------------------------------------+----------+----------+----------+----------+--------------+-------------+-------------+--------+----------+----------+----------+
   | **Thread TCP**                             |    --    |   --     |   --     |   --     |    --        | Experimental| Experimental| --     |  --      |  --      |  --      |
   +--------------------------------------------+----------+----------+----------+----------+--------------+-------------+-------------+--------+----------+----------+----------+

.. _software_maturity_protocol_matter:

Matter features support
***********************

The following table indicates the software maturity levels of the support for each Matter feature:

.. toggle::

   +--------------------------------------------------------------------+----------+----------+----------+----------+----------+----------+---------+-------+---------+---------+---------+
   |                                                                    | nRF52810 | nRF52811 | nRF52820 | nRF52832 | nRF52833 | nRF52840 | nRF5340 | nRF54 | nRF9151 | nRF9160 | nRF9161 |
   +--------------------------------------------------------------------+----------+----------+----------+----------+----------+----------+---------+-------+---------+---------+---------+
   | **Matter - OTA DFU over Bluetooth LE**                             |    --    |    --    |    --    |    --    |    --    |Supported |Supported|  --   |   --    |   --    |   --    |
   +--------------------------------------------------------------------+----------+----------+----------+----------+----------+----------+---------+-------+---------+---------+---------+
   | **Matter Intermittently Connected Device**                         |    --    |    --    |    --    |    --    |    --    |Supported |Supported|  --   |   --    |   --    |   --    |
   +--------------------------------------------------------------------+----------+----------+----------+----------+----------+----------+---------+-------+---------+---------+---------+
   | **Matter commissioning over Bluetooth LE with NFC onboarding**     |    --    |    --    |    --    |    --    |    --    |Supported |Supported|  --   |   --    |   --    |   --    |
   +--------------------------------------------------------------------+----------+----------+----------+----------+----------+----------+---------+-------+---------+---------+---------+
   | **Matter commissioning over Bluetooth LE with QR code onboarding** |    --    |    --    |    --    |    --    |    --    |Supported |Supported|  --   |   --    |   --    |   --    |
   +--------------------------------------------------------------------+----------+----------+----------+----------+----------+----------+---------+-------+---------+---------+---------+
   | **Matter commissioning over IP**                                   |    --    |    --    |    --    |    --    |    --    |Supported |Supported|  --   |   --    |   --    |   --    |
   +--------------------------------------------------------------------+----------+----------+----------+----------+----------+----------+---------+-------+---------+---------+---------+
   | **Matter over Thread**                                             |    --    |    --    |    --    |    --    |    --    |Supported |Supported|  --   |   --    |   --    |   --    |
   +--------------------------------------------------------------------+----------+----------+----------+----------+----------+----------+---------+-------+---------+---------+---------+
   | **Matter over Wi-Fi**                                              |    --    |    --    |    --    |    --    |    --    |    --    |Supported|  --   |   --    |   --    |   --    |
   +--------------------------------------------------------------------+----------+----------+----------+----------+----------+----------+---------+-------+---------+---------+---------+
   | **OTA DFU over Matter**                                            |    --    |    --    |    --    |    --    |    --    |Supported |Supported|  --   |   --    |   --    |   --    |
   +--------------------------------------------------------------------+----------+----------+----------+----------+----------+----------+---------+-------+---------+---------+---------+

Zigbee feature support
**********************

The following table indicates the software maturity levels of the support for each Zigbee feature:

.. toggle::

   +-----------------------------------------+----------+----------+----------+----------+----------+----------+----------+------+----------+----------+----------+
   |                                         | nRF52810 | nRF52811 | nRF52820 | nRF52832 | nRF52833 | nRF52840 | nRF5340  | nRF54| nRF9151  | nRF9160  | nRF9161  |
   +=========================================+==========+==========+==========+==========+==========+==========+==========+======+==========+==========+==========+
   | **OTA DFU over Zigbee**                 |    --    |    --    |    --    |    --    |    --    | Supported| Supported|  --  |    --    |    --    |    --    |
   +-----------------------------------------+----------+----------+----------+----------+----------+----------+----------+------+----------+----------+----------+
   | **Zigbee (Sleepy) End Device**          |    --    |    --    |    --    |    --    | Supported| Supported| Supported|  --  |    --    |    --    |    --    |
   +-----------------------------------------+----------+----------+----------+----------+----------+----------+----------+------+----------+----------+----------+
   | **Zigbee + Bluetooth LE multiprotocol** |    --    |    --    |    --    |    --    | Supported| Supported| Supported|  --  |    --    |    --    |    --    |
   +-----------------------------------------+----------+----------+----------+----------+----------+----------+----------+------+----------+----------+----------+
   | **Zigbee + nRF21540 (GPIO)**            |    --    |    --    |    --    |    --    |          | Supported|    --    |  --  |    --    |    --    |    --    |
   +-----------------------------------------+----------+----------+----------+----------+----------+----------+----------+------+----------+----------+----------+
   | **Zigbee Coordinator**                  |    --    |    --    |    --    |    --    | Supported| Supported| Supported|  --  |    --    |    --    |    --    |
   +-----------------------------------------+----------+----------+----------+----------+----------+----------+----------+------+----------+----------+----------+
   | **Zigbee Network Co-Processor (NCP)**   |    --    |    --    |    --    |    --    | Supported| Supported| Supported|  --  |    --    |    --    |    --    |
   +-----------------------------------------+----------+----------+----------+----------+----------+----------+----------+------+----------+----------+----------+
   | **Zigbee Router**                       |    --    |    --    |    --    |    --    | Supported| Supported| Supported|  --  |    --    |    --    |    --    |
   +-----------------------------------------+----------+----------+----------+----------+----------+----------+----------+------+----------+----------+----------+

Wi-Fi feature support
**********************

The following table indicates the software maturity levels of the support for each Wi-Fi feature:

.. toggle::

  +---------------------------------------+----------+----------+----------+----------+----------+------------------------+-----------------------+------+----------+-------------------+-------------------+
  |                                       | nRF52810 | nRF52811 | nRF52820 | nRF52832 | nRF52833 | nRF52840               | nRF5340               | nRF54| nRF9151  | nRF9160           | nRF9161           |
  +=======================================+==========+==========+==========+==========+==========+========================+=======================+======+==========+===================+===================+
  | **Bluetooth LE Co-existence**         |    --    |    --    |    --    |    --    |    --    |    --                  |    --                 |  --  |    --    |    --             |    --             |
  +---------------------------------------+----------+----------+----------+----------+----------+------------------------+-----------------------+------+----------+-------------------+-------------------+
  | **Monitor Mode**                      |    --    |    --    |    --    |    --    |    --    |    --                  | Experimental\ :sup:`1`|  --  |    --    |    --             |    --             |
  +---------------------------------------+----------+----------+----------+----------+----------+------------------------+-----------------------+------+----------+-------------------+-------------------+
  | **Promiscuous Mode**                  |    --    |    --    |    --    |    --    |    --    |    --                  | Experimental\ :sup:`2`|  --  |    --    |    --             |    --             |
  +---------------------------------------+----------+----------+----------+----------+----------+------------------------+-----------------------+------+----------+-------------------+-------------------+
  | **STA Mode**                          |    --    |    --    |    --    |    --    |    --    | Experimental\ :sup:`3` | Supported\ :sup:`4`   |  --  |    --    |    --             |    --             |
  +---------------------------------------+----------+----------+----------+----------+----------+------------------------+-----------------------+------+----------+-------------------+-------------------+
  | **Scan only (for location accuracy)** |    --    |    --    |    --    |    --    |    --    | Experimental\ :sup:`3` | Supported\ :sup:`5`   |  --  |Supported |Supported\ :sup:`6`|Supported\ :sup:`6`|
  +---------------------------------------+----------+----------+----------+----------+----------+------------------------+-----------------------+------+----------+-------------------+-------------------+
  | **SoftAP Mode**                       |    --    |    --    |    --    |    --    |    --    |    --                  | Experimental\ :sup:`7`|  --  |    --    |    --             |    --             |
  +---------------------------------------+----------+----------+----------+----------+----------+------------------------+-----------------------+------+----------+-------------------+-------------------+
  | **TX injection Mode**                 |    --    |    --    |    --    |    --    |    --    |    --                  | Experimental\ :sup:`1`|  --  |    --    |    --             |    --             |
  +---------------------------------------+----------+----------+----------+----------+----------+------------------------+-----------------------+------+----------+-------------------+-------------------+

  | [1]: Only with nRF7002 DK, nRF7002 DK in nRF7001 emulation mode or nRF7002 EK
  | [2]: Only with nRF7002 DK or nRF7002 DK in nRF7001 emulation mode
  | [3]: Only with nRF7002 EK or nRF7002 EK in nRF7001 emulation mode
  | [4]: Only with nRF7002 DK, nRF7002 DK in nRF7001 emulation mode, nRF7002 EB, nRF7002 EK or nRF7002 EK in nRF7001 emulation mode
  | [5]: Only with nRF7002 DK, nRF7002 EB, nRF7002 EK, nRF7002 EK in nRF7000 emulation mode or nRF7002 EK in nRF7001 emulation mode
  | [6]: Only with nRF7002 EK, nRF7002 EK in nRF7000 emulation mode or nRF7002 EK in nRF7001 emulation mode
  | [7]: Only with nRF7002 DK, nRF7002 DK in nRF7001 emulation mode, nRF7002 EK or nRF7002 EK in nRF7001 emulation mode

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

   +----------------------+----------+----------+----------+----------+----------+----------+--------------+------+-----------+--------------+--------------+
   |                      | nRF52810 | nRF52811 | nRF52820 | nRF52832 | nRF52833 | nRF52840 | nRF5340      | nRF54| nRF9151   | nRF9160      | nRF9161      |
   +======================+==========+==========+==========+==========+==========+==========+==============+======+===========+==============+==============+
   | **Full build**       |    --    |    --    |    --    |    --    |    --    |    --    | Experimental |  --  |    --     | Experimental | Experimental |
   +----------------------+----------+----------+----------+----------+----------+----------+--------------+------+-----------+--------------+--------------+
   | **Minimal Build**    |    --    |    --    |    --    |    --    |    --    |    --    | Supported    |  --  | Supported | Supported    | Supported    |
   +----------------------+----------+----------+----------+----------+----------+----------+--------------+------+-----------+--------------+--------------+

PSA Crypto support
==================

.. toggle::

   +----------------------+----------+----------+----------+----------+----------+----------+--------------+------+-----------+--------------+--------------+
   |                      | nRF52810 | nRF52811 | nRF52820 | nRF52832 | nRF52833 | nRF52840 | nRF5340      | nRF54| nRF9151   | nRF9160      | nRF9161      |
   +======================+==========+==========+==========+==========+==========+==========+==============+======+===========+==============+==============+
   | **PSA Crypto APIs**  |    --    |    --    |    --    |Supported | Supported|Supported |Supported     |  --  | Supported | Supported    | Supported    |
   +----------------------+----------+----------+----------+----------+----------+----------+--------------+------+-----------+--------------+--------------+

|NSIB|
======

.. toggle::

   +--------------------------------------------+----------+----------+----------+----------+----------+----------+--------------+------+-----------+--------------+--------------+
   |                                            | nRF52810 | nRF52811 | nRF52820 | nRF52832 | nRF52833 | nRF52840 | nRF5340      | nRF54| nRF9151   | nRF9160      | nRF9161      |
   +============================================+==========+==========+==========+==========+==========+==========+==============+======+===========+==============+==============+
   | **Immutable Bootloader as part of build**  |    --    |    --    |    --    |Supported | Supported|Supported |Supported     |  --  | Supported | Supported    | Supported    |
   +--------------------------------------------+----------+----------+----------+----------+----------+----------+--------------+------+-----------+--------------+--------------+

Hardware Unique Key
===================

.. toggle::

   +----------------------------------------------+----------+----------+----------+----------+----------+----------+--------------+------+-----------+--------------+--------------+
   |                                              | nRF52810 | nRF52811 | nRF52820 | nRF52832 | nRF52833 | nRF52840 | nRF5340      | nRF54| nRF9151   | nRF9160      | nRF9161      |
   +==============================================+==========+==========+==========+==========+==========+==========+==============+======+===========+==============+==============+
   | **Key Derivation from Hardware Unique Key**  |    --    |    --    |    --    |    --    |    --    |Supported |Supported     |  --  |     --    | Supported    | Supported    |
   +----------------------------------------------+----------+----------+----------+----------+----------+----------+--------------+------+-----------+--------------+--------------+


Trusted storage
===============

.. toggle::

   +----------------------------------------------+----------+----------+----------+----------+-----------+----------+--------------+------+-----------+--------------+--------------+
   |                                              | nRF52810 | nRF52811 | nRF52820 | nRF52832 | nRF52833  | nRF52840 | nRF5340      | nRF54| nRF9151   | nRF9160      | nRF9161      |
   +==============================================+==========+==========+==========+==========+===========+==========+==============+======+===========+==============+==============+
   | **Trusted storage implements the PSA         |    --    |    --    |    --    |    --    | Supported |Supported |Supported     |  --  |     --    | Supported    | Supported    |
   | Certified Secure Storage APIs without TF-M** |          |          |          |          |           |          |              |      |           |              |              |
   +----------------------------------------------+----------+----------+----------+----------+-----------+----------+--------------+------+-----------+--------------+--------------+

Power management device support
*******************************

The following table indicates the software maturity levels of the support for each power management device:

.. toggle::

   +--------------+----------+----------+----------+----------+----------+----------+--------------+------+-----------+--------------+--------------+
   |              | nRF52810 | nRF52811 | nRF52820 | nRF52832 | nRF52833 | nRF52840 | nRF5340      | nRF54| nRF9151   | nRF9160      | nRF9161      |
   +==============+==========+==========+==========+==========+==========+==========+==============+======+===========+==============+==============+
   | **nPM1100**  |    --    |    --    |    --    |    --    |    --    |    --    |Supported     |  --  |     --    |     --       |     --       |
   +--------------+----------+----------+----------+----------+----------+----------+--------------+------+-----------+--------------+--------------+
   | **nPM1300**  |    --    |    --    |    --    |Supported |     --   |Supported |Supported     |  --  | Supported | Supported    |     --       |
   +--------------+----------+----------+----------+----------+----------+----------+--------------+------+-----------+--------------+--------------+
   | **nPM6001**  |    --    |    --    |    --    |    --    |     --   |    --    |    --        |  --  | Supported |    --        |     --       |
   +--------------+----------+----------+----------+----------+----------+----------+--------------+------+-----------+--------------+--------------+
