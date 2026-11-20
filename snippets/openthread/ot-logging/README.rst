.. _snippet_ot_logging:
.. _snippet-ot-logging:

OpenThread snippet for logging (ot-logging)
###########################################

.. contents::
   :local:
   :depth: 2

The ``ot-logging`` snippet enables logging for the OpenThread stack.

Overview
********

The snippet applies the following configuration:

* Enables logging (:kconfig:option:`CONFIG_LOG`) in the deferred mode (:kconfig:option:`CONFIG_LOG_MODE_DEFERRED`), with the maximum log level set to ``3`` (:kconfig:option:`CONFIG_LOG_MAX_LEVEL`).
* Enables the OpenThread debug output (:kconfig:option:`CONFIG_OPENTHREAD_DEBUG`) with the log level set to info (:kconfig:option:`CONFIG_OPENTHREAD_LOG_LEVEL_INFO`).
* Selects the RTT log backend (:kconfig:option:`CONFIG_LOG_BACKEND_RTT`) and disables the Spinel, UART, and shell log backends.

Development kits support
************************

This snippet enables RTT logging for debugging Thread samples on supported development kits.
For additional options, see :ref:`RTT logging <ug_logging_backends_rtt>`.

Usage
*****

To build with this snippet, follow the instructions in the :ref:`zephyr:using-snippets` page.
When building with west, run the following command:

.. code-block:: console

   west build -- -D<project_name>_SNIPPET=ot-logging
