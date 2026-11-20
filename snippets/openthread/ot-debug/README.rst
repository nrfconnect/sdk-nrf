.. _snippet_ot_debug:
.. _snippet-ot-debug:

OpenThread snippet for debugging purpose (ot-debug)
###################################################

.. contents::
   :local:
   :depth: 2

The ``ot-debug`` snippet enables the :c:func:`__ASSERT` function for debugging OpenThread samples.
This snippet also enables multithread support in GNU Debugger (GDB) and thread monitoring.

Overview
********

The snippet applies the following configuration:

* Enables the debug build options (:kconfig:option:`CONFIG_DEBUG`) and disables the timeslicing (:kconfig:option:`CONFIG_TIMESLICING`).
* Enables thread monitoring (:kconfig:option:`CONFIG_THREAD_MONITOR`), thread stack information (:kconfig:option:`CONFIG_THREAD_STACK_INFO`), and thread names (:kconfig:option:`CONFIG_THREAD_NAME`).
* Enables the thread awareness information for debuggers (:kconfig:option:`CONFIG_DEBUG_THREAD_INFO`).
* Builds OpenThread from sources (:kconfig:option:`CONFIG_OPENTHREAD_SOURCES`).
* Enables assertions (:kconfig:option:`CONFIG_ASSERT`) without the condition information (:kconfig:option:`CONFIG_ASSERT_NO_COND_INFO`).

For additional logging options, see :ref:`RTT logging <ug_logging_backends_rtt>`.

Usage
*****

To build with this snippet, follow the instructions in the :ref:`zephyr:using-snippets` page.
When building with west, run the following command:

.. code-block:: console

   west build -- -D<project_name>_SNIPPET=ot-debug
