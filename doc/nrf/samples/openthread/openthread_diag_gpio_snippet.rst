.. _snippet_ot_diag_gpio:

OpenThread snippet for diagnostic GPIO commands (ot-diag-gpio)
==============================================================

.. contents::
   :local:
   :depth: 2

To build with this snippet, follow the instructions in the :ref:`using-snippets` page.
When building with west, run the following command:

.. code-block:: console

   west build -- -D<project_name>_SNIPPET=ot-diag-gpio

Overview
********

This snippet enables the :kconfig:option:`CONFIG_GPIO_GET_DIRECTION` Kconfig option to support querying the GPIO direction.
It is used in the OpenThread samples to configure the DK buttons and LEDs for diagnostic GPIO commands.
