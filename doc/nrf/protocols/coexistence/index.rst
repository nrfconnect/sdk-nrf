.. _ug_wireless_coexistence:

Wireless coexistence
####################

The |NCS| allows you to develop applications with wireless coexistence.

The following table lists wireless coexistence between protocols:

.. list-table::
    :widths: auto
    :header-rows: 1

    * - Wireless coexistence
      - Bluetooth LE
      - Thread
      - Zigbee
      - Wi-Fi
      - LTE-M/NB-IoT
    * - Bluetooth LE
      - N/A
      - :ref:`Supported<ug_multiprotocol_support>`
      - :ref:`Supported<ug_multiprotocol_support>`
      - :ref:`Supported<ug_radio_coex>`
      - :ref:`Supported <ug_radio_coex_bluetooth_only_1wire>`
    * - Thread
      - :ref:`Supported<ug_multiprotocol_support>`
      - N/A
      -
      - :ref:`Supported<ug_radio_coex>`
      -
    * - Zigbee
      - :ref:`Supported<ug_multiprotocol_support>`
      -
      - N/A
      - :ref:`Supported<ug_radio_coex>`
      -
    * - Wi-Fi
      - :ref:`Supported<ug_radio_coex>`
      - :ref:`Supported<ug_radio_coex>`
      - :ref:`Supported<ug_radio_coex>`
      - N/A
      -
    * - LTE-M/NB-IoT
      - :ref:`Supported <ug_radio_coex_bluetooth_only_1wire>`
      -
      -
      -
      - N/A
