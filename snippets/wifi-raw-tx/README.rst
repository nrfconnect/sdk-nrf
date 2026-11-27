.. _snippet-wifi-raw-tx:

Wi-Fi raw transmit snippet (wifi-raw-tx)
########################################

.. contents::
   :local:
   :depth: 2

The ``wifi-raw-tx`` snippet enables the transmission of raw 802.11 frames, which lets an application inject fully formed frames instead of sending data through the networking stack.

Overview
********

The snippet enables raw data transmission in the driver (:kconfig:option:`CONFIG_NRF70_RAW_DATA_TX` on the nRF70 Series and :kconfig:option:`CONFIG_NRF71_RAW_DATA_TX` on the nRF71 Series) and logging in immediate mode (:kconfig:option:`CONFIG_LOG_MODE_IMMEDIATE`).

Supported board targets
***********************

The snippet applies the nRF70 Series configuration to the following board targets:

* ``nrf7002dk/nrf5340/cpuapp``, including the ``nrf7001`` and ``ns`` variants
* ``nrf5340dk/nrf5340/cpuapp``
* ``nrf54l15dk/nrf54l15/cpuapp``
* ``nrf54lm20dk/nrf54lm20a/cpuapp`` and ``nrf54lm20dk/nrf54lm20b/cpuapp``

The snippet applies the nRF71 Series configuration to the following board targets:

* ``nrf7120dk/nrf7120/cpuapp``
* ``nrf7120dk/nrf7120/cpuapp/ns``

Usage
*****

Apply the snippet when building, for example for the :ref:`wifi_shell_sample` sample:

.. code-block:: console

   west build -p -b <board target> samples/wifi/shell -S wifi-raw-tx -- -DSB_CONFIG_WIFI_NRF70_SYSTEM_WITH_RAW_MODES=y
