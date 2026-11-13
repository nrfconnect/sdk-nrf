.. _snippet_ot_diag_gpio:
.. _snippet-ot-diag-gpio:

OpenThread snippet for diagnostic GPIO commands (ot-diag-gpio)
##############################################################

.. contents::
   :local:
   :depth: 2

The ``ot-diag-gpio`` snippet enables the :kconfig:option:`CONFIG_GPIO_GET_DIRECTION` Kconfig option to support querying the GPIO direction.
It is used in the OpenThread samples to configure the DK buttons and LEDs for diagnostic GPIO commands.

Overview
********

For more information about the diagnostic GPIO commands, see `OpenThread Factory Diagnostics Module Reference`_.

Supported board targets
***********************

The snippet provides a devicetree overlay for the following board targets:

* ``nrf5340dk/nrf5340/cpuapp``
* ``nrf54l15dk/nrf54l15/cpuapp``

Usage
*****

To build with this snippet, follow the instructions in the :ref:`zephyr:using-snippets` page.
When building with west, run the following command:

.. code-block:: console

   west build -- -D<project_name>_SNIPPET=ot-diag-gpio
