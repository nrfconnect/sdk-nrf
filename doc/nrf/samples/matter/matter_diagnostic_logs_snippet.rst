.. _snippet_matter_diagnostic_logs:
.. _ug_matter_diagnostic_logs_snippet:

Matter diagnostic logs snippet (matter-diagnostic-logs)
#######################################################

.. contents::
   :local:
   :depth: 2

To build with this snippet, follow the instructions in the :ref:`using-snippets` page.

.. tabs::

   .. group-tab:: |nRFVSC|

      When using |nRFVSC| select the ``matter-diagnostic-logs`` snippet from the list in the :guilabel:`Snippets` menu.

   .. group-tab:: Command line

      When building with west, run the following command:

      .. parsed-literal::
         :class: highlight

         west build -- -D<project_name>_SNIPPET=matter-diagnostic-logs

Overview
********

The Matter diagnostic logs snippet enables the set of configurations needed for full Matter diagnostic logs support.
The configuration set consists of devicetree overlays for each supported target board and a :file:`config` file that enables all diagnostic logs features by default.
The devicetree overlays add new RAM partitions that are configured as retained to keep the log data persistent and survive the device reboot.
They also reduce the SRAM size according to the size of the retained partition.
The partition sizes are configured using example values and might not be sufficient for all use cases.
To change the partition sizes, you need to change the configuration in the devicetree overlay.
You can, for example, increase the partition sizes to be able to store more logs.

The snippet sets the following Kconfig options:

  * :option:`CONFIG_NCS_SAMPLE_MATTER_DIAGNOSTIC_LOGS` to ``y``.
  * :option:`CONFIG_NCS_SAMPLE_MATTER_DIAGNOSTIC_LOGS_CRASH_LOGS` to ``y``.
  * :option:`CONFIG_NCS_SAMPLE_MATTER_DIAGNOSTIC_LOGS_REMOVE_CRASH_AFTER_READ` to ``y``.
  * :option:`CONFIG_NCS_SAMPLE_MATTER_DIAGNOSTIC_LOGS_END_USER_LOGS` to ``y``.
  * :option:`CONFIG_NCS_SAMPLE_MATTER_DIAGNOSTIC_LOGS_NETWORK_LOGS` to ``y``.
  * :kconfig:option:`CONFIG_LOG_MODE_DEFERRED` to ``y``.
  * :kconfig:option:`CONFIG_LOG_RUNTIME_FILTERING` to ``n``.

Deferred logs mode (:kconfig:option:`CONFIG_LOG_MODE_DEFERRED`) is enabled, because it is required by the log redirection functionality (:option:`CONFIG_NCS_SAMPLE_MATTER_DIAGNOSTIC_LOGS_REDIRECT`), which is enabled by default for diagnostic network and end-user logs.

.. note::

   You cannot set the :option:`CONFIG_NCS_SAMPLE_MATTER_DIAGNOSTIC_LOGS` Kconfig option separately without adding the devicetree overlays contained in the snippet.
   Instead, if you want to use just some of the diagnostic logs functionality, use the snippet and set the Kconfig options for the other functionalities to ``n``.

To use the snippet when building a sample:

.. tabs::

   .. group-tab:: |nRFVSC|

      When using |nRFVSC| select the ``matter-diagnostic-logs`` snippet from the list in the :guilabel:`Snippets` menu.

   .. group-tab:: Command line

      When building with west, add ``-D<project_name>_SNIPPET=matter-diagnostic-logs`` to the west arguments list.

      For example, for the ``nrf52840dk/nrf52840`` board target and the :ref:`matter_lock_sample` sample, run the following command:

      .. parsed-literal::
         :class: highlight

         west build -b nrf52840dk/nrf52840 -- -Dlock_SNIPPET=matter-diagnostic-logs
