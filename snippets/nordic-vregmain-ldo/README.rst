.. _nordic-vregmain-ldo:

Nordic Main Voltage Regulator LDO snippet (nordic-vregmain-ldo)
###############################################################

.. code-block:: console

   west build -S nordic-vregmain-ldo [...]

Overview
********

This snippet allows you to build applications with the main voltage regulator enabled in LDO (Low Dropout) mode.
The LDO mode provides regulated power output with a lower dropout voltage requirement compared to other regulation modes.

Supported boards
****************

* ``nrf5340dk/nrf5340/cpuapp``
* ``nrf54l15dk/nrf54l05/cpuapp``
* ``nrf54l15dk/nrf54l10/cpuapp``
* ``nrf54l15dk/nrf54l15/cpuapp``
* ``nrf54lc10dk/nrf54lc10a/cpuapp``
* ``nrf54lm20dk/nrf54lm20a/cpuapp``
* ``nrf54lm20dk/nrf54lm20b/cpuapp``
* ``nrf54ls05dk/nrf54ls05a/cpuapp``
* ``nrf54ls05dk/nrf54ls05b/cpuapp``
* ``nrf54lv10dk/nrf54lv10a/cpuapp``
