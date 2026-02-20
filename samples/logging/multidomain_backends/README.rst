.. _multidomain_backends_sample:

Multi-domain Logging Backends (nRF54LM20)
#########################################

Overview
********

This sample runs on ``nrf54lm20dk/nrf54lm20a/cpuapp`` with a remote image on
``cpuflpr`` and demonstrates:

* Multi-domain logging (``cpuapp`` root domain + ``cpuflpr`` remote domain)
* Runtime filtering by backend and module
* UART backend carrying ``ERR`` from all sources plus application ``INF``/``WRN``
* BLE backend (NUS-compatible UUIDs) carrying only application logs at ``INF``+
* Filesystem backend on internal RRAM carrying only ``ERR`` logs
* Optional dictionary logging for all three backends

Configuration behavior
**********************

At runtime the sample applies backend filters:

* UART backend: all sources in all active domains at ``LOG_LEVEL_ERR`` then
  application modules (``cpuapp_main`` and ``cpuflpr_main``) set to ``LOG_LEVEL_INF``
* Filesystem backend: all sources in all active domains set to ``LOG_LEVEL_NONE``,
  then application modules (``cpuapp_main`` and ``cpuflpr_main``) set to ``LOG_LEVEL_ERR``
* BLE backend: all sources first set to ``LOG_LEVEL_NONE``, then application
  modules (``cpuapp_main`` and ``cpuflpr_main``) set to ``LOG_LEVEL_INF``

After startup, runtime filtering is exercised again by enabling ``INF`` on
``cpuapp_verbose`` and ``cpuflpr_verbose`` for the BLE backend.

Dictionary logging option
*************************

Enable dictionary mode on all enabled backends with:

.. code-block:: console

   west build -b nrf54lm20dk/nrf54lm20a/cpuapp \
     nrf/samples/logging/multidomain_backends \
     -- -DCONFIG_MULTIDOMAIN_BACKENDS_DICTIONARY_LOGGING=y

Build and run
*************

.. code-block:: console

   west build -b nrf54lm20dk/nrf54lm20a/cpuapp \
     nrf/samples/logging/multidomain_backends

.. code-block:: console

   west flash

Serial output can be observed on the SEGGER CDC ACM port, for example:

.. code-block:: console

   /dev/serial/by-id/usb-SEGGER_J-Link_001051851773-if02

BLE backend notes
*****************

The BLE logger backend exposes NUS-compatible UUIDs. Connect with nRF Toolbox
(or another NUS client), enable notifications, and BLE log streaming starts.
