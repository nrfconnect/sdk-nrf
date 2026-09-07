.. _nrf70_wifi_low_level_api:

Low-level API
#############

.. contents::
   :local:
   :depth: 2

The nRF70 Series Wi-Fi® driver provides a low-level API for applications that need direct access to the nRF70 Series device.
This is intended for users who want to use the nRF70 Series device on a platform other than Zephyr.

Common
******

The common API is used for both normal operation and Radio Test mode.

| Header file: :file:`nrf_wifi/fw_if/umac_if/inc/fmac_structs_common.h`
| Source file: :file:`nrf_wifi/fw_if/umac_if/src/fmac_api_common.c`


.. doxygengroup:: nrf_wifi_api_common
   :project: wifi

Radio Test mode
***************

The Radio Test mode is used for testing the RF performance of the nRF70 Series device, mainly to characterize the RF.

.. note::

   Documentation for data structure definitions is disabled for this module.
   It will be enabled in the future.

| Source file: :file:`nrf_wifi/fw_if/umac_if/src/radio_test/fmac_api.c`

.. doxygengroup:: nrf_wifi_api_radio_test
   :project: wifi

Wi-Fi mode
**********

The Wi-Fi mode is used for the normal operation of the nRF70 Series device in the :abbr:`STA (Station)` mode or other modes that will be supported in the future.

| Header file: :file:`nrf_wifi/fw_if/umac_if/inc/default/fmac_api.h`
| Source file: :file:`nrf_wifi/fw_if/umac_if/src/default/fmac_api.c`

.. doxygengroup:: nrf_wifi_api
   :project: wifi
