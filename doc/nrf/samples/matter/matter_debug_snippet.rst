.. _snippet_matter_debug:
.. _ug_matter_debug_snippet:

Matter debug snippet (matter-debug)
###################################

.. contents::
   :local:
   :depth: 2

To build with this snippet, follow the instructions in the :ref:`using-snippets` page.
When building with west, run the following command:

.. code-block:: console

   west build -- -D<project_name>_SNIPPET=matter-debug

Overview
********

The Matter debug snippet allows you to enable additional debug features while using Matter samples.

The following features are enabled when using this snippet:

  * UART speed is increased to 1 Mbit/s.
  * Log buffer size is set to high value to allow showing all logs.
  * Deferred mode of logging.
  * Increased verbosity of Matter logs.
  * OpenThread is built from sources.
  * OpenThread shell is enabled.
  * OpenThread logging level is set to INFO.
  * Full shell functionalities.
  * Logging source code location on VerifyOrDie failure that occurs in the Matter stack.

To use the snippet when building a sample, add ``-D<project_name>_SNIPPET=matter-debug`` to the west arguments list.

For example, for the ``nrf52840dk/nrf52840`` board target and the :ref:`matter_lock_sample` sample, run the following command:

.. parsed-literal::
   :class: highlight

   west build -b nrf52840dk/nrf52840 -- -Dlock_SNIPPET=matter-debug

Requirements
************

You can increase the UART speed using this snippet only for Nordic Semiconductor's development kits.
If you want to use the snippet for your custom board, you need to adjust the UART speed manually.
