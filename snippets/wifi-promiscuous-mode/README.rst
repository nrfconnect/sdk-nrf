.. _snippet-wifi-promiscuous-mode:

Wi-Fi Promiscuous mode snippet (wifi-promiscuous-mode)
######################################################

.. contents::
   :local:
   :depth: 2

The ``wifi-promiscuous-mode`` snippet configures the Wi-Fi® driver to operate in Promiscuous mode, in which the device passes all the received frames to the networking stack instead of only the frames addressed to it.

Overview
********

The snippet enables promiscuous data reception in the driver (:kconfig:option:`CONFIG_NRF70_PROMISC_DATA_RX` on the nRF70 Series and :kconfig:option:`CONFIG_NRF71_PROMISC_DATA_RX` on the nRF71 Series) and the networking stack Promiscuous mode (:kconfig:option:`CONFIG_NET_PROMISCUOUS_MODE`).

On the nRF71 Series, the snippet additionally increases the number of receive buffers (:kconfig:option:`CONFIG_NRF71_RX_NUM_BUFS`) and the maximum transmit aggregation (:kconfig:option:`CONFIG_NRF71_MAX_TX_AGGREGATION`) to handle the higher frame rate.

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

Apply the snippet when building, for example:

.. code-block:: console

   west build -S wifi-promiscuous-mode [...]
