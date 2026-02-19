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

.. software_maturity_definitions_start

Supported
   The feature or component is implemented and maintained, and is suitable for product development.

Not supported (--)
   The feature or component is neither implemented nor maintained, and it does not work.

Experimental
   The feature can be used for development, but it is not recommended for production.
   This means that the feature is incomplete in functionality or verification and can be expected to change in future releases.
   The feature is made available in its current state, but the design and interfaces can change between release tags.
   The feature is also labeled as :ref:`experimental in Kconfig files <app_build_additions_experimental>` and a build warning is generated to indicate this status.

.. software_maturity_definitions_end

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

          * One BIG, one of the two BIS streams or a mixed stereo comprising of the two (selectable).
          * Audio output: I2S/Analog headset output.
          * Configuration: 16 bit, several bit rates ranging from 32 kbps to 124 kbps.

        - Experimental
      * - :ref:`Unicast client <nrf53_audio_unicast_client_app>`
        - One Connected Isochronous Group (CIG) with two Connected Isochronous Streams (CIS).

          Transmitting unidirectional or transceiving bidirectional audio using CIG and CIS.
        - The following limitations apply:

          * Audio input: USB or I2S (Line in or using Pulse Density Modulation).
          * Audio output: USB or I2S/Analog headset output.
          * Configuration: 16 bit, several bit rates ranging from 32 kbps to 124 kbps.

        - Experimental
      * - :ref:`Unicast server <nrf53_audio_unicast_server_app>`
        - Receiving unidirectional or transceiving bidirectional audio using CIG and CIS.

          Coordinated Set Identification Service (CSIS) is implemented on the server side.
        - The following limitations apply:

          * One CIG, one of the two CIS streams or a mixed stereo comprising of the two (selectable).
          * Audio output: I2S/Analog headset output.
          * Audio input: PDM microphone over I2S.
          * Configuration: 16 bit, several bit rates ranging from 32 kbps to 124 kbps.

        - Experimental

.. _software_maturity_protocol:

Protocol support
****************

The following table indicates the software maturity levels of the support for each :ref:`protocol <protocols>`:

.. tabs::

   .. group-tab:: nRF52 Series

      .. list-table:: Protocol support
         :widths: auto
         :header-rows: 1

         * -
           - nRF52810
           - nRF52811
           - nRF52820
           - nRF52832
           - nRF52833
           - nRF52840
         * - **Bluetooth®**
           - Supported
           - Supported
           - Supported
           - Supported
           - Supported
           - Supported
         * - **Bluetooth Mesh**
           - --
           - --
           - --
           - Supported
           - Supported
           - Supported
         * - **DECT NR+ PHY**
           - --
           - --
           - --
           - --
           - --
           - --
         * - **LTE**
           - --
           - --
           - --
           - --
           - --
           - --
         * - **Matter**
           - --
           - --
           - --
           - --
           - --
           - Supported
         * - **NFC**
           - --
           - --
           - --
           - Supported
           - Supported
           - Supported
         * - **Amazon Sidewalk**
           - --\ :sup:`4`
           - --\ :sup:`4`
           - --\ :sup:`4`
           - --\ :sup:`4`
           - --\ :sup:`4`
           - --\ :sup:`4`
         * - **Thread**
           - --
           - --
           - --
           - --
           - Supported
           - Supported
         * - **Wi-Fi®**
           - --
           - --
           - --
           - --
           - --
           - Supported\ :sup:`1`
         * - **Zigbee**
           - --\ :sup:`5`
           - --\ :sup:`5`
           - --\ :sup:`5`
           - --\ :sup:`5`
           - --\ :sup:`5`
           - --\ :sup:`5`

   .. group-tab:: nRF53 Series

      .. list-table:: Protocol support
         :widths: auto
         :header-rows: 1

         * -
           - nRF5340
         * - **Bluetooth®**
           - Supported
         * - **Bluetooth Mesh**
           - Supported
         * - **DECT NR+ PHY**
           - --
         * - **LTE**
           - --
         * - **Matter**
           - Supported
         * - **NFC**
           - Supported
         * - **Amazon Sidewalk**
           - --\ :sup:`4`
         * - **Thread**
           - Supported
         * - **Wi-Fi®**
           - Supported\ :sup:`2`
         * - **Zigbee**
           - --\ :sup:`5`

   .. group-tab:: nRF54H Series

      .. list-table:: Protocol support
         :widths: auto
         :header-rows: 1

         * -
           - nRF54H20
         * - **Bluetooth®**
           - Supported
         * - **Bluetooth Mesh**
           - --
         * - **DECT NR+ PHY**
           - --
         * - **LTE**
           - --
         * - **Matter**
           - --
         * - **NFC**
           - Experimental
         * - **Amazon Sidewalk**
           - --\ :sup:`4`
         * - **Thread**
           - --
         * - **Wi-Fi®**
           - Experimental\ :sup:`3`
         * - **Zigbee**
           - --\ :sup:`5`

   .. group-tab:: nRF54L Series

      .. list-table:: Protocol support
         :widths: auto
         :header-rows: 1

         * -
           - nRF54L05
           - nRF54L10
           - nRF54L15
           - nRF54LM20A
           - nRF54LV10A
           - nRF54LS05B
         * - **Bluetooth®**
           - Supported
           - Supported
           - Supported
           - Supported
           - Experimental
           - Experimental
         * - **Bluetooth Mesh**
           - Supported
           - Supported
           - Supported
           - Supported
           - --
           - --
         * - **DECT NR+ PHY**
           - --
           - --
           - --
           - --
           - --
           - --
         * - **LTE**
           - --
           - --
           - --
           - --
           - --
           - --
         * - **Matter**
           - --
           - Supported
           - Supported
           - Experimental
           - --
           - --
         * - **NFC**
           - Supported
           - Supported
           - Supported
           - Experimental
           - --
           - --
         * - **Amazon Sidewalk**
           - --\ :sup:`4`
           - --\ :sup:`4`
           - --\ :sup:`4`
           - --\ :sup:`4`
           - --\ :sup:`4`
           - --\ :sup:`4`
         * - **Thread**
           - Supported
           - Supported
           - Supported
           - Experimental
           - --
           - --
         * - **Wi-Fi®**
           - --
           - --
           - Experimental\ :sup:`3`
           - Experimental\ :sup:`3`
           - --
           - --
         * - **Zigbee**
           - --\ :sup:`5`
           - --\ :sup:`5`
           - --\ :sup:`5`
           - --\ :sup:`5`
           - --\ :sup:`5`
           - --\ :sup:`5`

   .. group-tab:: nRF91 Series

         .. list-table:: Protocol support
            :widths: auto
            :header-rows: 1

            * -
              - nRF9131
              - nRF9151
              - nRF9160
              - nRF9161
            * - **Bluetooth®**
              - --
              - --
              - --
              - --
            * - **Bluetooth Mesh**
              - --
              - --
              - --
              - --
            * - **DECT NR+ PHY**
              - Experimental
              - Experimental
              - --
              - Experimental
            * - **LTE**
              - Supported
              - Supported
              - Supported
              - Supported
            * - **Matter**
              - --
              - --
              - --
              - --
            * - **NFC**
              - --
              - --
              - --
              - --
            * - **Amazon Sidewalk**
              - --\ :sup:`4`
              - --\ :sup:`4`
              - --\ :sup:`4`
              - --\ :sup:`4`
            * - **Thread**
              - --
              - --
              - --
              - --
            * - **Wi-Fi®**
              - --
              - Supported\ :sup:`1`
              - Supported\ :sup:`1`
              - Supported\ :sup:`1`
            * - **Zigbee**
              - --\ :sup:`5`
              - --\ :sup:`5`
              - --\ :sup:`5`
              - --\ :sup:`5`

| [1]: Only with nRF7002 EK, nRF7002 EK in nRF7000 emulation mode or nRF7002 EK in nRF7001 emulation mode
| [2]: Only with nRF7002 DK, nRF7002 DK in nRF7001 emulation mode, nRF7002 EB, nRF7002 EK, nRF7002 EK in nRF7000 emulation mode or nRF7002 EK in nRF7001 emulation mode
| [3]: Only with nRF7002-EB II
| [4]: The software maturity levels for Amazon Sidewalk can be found on the `Amazon Sidewalk <Amazon Sidewalk documentation_>`_ add-on page
| [5]: The software maturity levels for Zigbee can be found on the `Zigbee R23`_ add-on page

Amazon Sidewalk features support
********************************

The software maturity levels of the support for each Amazon Sidewalk feature can be found on the `Amazon Sidewalk <Amazon Sidewalk documentation_>`_ add-on page.

Bluetooth features support
**************************

The following table indicates the software maturity levels of the support for each Bluetooth feature:

.. toggle::

  .. tabs::

     .. group-tab:: nRF52 Series

        .. list-table:: Bluetooth features support
           :widths: auto
           :header-rows: 1

           * -
             - nRF52810
             - nRF52811
             - nRF52820
             - nRF52832
             - nRF52833
             - nRF52840
           * - **2 Mbps PHY**
             - Supported
             - Supported
             - Supported
             - Supported
             - Supported
             - Supported
           * - **Coded PHY (Long Range)**
             - --
             - Supported
             - Supported
             - --
             - Supported
             - Supported
           * - **Concurrent Roles**\ :sup:`1`
             - Supported
             - Supported
             - Supported
             - Supported
             - Supported
             - Supported
           * - **Data Length Extensions**
             - Supported
             - Supported
             - Supported
             - Supported
             - Supported
             - Supported
           * - **Advertising Extensions**
             - Supported
             - Supported
             - Supported
             - Supported
             - Supported
             - Supported
           * - **Periodic Advertising with Responses**
             - Supported
             - Supported
             - Supported
             - Supported
             - Supported
             - Supported
           * - **Periodic Advertising Sync Transfer**
             - Supported
             - Supported
             - Supported
             - Supported
             - Supported
             - Supported
           * - **Isochronous Channels**
             - Supported\ :sup:`2`
             - Supported\ :sup:`2`
             - Supported
             - Supported\ :sup:`2`
             - Supported
             - Supported\ :sup:`2`
           * - **Direction Finding**\ :sup:`3`
             - --
             - Supported
             - Supported
             - --
             - Supported
             - --
           * - **LE Power Control**
             - Supported
             - Supported
             - Supported
             - Supported
             - Supported
             - Supported
           * - **Connection Subrating**
             - Supported
             - Supported
             - Supported
             - Supported
             - Supported
             - Supported
           * - **Channel Sounding**
             - --
             - --
             - --
             - --
             - --
             - --
           * - **GATT Database Hash**
             - Supported
             - Supported
             - Supported
             - Supported
             - Supported
             - Supported
           * - **Enhanced ATT**
             - Supported
             - Supported
             - Supported
             - Supported
             - Supported
             - Supported
           * - **L2CAP Connection Oriented Channels**
             - Supported
             - Supported
             - Supported
             - Supported
             - Supported
             - Supported
           * - **Shorter Connection Intervals**
             - Supported
             - Supported
             - Supported
             - Supported
             - Supported
             - Supported
           * - **Frame Space Update**
             - Supported
             - Supported
             - Supported
             - Supported
             - Supported
             - Supported
           * - **Extended Feature Set**
             - Supported
             - Supported
             - Supported
             - Supported
             - Supported
             - Supported

     .. group-tab:: nRF53 Series

        .. list-table:: Bluetooth features support
           :widths: auto
           :header-rows: 1

           * -
             - nRF5340
           * - **2 Mbps PHY**
             - Supported
           * - **Coded PHY (Long Range)**
             - Supported
           * - **Concurrent Roles**\ :sup:`1`
             - Supported
           * - **Data Length Extensions**
             - Supported
           * - **Advertising Extensions**
             - Supported
           * - **Periodic Advertising with Responses**
             - Supported
           * - **Periodic Advertising Sync Transfer**
             - Supported
           * - **Isochronous Channels**
             - Supported
           * - **Direction Finding**\ :sup:`3`
             - Supported
           * - **LE Power Control**
             - Supported
           * - **Connection Subrating**
             - Supported
           * - **Channel Sounding**
             - --
           * - **GATT Database Hash**
             - Supported
           * - **Enhanced ATT**
             - Supported
           * - **L2CAP Connection Oriented Channels**
             - Supported
           * - **Shorter Connection Intervals**
             - Supported
           * - **Frame Space Update**
             - Supported
           * - **Extended Feature Set**
             - Supported

     .. group-tab:: nRF54H Series

        .. list-table:: Bluetooth features support
           :widths: auto
           :header-rows: 1

           * -
             - nRF54H20
           * - **2 Mbps PHY**
             - Supported
           * - **Coded PHY (Long Range)**
             - Supported
           * - **Concurrent Roles**\ :sup:`1`
             - Supported
           * - **Data Length Extensions**
             - Supported
           * - **Advertising Extensions**
             - Supported
           * - **Periodic Advertising with Responses**
             - Supported
           * - **Periodic Advertising Sync Transfer**
             - Supported
           * - **Isochronous Channels**
             - Supported
           * - **LE Power Control**
             - Supported
           * - **Connection Subrating**
             - Supported
           * - **Channel Sounding**
             - Experimental
           * - **GATT Database Hash**
             - Supported
           * - **Enhanced ATT**
             - Supported
           * - **L2CAP Connection Oriented Channels**
             - Supported
           * - **Shorter Connection Intervals**
             - Supported
           * - **Frame Space Update**
             - Supported
           * - **Extended Feature Set**
             - Supported

     .. group-tab:: nRF54L Series

        .. list-table:: Bluetooth features support
           :widths: auto
           :header-rows: 1

           * -
             - nRF54L05
             - nRF54L10
             - nRF54L15
             - nRF54LM20A
             - nRF54LV10A
             - nRF54LS05B
           * - **2 Mbps PHY**
             - Supported
             - Supported
             - Supported
             - Supported
             - Experimental
             - Experimental
           * - **Coded PHY (Long Range)**
             - Supported
             - Supported
             - Supported
             - Supported
             - Experimental
             - Experimental
           * - **Concurrent Roles**\ :sup:`1`
             - Supported
             - Supported
             - Supported
             - Supported
             - Experimental
             - Experimental
           * - **Data Length Extensions**
             - Supported
             - Supported
             - Supported
             - Supported
             - Experimental
             - Experimental
           * - **Advertising Extensions**
             - Supported
             - Supported
             - Supported
             - Supported
             - Experimental
             - Experimental
           * - **Periodic Advertising with Responses**
             - Supported
             - Supported
             - Supported
             - Supported
             - Experimental
             - Experimental
           * - **Periodic Advertising Sync Transfer**
             - Supported
             - Supported
             - Supported
             - Supported
             - Experimental
             - Experimental
           * - **Isochronous Channels**
             - Supported
             - Supported
             - Supported
             - Supported
             - Experimental
             - Experimental
           * - **Direction Finding**\ :sup:`3`
             - Experimental
             - Experimental
             - Experimental
             - Experimental
             - Experimental
             - Experimental
           * - **LE Power Control**
             - Supported
             - Supported
             - Supported
             - Supported
             - Experimental
             - Experimental
           * - **Connection Subrating**
             - Supported
             - Supported
             - Supported
             - Supported
             - Experimental
             - Experimental
           * - **Channel Sounding**
             - Supported
             - Supported
             - Supported
             - Supported
             - Experimental
             - Experimental
           * - **GATT Database Hash**
             - Supported
             - Supported
             - Supported
             - Supported
             - Experimental
             - Experimental
           * - **Enhanced ATT**
             - Supported
             - Supported
             - Supported
             - Supported
             - Experimental
             - Experimental
           * - **L2CAP Connection Oriented Channels**
             - Supported
             - Supported
             - Supported
             - Supported
             - Experimental
             - Experimental
           * - **Shorter Connection Intervals**
             - Supported
             - Supported
             - Supported
             - Supported
             - Experimental
             - Experimental
           * - **Frame Space Update**
             - Supported
             - Supported
             - Supported
             - Supported
             - Experimental
             - Experimental
           * - **Extended Feature Set**
             - Supported
             - Supported
             - Supported
             - Supported
             - Experimental
             - Experimental

  | [1]: Subject to RAM availability
  | [2]: Do not support encrypting and decrypting the Isochronous Channels packets
  | [3]: Only AoA transmitter is supported


The following table indicates the software maturity levels of the support for each proprietary Bluetooth feature:

.. toggle::

   .. tabs::

      .. tab:: nRF52 Series

         .. list-table:: Proprietary Bluetooth features support
            :widths: auto
            :header-rows: 1

            * -
              - nRF52810
              - nRF52811
              - nRF52820
              - nRF52832
              - nRF52833
              - nRF52840
            * - **Low Latency Packet Mode**
              - Supported
              - Supported
              - Supported
              - Supported
              - Supported
              - Supported
            * - **Multi-protocol Support**
              - Supported
              - Supported
              - Supported
              - Supported
              - Supported
              - Supported
            * - **QoS Conn Event Reports**
              - Supported
              - Supported
              - Supported
              - Supported
              - Supported
              - Supported
            * - **QoS Channel Survey**
              - Supported
              - Supported
              - Supported
              - Supported
              - Supported
              - Supported
            * - **Radio Coexistence**
              - Supported
              - Supported
              - Supported
              - Supported
              - Supported
              - Supported

      .. tab:: nRF53 Series

         .. list-table:: Proprietary Bluetooth features support
            :widths: auto
            :header-rows: 1

            * -
              - nRF5340
            * - **Low Latency Packet Mode**
              - --
            * - **Multi-protocol Support**
              - Supported
            * - **QoS Conn Event Reports**
              - Supported
            * - **QoS Channel Survey**
              - Supported
            * - **Radio Coexistence**
              - Supported

      .. tab:: nRF54H Series

         .. list-table:: Proprietary Bluetooth features support
            :widths: auto
            :header-rows: 1

            * -
              - nRF54H20
            * - **Low Latency Packet Mode**
              - Supported
            * - **Multi-protocol Support**
              - Supported
            * - **QoS Conn Event Reports**
              - Supported
            * - **QoS Channel Survey**
              - Supported
            * - **Radio Coexistence**
              - Supported

      .. tab:: nRF54L Series

         .. list-table:: Proprietary Bluetooth features support
            :widths: auto
            :header-rows: 1

            * -
              - nRF54L05
              - nRF54L10
              - nRF54L15
              - nRF54LM20A
              - nRF54LV10A
              - nRF54LS05B
            * - **Low Latency Packet Mode**
              - Supported
              - Supported
              - Supported
              - Supported
              - Experimental
              - Experimental
            * - **Multi-protocol Support**
              - Supported
              - Supported
              - Supported
              - Supported
              - Experimental
              - Experimental
            * - **QoS Conn Event Reports**
              - Supported
              - Supported
              - Supported
              - Supported
              - Experimental
              - Experimental
            * - **QoS Channel Survey**
              - Supported
              - Supported
              - Supported
              - Supported
              - Experimental
              - Experimental
            * - **Radio Coexistence**
              - Supported
              - Supported
              - Supported
              - Supported
              - Experimental
              - Experimental

Thread features support
***********************

The following table indicates the software maturity levels of the support for each Thread feature:

.. toggle::

   .. tabs::

      .. tab:: nRF52 Series

         .. list-table:: Thread features support
            :widths: auto
            :header-rows: 1

            * - Feature
              - nRF52810
              - nRF52811
              - nRF52820
              - nRF52832
              - nRF52833
              - nRF52840
            * - **Thread - Full Thread Device (FTD)**
              - --
              - --
              - --
              - --
              - Supported
              - Supported
            * - **Thread - Minimal Thread Device (MTD)**
              - --
              - --
              - --
              - --
              - --
              - Supported
            * - **Thread 1.1**
              - --
              - --
              - --
              - --
              - Supported
              - Supported
            * - **Thread 1.2 - CSL Receiver**
              - --
              - --
              - --
              - --
              - Supported
              - Supported
            * - **Thread 1.2 - Core**
              - --
              - --
              - --
              - --
              - Supported
              - Supported
            * - **Thread 1.2 - Link Metrics**
              - --
              - --
              - --
              - --
              - Supported
              - Supported
            * - **Thread 1.3 - Core**
              - --
              - --
              - --
              - --
              - Supported
              - Supported
            * - **Thread 1.4 - Core**
              - --
              - --
              - --
              - --
              - Supported
              - Supported
            * - **Thread FTD + Bluetooth LE multiprotocol**
              - --
              - --
              - --
              - --
              - --
              - Supported
            * - **Thread MTD + Bluetooth LE multiprotocol**
              - --
              - --
              - --
              - --
              - --
              - Supported
            * - **Thread Radio Co-Processor (RCP)**
              - --
              - --
              - --
              - --
              - Supported
              - Supported
            * - **Thread TCP**
              - --
              - --
              - --
              - --
              - --
              - Supported

      .. tab:: nRF53 Series

         .. list-table:: Thread features support
            :widths: auto
            :header-rows: 1

            * - Feature
              - nRF5340
            * - **Thread - Full Thread Device (FTD)**
              - Supported
            * - **Thread - Minimal Thread Device (MTD)**
              - Supported
            * - **Thread 1.1**
              - Supported
            * - **Thread 1.2 - CSL Receiver**
              - Supported
            * - **Thread 1.2 - Core**
              - Supported
            * - **Thread 1.2 - Link Metrics**
              - Supported
            * - **Thread 1.3 - Core**
              - Supported
            * - **Thread 1.4 - Core**
              - Supported
            * - **Thread FTD + Bluetooth LE multiprotocol**
              - Supported
            * - **Thread MTD + Bluetooth LE multiprotocol**
              - Supported
            * - **Thread Radio Co-Processor (RCP)**
              - --
            * - **Thread TCP**
              - Supported

      .. tab:: nRF54H Series

         .. list-table:: Thread features support
            :widths: auto
            :header-rows: 1

            * - Feature
              - nRF54H20
            * - **Thread - Full Thread Device (FTD)**
              - --
            * - **Thread - Minimal Thread Device (MTD)**
              - --
            * - **Thread 1.1**
              - --
            * - **Thread 1.2 - CSL Receiver**
              - --
            * - **Thread 1.2 - Core**
              - --
            * - **Thread 1.2 - Link Metrics**
              - --
            * - **Thread 1.3 - Core**
              - --
            * - **Thread 1.4 - Core**
              - --
            * - **Thread FTD + Bluetooth LE multiprotocol**
              - --
            * - **Thread MTD + Bluetooth LE multiprotocol**
              - --
            * - **Thread Radio Co-Processor (RCP)**
              - --
            * - **Thread TCP**
              - --

      .. tab:: nRF54L Series

         .. list-table:: Thread features support
            :widths: auto
            :header-rows: 1

            * - Feature
              - nRF54L05
              - nRF54L10
              - nRF54L15
              - nRF54LM20A
              - nRF54LV10A
            * - **Thread - Full Thread Device (FTD)**
              - --
              - Supported
              - Supported
              - Experimental
              - --
            * - **Thread - Minimal Thread Device (MTD)**
              - --
              - Supported
              - Supported
              - Experimental
              - --
            * - **Thread 1.1**
              - --
              - Supported
              - Supported
              - Experimental
              - --
            * - **Thread 1.2 - CSL Receiver**
              - --
              - Supported
              - Supported
              - Experimental
              - --
            * - **Thread 1.2 - Core**
              - --
              - Supported
              - Supported
              - Experimental
              - --
            * - **Thread 1.2 - Link Metrics**
              - --
              - Supported
              - Supported
              - Experimental
              - --
            * - **Thread 1.3 - Core**
              - --
              - Supported
              - Supported
              - Experimental
              - --
            * - **Thread 1.4 - Core**
              - --
              - Supported
              - Supported
              - Experimental
              - --
            * - **Thread FTD + Bluetooth LE multiprotocol**
              - --
              - Supported
              - Supported
              - Experimental
              - --
            * - **Thread MTD + Bluetooth LE multiprotocol**
              - --
              - Supported
              - Supported
              - Experimental
              - --
            * - **Thread Radio Co-Processor (RCP)**
              - Supported
              - Supported
              - Supported
              - Experimental
              - --
            * - **Thread TCP**
              - --
              - Supported
              - Supported
              - Experimental
              - --

      .. tab:: nRF91 Series

         .. list-table:: Thread features support
            :widths: auto
            :header-rows: 1

            * - Feature
              - nRF9131
              - nRF9151
              - nRF9160
              - nRF9161
            * - **Thread - Full Thread Device (FTD)**
              - --
              - --
              - --
              - --
            * - **Thread - Minimal Thread Device (MTD)**
              - --
              - --
              - --
              - --
            * - **Thread 1.1**
              - --
              - --
              - --
              - --
            * - **Thread 1.2 - CSL Receiver**
              - --
              - --
              - --
              - --
            * - **Thread 1.2 - Core**
              - --
              - --
              - --
              - --
            * - **Thread 1.2 - Link Metrics**
              - --
              - --
              - --
              - --
            * - **Thread 1.3 - Core**
              - --
              - --
              - --
              - --
            * - **Thread 1.4 - Core**
              - --
              - --
              - --
              - --
            * - **Thread FTD + Bluetooth LE multiprotocol**
              - --
              - --
              - --
              - --
            * - **Thread MTD + Bluetooth LE multiprotocol**
              - --
              - --
              - --
              - --
            * - **Thread Radio Co-Processor (RCP)**
              - --
              - --
              - --
              - --
            * - **Thread TCP**
              - --
              - --
              - --
              - --

.. _software_maturity_protocol_matter:

Matter features support
***********************

.. include:: /includes/matter/wifi_nrf5340_deprecation.txt

The following table indicates the software maturity levels of the support for each Matter feature:

.. toggle::

   .. tabs::

      .. tab:: nRF52 Series
         .. list-table:: Matter features support
            :widths: auto
            :header-rows: 1

            * - Feature
              - nRF52810
              - nRF52811
              - nRF52820
              - nRF52832
              - nRF52833
              - nRF52840
            * - **Matter - OTA DFU over Bluetooth LE**
              - --
              - --
              - --
              - --
              - --
              - Supported
            * - **Matter Intermittently Connected Device**
              - --
              - --
              - --
              - --
              - --
              - Supported
            * - **Matter commissioning over Bluetooth LE with NFC onboarding**
              - --
              - --
              - --
              - --
              - --
              - Supported
            * - **Matter commissioning over Bluetooth LE with QR code onboarding**
              - --
              - --
              - --
              - --
              - --
              - Supported
            * - **Matter commissioning over IP**
              - --
              - --
              - --
              - --
              - --
              - Supported
            * - **Matter over Thread**
              - --
              - --
              - --
              - --
              - --
              - Supported
            * - **Matter over Wi-Fi**
              - --
              - --
              - --
              - --
              - --
              - --
            * - **OTA DFU over Matter**
              - --
              - --
              - --
              - --
              - --
              - Supported

      .. tab:: nRF53 Series
         .. list-table:: Matter features support
            :widths: auto
            :header-rows: 1

            * - Feature
              - nRF5340
            * - **Matter - OTA DFU over Bluetooth LE**
              - Supported
            * - **Matter Intermittently Connected Device**
              - Supported
            * - **Matter commissioning over Bluetooth LE with NFC onboarding**
              - Supported
            * - **Matter commissioning over Bluetooth LE with QR code onboarding**
              - Supported
            * - **Matter commissioning over IP**
              - Supported
            * - **Matter over Thread**
              - Supported
            * - **Matter over Wi-Fi**
              - Supported
            * - **OTA DFU over Matter**
              - Supported

      .. tab:: nRF54H Series
         .. list-table:: Matter features support
            :widths: auto
            :header-rows: 1

            * - Feature
              - nRF54H20
            * - **Matter - OTA DFU over Bluetooth LE**
              - --
            * - **Matter Intermittently Connected Device**
              - --
            * - **Matter commissioning over Bluetooth LE with NFC onboarding**
              - --
            * - **Matter commissioning over Bluetooth LE with QR code onboarding**
              - --
            * - **Matter commissioning over IP**
              - --
            * - **Matter over Thread**
              - --
            * - **Matter over Wi-Fi**
              - --
            * - **OTA DFU over Matter**
              - --

      .. tab:: nRF54L Series
         .. list-table:: Matter features support
            :widths: auto
            :header-rows: 1

            * - Feature
              - nRF54L05
              - nRF54L10
              - nRF54L15
              - nRF54LM20A
              - nRF54LV10A
            * - **Matter - OTA DFU over Bluetooth LE**
              - --
              - Supported
              - Supported
              - Experimental
              - --
            * - **Matter Intermittently Connected Device**
              - --
              - Supported
              - Supported
              - Experimental
              - --
            * - **Matter commissioning over Bluetooth LE with NFC onboarding**
              - --
              - Supported
              - Supported
              - Experimental
              - --
            * - **Matter commissioning over Bluetooth LE with QR code onboarding**
              - --
              - Supported
              - Supported
              - Experimental
              - --
            * - **Matter commissioning over IP**
              - --
              - Supported
              - Supported
              - Experimental
              - --
            * - **Matter over Thread**
              - --
              - Supported
              - Supported
              - Experimental
              - --
            * - **Matter over Wi-Fi**
              - --
              - --
              - --
              - Experimental
              - --
            * - **OTA DFU over Matter**
              - --
              - Supported
              - Supported
              - Experimental
              - --

      .. tab:: nRF91 Series
         .. list-table:: Matter features support
            :widths: auto
            :header-rows: 1

            * - Feature
              - nRF9131
              - nRF9151
              - nRF9160
              - nRF9161
            * - **Matter - OTA DFU over Bluetooth LE**
              - --
              - --
              - --
              - --
            * - **Matter Intermittently Connected Device**
              - --
              - --
              - --
              - --
            * - **Matter commissioning over Bluetooth LE with NFC onboarding**
              - --
              - --
              - --
              - --
            * - **Matter commissioning over Bluetooth LE with QR code onboarding**
              - --
              - --
              - --
              - --
            * - **Matter commissioning over IP**
              - --
              - --
              - --
              - --
            * - **Matter over Thread**
              - --
              - --
              - --
              - --
            * - **Matter over Wi-Fi**
              - --
              - --
              - --
              - --
            * - **OTA DFU over Matter**
              - --
              - --
              - --
              - --

NFC features support
********************

The following table indicates the software maturity levels of the support for each NFC feature:

.. toggle::

   .. tabs::

      .. group-tab:: nRF52 Series

         .. list-table:: NFC features support
            :widths: auto
            :header-rows: 1

            * -
              - nRF52810
              - nRF52811
              - nRF52820
              - nRF52832
              - nRF52833
              - nRF52840
            * - **NFC Type 2 Tag (read-only)**
              - --
              - --
              - --
              - Supported
              - Supported
              - Supported
            * - **NFC Type 4 Tag (read/write)**
              - --
              - --
              - --
              - Supported
              - Supported
              - Supported
            * - **NFC Reader/Writer (polling device)**
              - --
              - --
              - --
              - Supported
              - Supported
              - Supported
            * - **NFC ISO-DEP protocol (ISO/IEC 14443-4)**
              - --
              - --
              - --
              - Supported
              - Supported
              - Supported
            * - **NDEF encoding/decoding**
              - --
              - --
              - --
              - Supported
              - Supported
              - Supported
            * - **NFC Record Type Definitions: URI, Text, Connection Handover**
              - --
              - --
              - --
              - Supported
              - Supported
              - Supported
            * - **NFC Connection Handover to Bluetooth carrier, Static and Negotiated Handover**
              - --
              - --
              - --
              - Supported
              - Supported
              - Supported
            * - **NFC Tag NDEF Exchange Protocol (TNEP)**
              - --
              - --
              - --
              - Supported
              - Supported
              - Supported

      .. group-tab:: nRF53 Series

         .. list-table:: NFC features support
            :widths: auto
            :header-rows: 1

            * -
              - nRF5340
            * - **NFC Type 2 Tag (read-only)**
              - Supported
            * - **NFC Type 4 Tag (read/write)**
              - Supported
            * - **NFC Reader/Writer (polling device)**
              - Supported
            * - **NFC ISO-DEP protocol (ISO/IEC 14443-4)**
              - Supported
            * - **NDEF encoding/decoding**
              - Supported
            * - **NFC Record Type Definitions: URI, Text, Connection Handover**
              - Supported
            * - **NFC Connection Handover to Bluetooth carrier, Static and Negotiated Handover**
              - Supported
            * - **NFC Tag NDEF Exchange Protocol (TNEP)**
              - Supported

      .. group-tab:: nRF54H Series

         .. list-table:: NFC features support
            :widths: auto
            :header-rows: 1

            * -
              - nRF54H20
            * - **NFC Type 2 Tag (read-only)**
              - Experimental
            * - **NFC Type 4 Tag (read/write)**
              - Experimental
            * - **NFC Reader/Writer (polling device)**
              - --
            * - **NFC ISO-DEP protocol (ISO/IEC 14443-4)**
              - Experimental
            * - **NDEF encoding/decoding**
              - Experimental
            * - **NFC Record Type Definitions: URI, Text, Connection Handover**
              - Experimental
            * - **NFC Connection Handover to Bluetooth carrier, Static and Negotiated Handover**
              - Experimental
            * - **NFC Tag NDEF Exchange Protocol (TNEP)**
              - Experimental\ :sup:`1`

      .. group-tab:: nRF54L Series

         .. list-table:: NFC features support
            :widths: auto
            :header-rows: 1

            * -
              - nRF54L05
              - nRF54L10
              - nRF54L15
              - nRF54LM20A
              - nRF54LV10A
            * - **NFC Type 2 Tag (read-only)**
              - Supported
              - Supported
              - Supported
              - Experimental
              - --
            * - **NFC Type 4 Tag (read/write)**
              - Supported
              - Supported
              - Supported
              - Experimental
              - --
            * - **NFC Reader/Writer (polling device)**
              - --
              - --
              - --
              - --
              - --
            * - **NFC ISO-DEP protocol (ISO/IEC 14443-4)**
              - Supported
              - Supported
              - Supported
              - Experimental
              - --
            * - **NDEF encoding/decoding**
              - Supported
              - Supported
              - Supported
              - Experimental
              - --
            * - **NFC Record Type Definitions: URI, Text, Connection Handover**
              - Supported
              - Supported
              - Supported
              - Experimental
              - --
            * - **NFC Connection Handover to Bluetooth carrier, Static and Negotiated Handover**
              - Supported
              - Supported
              - Supported
              - Experimental
              - --
            * - **NFC Tag NDEF Exchange Protocol (TNEP)**
              - Supported\ :sup:`1`
              - Supported\ :sup:`1`
              - Supported\ :sup:`1`
              - Experimental\ :sup:`1`
              - --

      .. group-tab:: nRF91 Series

         .. list-table:: NFC features support
            :widths: auto
            :header-rows: 1

            * -
              - nRF9131
              - nRF9151
              - nRF9160
              - nRF9161
            * - **NFC Type 2 Tag (read-only)**
              - --
              - --
              - --
              - --
            * - **NFC Type 4 Tag (read/write)**
              - --
              - --
              - --
              - --
            * - **NFC Reader/Writer (polling device)**
              - --
              - --
              - --
              - --
            * - **NFC ISO-DEP protocol (ISO/IEC 14443-4)**
              - --
              - --
              - --
              - --
            * - **NDEF encoding/decoding**
              - --
              - --
              - --
              - --
            * - **NFC Record Type Definitions: URI, Text, Connection Handover**
              - --
              - --
              - --
              - --
            * - **NFC Connection Handover to Bluetooth carrier, Static and Negotiated Handover**
              - --
              - --
              - --
              - --
            * - **NFC Tag NDEF Exchange Protocol (TNEP)**
              - --
              - --
              - --
              - --

  | [1]: Only supported on the NFC Tag device

Zigbee feature support
**********************

.. include:: /includes/zigbee_deprecation.txt

The software maturity levels of the support for each Zigbee feature can be found on the `Zigbee R23`_ add-on page.

Wi-Fi feature support
*********************

The following table indicates the software maturity levels of the support for each Wi-Fi feature:

.. toggle::

   .. tabs::

      .. tab:: nRF52 Series
         .. list-table:: Wi-Fi feature support
            :header-rows: 1

            * - Feature
              - nRF52810
              - nRF52811
              - nRF52820
              - nRF52832
              - nRF52833
              - nRF52840
            * - **Bluetooth LE Coexistence**
              - --
              - --
              - --
              - --
              - --
              - Experimental
            * - **Monitor Mode**
              - --
              - --
              - --
              - --
              - --
              - --
            * - **Promiscuous Mode**
              - --
              - --
              - --
              - --
              - --
              - --
            * - **STA Mode**
              - --
              - --
              - --
              - --
              - --
              - Experimental\ :sup:`3`
            * - **Scan only (for location accuracy)**
              - --
              - --
              - --
              - --
              - --
              - Experimental\ :sup:`5`
            * - **SoftAP Mode (for Wi-Fi provisioning)**
              - --
              - --
              - --
              - --
              - --
              - --
            * - **TX injection Mode**
              - --
              - --
              - --
              - --
              - --
              - --
            * - **Thread Coexistence**
              - --
              - --
              - --
              - --
              - --
              - --

      .. tab:: nRF53 Series
         .. list-table:: Wi-Fi feature support
            :widths: auto
            :header-rows: 1

            * - Feature
              - nRF5340
            * - **Bluetooth LE Coexistence**
              - Supported\ :sup:`1`
            * - **Monitor Mode**
              - Supported\ :sup:`1`
            * - **Promiscuous Mode**
              - Supported\ :sup:`2`
            * - **STA Mode**
              - Supported\ :sup:`1`
            * - **Scan only (for location accuracy)**
              - Supported\ :sup:`6`
            * - **SoftAP Mode (for Wi-Fi provisioning)**
              - Supported\ :sup:`1`
            * - **TX injection Mode**
              - Supported\ :sup:`1`
            * - **Thread Coexistence**
              - Experimental

      .. tab:: nRF54H Series
         .. list-table:: Wi-Fi feature support
            :widths: auto
            :header-rows: 1

            * - Feature
              - nRF54H20
            * - **Bluetooth LE Coexistence**
              - --
            * - **Monitor Mode**
              - --
            * - **Promiscuous Mode**
              - --
            * - **STA Mode**
              - Experimental\ :sup:`4`
            * - **Scan only (for location accuracy)**
              - --
            * - **SoftAP Mode (for Wi-Fi provisioning)**
              - --
            * - **TX injection Mode**
              - --
            * - **Thread Coexistence**
              - --

      .. tab:: nRF54L Series
         .. list-table:: Wi-Fi feature support
            :widths: auto
            :header-rows: 1

            * - Feature
              - nRF54L05
              - nRF54L10
              - nRF54L15
              - nRF54LM20A
              - nRF54LV10A
            * - **Bluetooth LE Coexistence**
              - --
              - --
              - --
              - Experimental\ :sup:`4`
              - --
            * - **Monitor Mode**
              - --
              - --
              - Experimental\ :sup:`4`
              - Experimental\ :sup:`4`
              - --
            * - **Promiscuous Mode**
              - --
              - --
              - Experimental\ :sup:`4`
              - Experimental\ :sup:`4`
              - --
            * - **STA Mode**
              - --
              - --
              - Experimental\ :sup:`4`
              - Experimental\ :sup:`4`
              - --
            * - **Scan only (for location accuracy)**
              - --
              - --
              - --
              - --
              - --
            * - **SoftAP Mode (for Wi-Fi provisioning)**
              - --
              - --
              - Experimental\ :sup:`4`
              - Experimental\ :sup:`4`
              - --
            * - **TX injection Mode**
              - --
              - --
              - Experimental\ :sup:`4`
              - Experimental\ :sup:`4`
              - --
            * - **Thread Coexistence**
              - --
              - --
              - --
              - --
              - --

      .. tab:: nRF91 Series
         .. list-table:: Wi-Fi feature support
            :widths: auto
            :header-rows: 1

            * - Feature
              - nRF9131
              - nRF9151
              - nRF9160
              - nRF9161
            * - **Bluetooth LE Coexistence**
              - --
              - --
              - --
              - --
            * - **Monitor Mode**
              - --
              - --
              - --
              - --
            * - **Promiscuous Mode**
              - --
              - --
              - --
              - --
            * - **STA Mode**
              - --
              - --
              - --
              - --
            * - **Scan only (for location accuracy)**
              - --
              - Supported\ :sup:`5`
              - Supported\ :sup:`5`
              - Supported\ :sup:`5`
            * - **SoftAP Mode (for Wi-Fi provisioning)**
              - --
              - --
              - --
              - --
            * - **TX injection Mode**
              - --
              - --
              - --
              - --
            * - **Thread Coexistence**
              - --
              - --
              - --
              - --

   | [1]: Only with nRF7002 DK, nRF7002 DK in nRF7001 emulation mode, nRF7002 EB, nRF7002 EK or nRF7002 EK in nRF7001 emulation mode
   | [2]: Only with nRF7002 DK, nRF7002 DK in nRF7001 emulation mode or nRF7002 EK
   | [3]: Only with nRF7002 EK or nRF7002 EK in nRF7001 emulation mode
   | [4]: Only with nRF7002-EB II
   | [5]: Only with nRF7002 EK, nRF7002 EK in nRF7000 emulation mode or nRF7002 EK in nRF7001 emulation mode
   | [6]: Only with nRF7002 DK, nRF7002 EB, nRF7002 EK, nRF7002 EK in nRF7000 emulation mode or nRF7002 EK in nRF7001 emulation mode

Ecosystem support
*****************

The following sections contain the tables indicating the software maturity levels of the support for the following ecosystems:

* Google Fast Pair

.. _software_maturity_fast_pair:

Google Fast Pair
================

The following table indicates the software maturity levels of the support for Google Fast Pair use cases integrated in the |NCS|:

.. toggle::

   .. _software_maturity_fast_pair_use_case:

   .. tabs::

      .. tab:: nRF52 Series

         .. list-table:: Google Fast Pair use case support
            :header-rows: 1
            :widths: auto

            * - Use case
              - |NCS| sample demonstration
              - nRF52810
              - nRF52811
              - nRF52820
              - nRF52832
              - nRF52833
              - nRF52840
            * - **Input device**
              - :ref:`fast_pair_input_device`
              - --
              - --
              - --
              - Experimental
              - Experimental
              - Experimental
            * - **Locator tag**
              - :ref:`fast_pair_locator_tag`
              - --
              - --
              - --
              - Supported
              - Supported
              - Supported

      .. tab:: nRF53 Series

         .. list-table:: Google Fast Pair use case support
            :header-rows: 1
            :widths: auto

            * - Use case
              - |NCS| sample demonstration
              - nRF5340
            * - **Input device**
              - :ref:`fast_pair_input_device`
              - Experimental
            * - **Locator tag**
              - :ref:`fast_pair_locator_tag`
              - Experimental

      .. tab:: nRF54H Series

         .. list-table:: Google Fast Pair use case support
            :header-rows: 1
            :widths: auto

            * - Use case
              - |NCS| sample demonstration
              - nRF54H20
            * - **Input device**
              - :ref:`fast_pair_input_device`
              - Experimental
            * - **Locator tag**
              - :ref:`fast_pair_locator_tag`
              - Experimental

      .. tab:: nRF54L Series

         .. list-table:: Google Fast Pair use case support
            :header-rows: 1
            :widths: auto

            * - Use case
              - |NCS| sample demonstration
              - nRF54L05
              - nRF54L10
              - nRF54L15
              - nRF54LM20A
              - nRF54LV10A
            * - **Input device**
              - :ref:`fast_pair_input_device`
              - Experimental
              - Experimental
              - Experimental
              - Experimental
              - --
            * - **Locator tag**
              - :ref:`fast_pair_locator_tag`
              - Supported
              - Supported
              - Supported
              - Experimental
              - --

      .. tab:: nRF91 Series

         .. list-table:: Google Fast Pair use case support
            :header-rows: 1
            :widths: auto

            * - Use case
              - |NCS| sample demonstration
              - nRF9131
              - nRF9151
              - nRF9160
              - nRF9161
            * - **Input device**
              - :ref:`fast_pair_input_device`
              - --
              - --
              - --
              - --
            * - **Locator tag**
              - :ref:`fast_pair_locator_tag`
              - --
              - --
              - --
              - --

The following table indicates the software maturity levels of the support for each Fast Pair feature:

.. toggle::

   .. _software_maturity_fast_pair_feature:

   .. tabs::

      .. tab:: nRF52 Series

         .. list-table:: Google Fast Pair feature support
            :header-rows: 1
            :widths: auto

            * - Feature
              - nRF52810
              - nRF52811
              - nRF52820
              - nRF52832
              - nRF52833
              - nRF52840
            * - **Initial pairing**
              - --
              - --
              - --
              - Supported
              - Supported
              - Supported
            * - **Subsequent pairing**
              - --
              - --
              - --
              - Experimental
              - Experimental
              - Experimental
            * - **Battery Notification extension**
              - --
              - --
              - --
              - Experimental
              - Experimental
              - Experimental
            * - **Personalized Name extension**
              - --
              - --
              - --
              - Experimental
              - Experimental
              - Experimental
            * - **Find My Device Network extension**
              - --
              - --
              - --
              - Supported
              - Supported
              - Supported

      .. tab:: nRF53 Series

         .. list-table:: Google Fast Pair feature support
            :header-rows: 1
            :widths: auto

            * - Feature
              - nRF5340
            * - **Initial pairing**
              - Experimental
            * - **Subsequent pairing**
              - Experimental
            * - **Battery Notification extension**
              - Experimental
            * - **Personalized Name extension**
              - Experimental
            * - **Find My Device Network extension**
              - Experimental

      .. tab:: nRF54H Series

         .. list-table:: Google Fast Pair feature support
            :header-rows: 1
            :widths: auto

            * - Feature
              - nRF54H20
            * - **Initial pairing**
              - Experimental
            * - **Subsequent pairing**
              - Experimental
            * - **Battery Notification extension**
              - Experimental
            * - **Personalized Name extension**
              - Experimental
            * - **Find My Device Network extension**
              - Experimental

      .. tab:: nRF54L Series

         .. list-table:: Google Fast Pair feature support
            :header-rows: 1
            :widths: auto

            * - Feature
              - nRF54L05
              - nRF54L10
              - nRF54L15
              - nRF54LM20A
              - nRF54LV10A
            * - **Initial pairing**
              - Supported
              - Supported
              - Supported
              - Experimental
              - --
            * - **Subsequent pairing**
              - Experimental
              - Experimental
              - Experimental
              - Experimental
              - --
            * - **Battery Notification extension**
              - Experimental
              - Experimental
              - Experimental
              - Experimental
              - --
            * - **Personalized Name extension**
              - Experimental
              - Experimental
              - Experimental
              - Experimental
              - --
            * - **Find My Device Network extension**
              - Supported
              - Supported
              - Supported
              - Experimental
              - --

      .. tab:: nRF91 Series

         .. list-table:: Google Fast Pair feature support
            :header-rows: 1
            :widths: auto

            * - Feature
              - nRF9131
              - nRF9151
              - nRF9160
              - nRF9161
            * - **Initial pairing**
              - --
              - --
              - --
              - --
            * - **Subsequent pairing**
              - --
              - --
              - --
              - --
            * - **Battery Notification extension**
              - --
              - --
              - --
              - --
            * - **Personalized Name extension**
              - --
              - --
              - --
              - --
            * - **Find My Device Network extension**
              - --
              - --
              - --
              - --

.. include:: /includes/fast_pair_fmdn_rename.txt

.. _software_maturity_security_features:

Security Feature Support
************************

The following sections contain the tables indicating the software maturity levels of the support for the following security features:

* Trusted Firmware-M
* PSA Crypto
* |NSIB|
* Hardware Unique Key
* Trusted storage

.. _software_maturity_security_features_tfm:

Trusted Firmware-M support
==========================

.. toggle::

   .. tfm_ncs_profiles_support_table_start

   .. tabs::

      .. tab:: nRF52 Series

         .. list-table:: TF-M profile support
            :header-rows: 1
            :widths: auto

            * - TF-M profile
              - nRF52810
              - nRF52811
              - nRF52820
              - nRF52832
              - nRF52833
              - nRF52840
            * - :ref:`Configurable <ug_tfm_supported_services_profiles_configurable>`
              - --
              - --
              - --
              - --
              - --
              - --
            * - :ref:`Minimal <ug_tfm_supported_services_profiles_minimal>`
              - --
              - --
              - --
              - --
              - --
              - --

      .. tab:: nRF53 Series

         For board targets supported by TF-M, see :ref:`ug_tfm_building_board_targets`.

         .. list-table:: TF-M profile support
            :header-rows: 1
            :widths: auto

            * - TF-M profile
              - nRF5340
            * - :ref:`Configurable <ug_tfm_supported_services_profiles_configurable>`
              - Experimental
            * - :ref:`Minimal <ug_tfm_supported_services_profiles_minimal>`
              - Supported

      .. tab:: nRF54H Series

         For board targets supported by TF-M, see :ref:`ug_tfm_building_board_targets`.

         .. list-table:: TF-M profile support
            :header-rows: 1
            :widths: auto

            * - TF-M profile
              - nRF54H20
            * - :ref:`Configurable <ug_tfm_supported_services_profiles_configurable>`
              - --
            * - :ref:`Minimal <ug_tfm_supported_services_profiles_minimal>`
              - --

      .. tab:: nRF54L Series

         For board targets supported by TF-M, see :ref:`ug_tfm_building_board_targets`.

         .. list-table:: TF-M profile support
            :header-rows: 1
            :widths: auto

            * - TF-M profile
              - nRF54L05
              - nRF54L10
              - nRF54L15
              - nRF54LM20A
              - nRF54LV10A
            * - :ref:`Configurable <ug_tfm_supported_services_profiles_configurable>`
              - --
              - Experimental
              - Experimental
              - Experimental (with :ref:`limitations <tfm_encrypted_its>`)
              - Experimental
            * - :ref:`Minimal <ug_tfm_supported_services_profiles_minimal>`
              - --
              - Experimental
              - Experimental
              - Experimental (with :ref:`limitations <tfm_encrypted_its>`)
              - Experimental

      .. tab:: nRF91 Series

         For board targets supported by TF-M, see :ref:`ug_tfm_building_board_targets`.

         .. list-table:: TF-M profile support
            :header-rows: 1
            :widths: auto

            * - TF-M profile
              - nRF9131
              - nRF9151
              - nRF9160
              - nRF9161
            * - :ref:`Configurable <ug_tfm_supported_services_profiles_configurable>`
              - --
              - Experimental
              - Experimental
              - Experimental
            * - :ref:`Minimal <ug_tfm_supported_services_profiles_minimal>`
              - Experimental
              - Supported
              - Supported
              - Supported

   .. tfm_ncs_profiles_support_table_end

   For more information about supported TF-M features in the |NCS|, see :ref:`ug_tfm_supported_services`.

.. _software_maturity_security_features_psa:

PSA Crypto support
==================

The following tables list hardware support for the :ref:`PSA Crypto implementations in the |NCS| <ug_crypto_architecture_implementation_standards>`.
The lists are organized by device Series and implementation.

.. toggle::

   .. crypto_support_table_start

   .. tabs::

      .. tab:: nRF52 Series

         The following tables list the cryptographic support for nRF52 Series devices.
         The nRF52 Series devices do not support the :ref:`CRACEN <crypto_drivers_cracen>` driver.

         .. list-table:: Cryptographic support by implementation - nRF52 Series
            :header-rows: 1
            :widths: auto

            * - Implementation
              - nRF52810
              - nRF52811
              - nRF52820
              - nRF52832
              - nRF52833
              - nRF52840
            * - :ref:`Oberon PSA Crypto - nrf_cc3xx <ug_crypto_architecture_implementation_standards_oberon>`
              - --
              - --
              - --
              - --
              - --
              - Supported
            * - :ref:`Oberon PSA Crypto - CRACEN <ug_crypto_architecture_implementation_standards_oberon>`
              - --
              - --
              - --
              - --
              - --
              - --
            * - :ref:`Oberon PSA Crypto - nrf_oberon <ug_crypto_architecture_implementation_standards_oberon>`
              - --
              - --
              - --
              - Supported
              - Supported
              - Supported
            * - :ref:`TF-M Crypto Service <ug_crypto_architecture_implementation_standards_tfm>`
              - --
              - --
              - --
              - --
              - --
              - --
            * - :ref:`IronSide Secure Element <ug_crypto_architecture_implementation_standards_ironside>`
              - --
              - --
              - --
              - --
              - --
              - --

      .. tab:: nRF53 Series

         The following tables list the cryptographic support for nRF53 Series devices.
         The nRF53 Series devices do not support the :ref:`CRACEN <crypto_drivers_cracen>` driver.

         .. list-table:: Cryptographic support by implementation - nRF53 Series
            :header-rows: 1
            :widths: auto

            * - Implementation
              - nRF5340
            * - :ref:`Oberon PSA Crypto - nrf_cc3xx <ug_crypto_architecture_implementation_standards_oberon>`
              - Supported
            * - :ref:`Oberon PSA Crypto - CRACEN <ug_crypto_architecture_implementation_standards_oberon>`
              - --
            * - :ref:`Oberon PSA Crypto - nrf_oberon <ug_crypto_architecture_implementation_standards_oberon>`
              - Supported
            * - :ref:`TF-M Crypto Service <ug_crypto_architecture_implementation_standards_tfm>`
              - Experimental
            * - :ref:`IronSide Secure Element <ug_crypto_architecture_implementation_standards_ironside>`
              - --

      .. tab:: nRF54H Series

         The following tables list the cryptographic support for nRF54H Series devices.
         nRF54H Series devices do not support either the :ref:`nrf_cc3xx <crypto_drivers_cc3xx>` or the :ref:`nrf_oberon <crypto_drivers_oberon>` drivers.

         .. list-table:: Cryptographic support by implementation - nRF54H Series
            :header-rows: 1
            :widths: auto

            * - Implementation
              - nRF54H20
            * - :ref:`Oberon PSA Crypto - nrf_cc3xx <ug_crypto_architecture_implementation_standards_oberon>`
              - --
            * - :ref:`Oberon PSA Crypto - CRACEN <ug_crypto_architecture_implementation_standards_oberon>`
              - --
            * - :ref:`Oberon PSA Crypto - nrf_oberon <ug_crypto_architecture_implementation_standards_oberon>`
              - --
            * - :ref:`TF-M Crypto Service <ug_crypto_architecture_implementation_standards_tfm>`
              - --
            * - :ref:`IronSide Secure Element <ug_crypto_architecture_implementation_standards_ironside>`
              - Supported

      .. tab:: nRF54L Series

         The following tables list the cryptographic support for nRF54L Series devices.
         The nRF54L Series devices do not support the :ref:`nrf_cc3xx <crypto_drivers_cc3xx>` driver.

         .. list-table:: Cryptographic support by implementation - nRF54L Series
            :header-rows: 1
            :widths: auto

            * - Implementation
              - nRF54L05
              - nRF54L10
              - nRF54L15
              - nRF54LM20A
              - nRF54LV10A
            * - :ref:`Oberon PSA Crypto - nrf_cc3xx <ug_crypto_architecture_implementation_standards_oberon>`
              - --
              - --
              - --
              - --
              - --
            * - :ref:`Oberon PSA Crypto - CRACEN <ug_crypto_architecture_implementation_standards_oberon>`
              - Supported
              - Supported
              - Supported
              - Experimental
              - Experimental
            * - :ref:`Oberon PSA Crypto - nrf_oberon <ug_crypto_architecture_implementation_standards_oberon>`
              - Supported
              - Supported
              - Supported
              - Supported
              - Supported
            * - :ref:`TF-M Crypto Service <ug_crypto_architecture_implementation_standards_tfm>`
              - --
              - Experimental
              - Experimental
              - Experimental
              - Experimental
            * - :ref:`IronSide Secure Element <ug_crypto_architecture_implementation_standards_ironside>`
              - --
              - --
              - --
              - --
              - --

      .. tab:: nRF91 Series

         The following tables list the cryptographic support for nRF91 Series devices.
         The nRF91 Series devices do not support the :ref:`CRACEN <crypto_drivers_cracen>` driver.

         .. list-table:: Cryptographic support by implementation - nRF91 Series
            :header-rows: 1
            :widths: auto

            * - Implementation
              - nRF9131
              - nRF9151
              - nRF9160
              - nRF9161
            * - :ref:`Oberon PSA Crypto - nrf_cc3xx <ug_crypto_architecture_implementation_standards_oberon>`
              - Experimental
              - Supported
              - Supported
              - Supported
            * - :ref:`Oberon PSA Crypto - CRACEN <ug_crypto_architecture_implementation_standards_oberon>`
              - --
              - --
              - --
              - --
            * - :ref:`Oberon PSA Crypto - nrf_oberon <ug_crypto_architecture_implementation_standards_oberon>`
              - Supported
              - Supported
              - Supported
              - Supported
            * - :ref:`TF-M Crypto Service <ug_crypto_architecture_implementation_standards_tfm>`
              - Experimental
              - Experimental
              - Experimental
              - Experimental
            * - :ref:`IronSide Secure Element <ug_crypto_architecture_implementation_standards_ironside>`
              - --
              - --
              - --
              - --

.. crypto_support_table_end

.. _software_maturity_security_features_nsib:

|NSIB|
======

.. toggle::

   .. tabs::

      .. tab:: nRF52 Series

        .. list-table:: Immutable Bootloader support
           :header-rows: 1
           :widths: auto

           * - Feature
             - nRF52810
             - nRF52811
             - nRF52820
             - nRF52832
             - nRF52833
             - nRF52840
           * - **Immutable Bootloader as part of build**
             - --
             - --
             - --
             - Supported
             - Supported
             - Supported

      .. tab:: nRF53 Series

        .. list-table:: Immutable Bootloader support
           :header-rows: 1
           :widths: auto

           * - Feature
             - nRF5340
           * - **Immutable Bootloader as part of build**
             - Supported

      .. tab:: nRF54H Series

        .. list-table:: Immutable Bootloader support
           :header-rows: 1
           :widths: auto

           * - Feature
             - nRF54H20
           * - **Immutable Bootloader as part of build**
             - --

      .. tab:: nRF54L Series

        .. list-table:: Immutable Bootloader support
           :header-rows: 1
           :widths: auto

           * - Feature
             - nRF54L05
             - nRF54L10
             - nRF54L15
             - nRF54LM20A
             - nRF54LV10A
           * - **Immutable Bootloader as part of build**
             - Experimental
             - Experimental
             - Experimental
             - Experimental
             - Experimental

      .. tab:: nRF91 Series

        .. list-table:: Immutable Bootloader support
           :header-rows: 1
           :widths: auto

           * - Feature
             - nRF9131
             - nRF9151
             - nRF9160
             - nRF9161
           * - **Immutable Bootloader as part of build**
             - --
             - Supported
             - Supported
             - Supported

.. _software_maturity_security_features_huk:

Hardware Unique Key
===================

.. toggle::

   .. tabs::

      .. tab:: nRF52 Series

        .. list-table:: Key Derivation support
           :header-rows: 1
           :widths: auto

           * - Feature
             - nRF52810
             - nRF52811
             - nRF52820
             - nRF52832
             - nRF52833
             - nRF52840
           * - **Key Derivation from Hardware Unique Key**
             - --
             - --
             - --
             - --
             - --
             - Supported

      .. tab:: nRF53 Series

        .. list-table:: Key Derivation support
           :header-rows: 1
           :widths: auto

           * - Feature
             - nRF5340
           * - **Key Derivation from Hardware Unique Key**
             - Supported

      .. tab:: nRF54H Series

        .. list-table:: Key Derivation support
           :header-rows: 1
           :widths: auto

           * - Feature
             - nRF54H20
           * - **Key Derivation from Hardware Unique Key**
             - --

      .. tab:: nRF54L Series

        .. list-table:: Key Derivation support
           :header-rows: 1
           :widths: auto

           * - Feature
             - nRF54L05
             - nRF54L10
             - nRF54L15
             - nRF54LM20A
             - nRF54LV10A
           * - **Key Derivation from Hardware Unique Key**
             - Experimental
             - Experimental
             - Experimental
             - Experimental
             - Experimental

      .. tab:: nRF91 Series

        .. list-table:: Key Derivation support
           :header-rows: 1
           :widths: auto

           * - Feature
             - nRF9131
             - nRF9151
             - nRF9160
             - nRF9161
           * - **Key Derivation from Hardware Unique Key**
             - --
             - Supported
             - Supported
             - Supported

.. _software_maturity_security_features_ts:

Trusted storage
===============

Trusted storage implements the PSA Certified Secure Storage APIs without TF-M.

.. toggle::

   .. tabs::

      .. tab:: nRF52 Series

         .. list-table:: Trusted storage support
            :header-rows: 1
            :widths: auto

            * - Feature
              - nRF52810
              - nRF52811
              - nRF52820
              - nRF52832
              - nRF52833
              - nRF52840
            * - **Trusted storage**
              - --
              - --
              - --
              - --
              - --
              - Supported

      .. tab:: nRF53 Series

         .. list-table:: Trusted storage support
            :header-rows: 1
            :widths: auto

            * - Feature
              - nRF5340
            * - **Trusted storage**
              - Supported

      .. tab:: nRF54H Series

         .. list-table:: Trusted storage support
            :header-rows: 1
            :widths: auto

            * - Feature
              - nRF54H20
            * - **Trusted storage**
              - Experimental

      .. tab:: nRF54L Series

         .. list-table:: Trusted storage support
            :header-rows: 1
            :widths: auto

            * - Feature
              - nRF54L05
              - nRF54L10
              - nRF54L15
              - nRF54LM20A
              - nRF54LV10A
            * - **Trusted storage**
              - Supported
              - Supported
              - Supported
              - Experimental
              - Supported

      .. tab:: nRF91 Series

         .. list-table:: Trusted storage support
            :header-rows: 1
            :widths: auto

            * - Feature
              - nRF9131
              - nRF9151
              - nRF9160
              - nRF9161
            * - **Trusted storage**
              - --
              - Supported
              - Supported
              - Supported

MCUboot bootloader
******************

The following table indicates the software maturity levels of the support for each MCUboot bootloader feature:

.. toggle::

   .. tabs::

      .. tab:: nRF52 Series

         .. list-table:: Bootloader and security features
            :header-rows: 1
            :widths: auto

            * - Feature
              - nRF52810
              - nRF52811
              - nRF52820
              - nRF52832
              - nRF52833
              - nRF52840
            * - **Immutable MCUboot as part of build**
              - --
              - --
              - --
              - Supported
              - Supported
              - Supported
            * - **Updatable MCUboot as part of build**
              - --
              - --
              - --
              - Supported
              - Supported
              - Supported
            * - **Application image compression**
              - --
              - --
              - --
              - --
              - --
              - Supported
            * - **Hardware cryptography acceleration**
              - --
              - --
              - --
              - --
              - --
              - Supported
            * - **Multiple signature keys**
              - --
              - --
              - --
              - --
              - --
              - --
            * - **Image encryption**
              - --
              - --
              - --
              - Experimental
              - Experimental
              - Experimental

      .. tab:: nRF53 Series

         .. list-table:: Bootloader and security features
            :header-rows: 1
            :widths: auto

            * - Feature
              - nRF5340
            * - **Immutable MCUboot as part of build**
              - Supported
            * - **Updatable MCUboot as part of build**
              - Supported
            * - **Application image compression**
              - Supported
            * - **Hardware cryptography acceleration**
              - --
            * - **Multiple signature keys**
              - --
            * - **Image encryption**
              - Experimental

      .. tab:: nRF54H Series

         .. list-table:: Bootloader and security features
            :header-rows: 1
            :widths: auto

            * - Feature
              - nRF54H20
            * - **Immutable MCUboot as part of build**
              - Experimental
            * - **Updatable MCUboot as part of build**
              - --
            * - **Application image compression**
              - --
            * - **Hardware cryptography acceleration**
              - --
            * - **Multiple signature keys**
              - --
            * - **Image encryption**
              - --

      .. tab:: nRF54L Series

         .. list-table:: Bootloader and security features
            :header-rows: 1
            :widths: auto

            * - Feature
              - nRF54L05
              - nRF54L10
              - nRF54L15
              - nRF54LM20A
              - nRF54LV10A
            * - **Immutable MCUboot as part of build**
              - Supported
              - Supported
              - Supported
              - Experimental
              - Experimental
            * - **Updatable MCUboot as part of build**
              - Experimental
              - Experimental
              - Experimental
              - Experimental
              - Experimental
            * - **Application image compression**
              - --
              - Supported
              - Supported
              - Experimental
              - Experimental
            * - **Hardware cryptography acceleration**
              - Supported
              - Supported
              - Supported
              - Experimental
              - Experimental
            * - **Multiple signature keys**
              - Supported
              - Supported
              - Supported
              - Experimental
              - Experimental
            * - **Image encryption**
              - Experimental
              - Experimental
              - Experimental
              - Experimental
              - Experimental

      .. tab:: nRF91 Series

         .. list-table:: Bootloader and security features
            :header-rows: 1
            :widths: auto

            * - Feature
              - nRF9131
              - nRF9151
              - nRF9160
              - nRF9161
            * - **Immutable MCUboot as part of build**
              - --
              - Supported
              - Supported
              - Supported
            * - **Updatable MCUboot as part of build**
              - --
              - Supported
              - Supported
              - Supported
            * - **Application image compression**
              - --
              - Supported
              - Supported
              - Supported
            * - **Hardware cryptography acceleration**
              - --
              - Supported
              - Supported
              - Supported
            * - **Multiple signature keys**
              - --
              - --
              - --
              - --
            * - **Image encryption**
              - --
              - Experimental
              - Experimental
              - Experimental

Power management device support
*******************************

The following table indicates the software maturity levels of the support for each power management device:

.. toggle::

   .. tabs::

      .. tab:: nRF52 Series

         .. list-table:: nPM module support
            :header-rows: 1
            :widths: auto

            * - Module
              - nRF52810
              - nRF52811
              - nRF52820
              - nRF52832
              - nRF52833
              - nRF52840
            * - **nPM1100**
              - --
              - --
              - --
              - --
              - --
              - --
            * - **nPM1300**
              - --
              - --
              - --
              - Supported
              - --
              - Supported
            * - **nPM2100**
              - --
              - --
              - --
              - --
              - --
              - Supported
            * - **nPM6001**
              - --
              - --
              - --
              - --
              - --
              - --
      .. tab:: nRF53 Series

         .. list-table:: nPM module support
            :header-rows: 1
            :widths: auto

            * - Module
              - nRF5340
            * - **nPM1100**
              - Supported
            * - **nPM1300**
              - Supported
            * - **nPM2100**
              - Supported
            * - **nPM6001**
              - Experimental

      .. tab:: nRF54H Series

         .. list-table:: nPM module support
            :header-rows: 1
            :widths: auto

            * - Module
              - nRF54H20
            * - **nPM1100**
              - --
            * - **nPM1300**
              - Supported
            * - **nPM1304**
              - Supported
            * - **nPM2100**
              - --
            * - **nPM6001**
              - --

      .. tab:: nRF54L Series

         .. list-table:: nPM module support
            :header-rows: 1
            :widths: auto

            * - Module
              - nRF54L05
              - nRF54L10
              - nRF54L15
              - nRF54LM20A
              - nRF54LV10A
            * - **nPM1100**
              - --
              - --
              - --
              - --
              - --
            * - **nPM1300**
              - --
              - --
              - Supported
              - Supported
              - --
            * - **nPM1304**
              - --
              - --
              - Supported
              - Supported
              - --
            * - **nPM2100**
              - --
              - --
              - Supported
              - --
              - --
            * - **nPM6001**
              - --
              - --
              - --
              - --
              - --

      .. tab:: nRF91 Series

         .. list-table:: nPM module support
            :header-rows: 1
            :widths: auto

            * - Module
              - nRF9131
              - nRF9151
              - nRF9160
              - nRF9161
            * - **nPM1100**
              - --
              - --
              - --
              - --
            * - **nPM1300**
              - Experimental
              - Supported
              - Supported
              - --
            * - **nPM2100**
              - --
              - --
              - --
              - --
            * - **nPM6001**
              - --
              - Supported
              - --
              - --

Front-End Modules support
*************************

The following table indicates the software maturity levels of the support for Front-End Modules:

.. toggle::

  .. software_maturity_fem_support_table_start

  .. tabs::

    .. group-tab:: nRF52 Series

        .. list-table:: Front-End Module support
          :widths: auto
          :header-rows: 1

          * - FEM device
            - Implementation
            - nRF52810
            - nRF52811
            - nRF52820
            - nRF52832
            - nRF52833
            - nRF52840
          * - nRF21540
            - nRF21540 GPIO
            - --
            - --
            - --
            - --
            - Supported
            - Supported
          * - nRF21540
            - nRF21540 GPIO+SPI
            - --
            - --
            - --
            - --
            - Supported
            - Supported
          * - SKY66112-11
            - Simple GPIO
            - --
            - --
            - --
            - --
            - Supported
            - Supported

    .. group-tab:: nRF53 Series

        .. list-table:: Front-End Module support
          :widths: auto
          :header-rows: 1

          * - FEM device
            - Implementation
            - nRF5340
          * - nRF21540
            - nRF21540 GPIO
            - Supported
          * - nRF21540
            - nRF21540 GPIO+SPI
            - Supported
          * - SKY66112-11
            - Simple GPIO
            - Supported

    .. group-tab:: nRF54H Series

        .. list-table:: Front-End Module support
          :widths: auto
          :header-rows: 1

          * - FEM device
            - Implementation
            - nRF54H20
          * - nRF21540
            - nRF21540 GPIO
            - --
          * - nRF21540
            - nRF21540 GPIO+SPI
            - --
          * - SKY66112-11
            - Simple GPIO
            - --

    .. group-tab:: nRF54L Series

        .. list-table:: Front-End Module support
          :widths: auto
          :header-rows: 1

          * - FEM device
            - Implementation
            - nRF54L05
            - nRF54L10
            - nRF54L15
            - nRF54LM20A
            - nRF54LV10A
          * - nRF21540
            - nRF21540 GPIO
            - Supported
            - Supported
            - Supported
            - --
            - --
          * - nRF21540
            - nRF21540 GPIO+SPI
            - Supported
            - Supported
            - Supported
            - Supported
            - --
          * - SKY66112-11
            - Simple GPIO
            - Supported
            - Supported
            - Supported
            - --
            - --

    .. group-tab:: nRF91 Series

        .. list-table:: Front-End Module support
          :widths: auto
          :header-rows: 1

          * - FEM device
            - Implementation
            - nRF9131
            - nRF9151
            - nRF9160
            - nRF9161
          * - nRF21540
            - nRF21540 GPIO
            - --
            - --
            - --
            - --
          * - nRF21540
            - nRF21540 GPIO+SPI
            - --
            - --
            - --
            - --
          * - SKY66112-11
            - Simple GPIO
            - --
            - --
            - --
            - --

  .. software_maturity_fem_support_table_end
