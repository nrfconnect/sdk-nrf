.. _sdp_gpio:

SDP GPIO
########

.. contents::
   :local:
   :depth: 2


This application demonstrates how to create SDP on FLPR in Zephyr threadless mode by implementing a subset of Zephyr GPIO API.
It is intended for use with Zephyr's :zephyr:code-sample:`blinky` sample.

You can use the following IPC backends:

* mbox (:kconfig:option:`SB_CONFIG_SDP_GPIO_BACKEND_MBOX`)
* icmsg (:kconfig:option:`SB_CONFIG_SDP_GPIO_BACKEND_ICMSG`)
* icbmsg (:kconfig:option:`SB_CONFIG_SDP_GPIO_BACKEND_ICBMSG`)

Requirements
************

The firmware supports the following development kit:

.. table-from-sample-yaml::

Building and running
********************

You must include code for both the application core and FLPR core.
The recommended method is to build the :zephyr:code-sample:`blinky` with the necessary sysbuild configuration.

For example, to build with icmsg backend, run the following commands:

  .. code-block:: console

     west build -b nrf54l15dk/nrf54l15/cpuapp -- -DSB_CONFIG_PARTITION_MANAGER=n -DSB_CONFIG_SDP=y -DSB_CONFIG_SDP_GPIO=y -DSB_CONFIG_SDP_GPIO_BACKEND_ICMSG=y -DEXTRA_DTC_OVERLAY_FILE="./boards/nrf54l15dk_nrf54l15_cpuapp_egpio.overlay"
     west flash

Upon successful execution, **LED0** will start flashing.
