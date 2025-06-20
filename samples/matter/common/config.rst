.. _matter_samples_config:

Shared configurations in Matter samples
#######################################

.. contents::
   :local:
   :depth: 2

This page lists Kconfig options and snippets shared by :ref:`matter_samples` in the |NCS|.
See the :ref:`ug_matter_device_advanced_kconfigs` page for more detailed information.

.. _matter_samples_kconfig:

Configuration options
*********************

Check and configure the following configuration options:

.. _CONFIG_NCS_SAMPLE_MATTER_APP_TASK_QUEUE_SIZE:

CONFIG_NCS_SAMPLE_MATTER_APP_TASK_QUEUE_SIZE
  ``int`` - Define the maximum number of tasks the queue dedicated for application tasks that have to be run in the application thread context.

.. _CONFIG_NCS_SAMPLE_MATTER_APP_TASK_MAX_SIZE:

CONFIG_NCS_SAMPLE_MATTER_APP_TASK_MAX_SIZE
  ``int`` - Define the maximum size (in bytes) of an application task that can be put in the task queue.

.. _CONFIG_NCS_SAMPLE_MATTER_CUSTOM_BLUETOOTH_ADVERTISING:

CONFIG_NCS_SAMPLE_MATTER_CUSTOM_BLUETOOTH_ADVERTISING
  ``bool`` - Disable the default Bluetooth advertising start, which is defined in the :file:`board.cpp` file.
  This allows you to use a custom one.

.. _CONFIG_NCS_SAMPLE_MATTER_LEDS:

CONFIG_NCS_SAMPLE_MATTER_LEDS
  ``bool`` - Enable the LEDs module to be used.

.. _CONFIG_NCS_SAMPLE_MATTER_SETTINGS_SHELL:

CONFIG_NCS_SAMPLE_MATTER_SETTINGS_SHELL
  ``bool`` - Enable using :ref:`ug_matter_configuring_settings_shell`.

.. _CONFIG_NCS_SAMPLE_MATTER_TEST_SHELL:

CONFIG_NCS_SAMPLE_MATTER_TEST_SHELL
  ``bool`` - Enable support for test-specific shell commands in Matter applications.

.. _CONFIG_NCS_SAMPLE_MATTER_ZAP_FILE_PATH:

CONFIG_NCS_SAMPLE_MATTER_ZAP_FILE_PATH
  ``string`` - Set the absolute path under which ZAP file is located.

Diagnostics logs
================

You can configure the :ref:`ug_matter_configuration_diagnostic_logs` support for Matter applications using the following options:

.. _CONFIG_NCS_SAMPLE_MATTER_DIAGNOSTIC_LOGS:

CONFIG_NCS_SAMPLE_MATTER_DIAGNOSTIC_LOGS
  ``bool`` - Enable support for diagnostics logs.
  Diagnostics logs allow the Matter controller to read end-user, network, and crash logs from the Matter device.

.. _CONFIG_NCS_SAMPLE_MATTER_DIAGNOSTIC_LOGS_MAX_SIMULTANEOUS_SESSIONS:

CONFIG_NCS_SAMPLE_MATTER_DIAGNOSTIC_LOGS_MAX_SIMULTANEOUS_SESSIONS
  ``int`` - Define the maximum number of simultaneous sessions.

.. _CONFIG_NCS_SAMPLE_MATTER_DIAGNOSTIC_LOGS_CRASH_LOGS:

CONFIG_NCS_SAMPLE_MATTER_DIAGNOSTIC_LOGS_CRASH_LOGS
  ``bool`` - Enable support for storing crash logs when the crash occurs.

  Also check and configure the following option:

  .. _CONFIG_NCS_SAMPLE_MATTER_DIAGNOSTIC_LOGS_REMOVE_CRASH_AFTER_READ:

  CONFIG_NCS_SAMPLE_MATTER_DIAGNOSTIC_LOGS_REMOVE_CRASH_AFTER_READ
    ``bool`` - Set whether the last crash log is removed after it is read.
    Disable this option to read the last crash log multiple times.

.. _CONFIG_NCS_SAMPLE_MATTER_DIAGNOSTIC_LOGS_END_USER_LOGS:

CONFIG_NCS_SAMPLE_MATTER_DIAGNOSTIC_LOGS_END_USER_LOGS
	``bool`` - Enable support for capturing end-user diagnostic logs.

.. _CONFIG_NCS_SAMPLE_MATTER_DIAGNOSTIC_LOGS_NETWORK_LOGS:

CONFIG_NCS_SAMPLE_MATTER_DIAGNOSTIC_LOGS_NETWORK_LOGS
	``bool`` - Enable support for capturing network diagnostic logs.

.. _CONFIG_NCS_SAMPLE_MATTER_DIAGNOSTIC_LOGS_TEST:

ONFIG_NCS_SAMPLE_MATTER_DIAGNOSTIC_LOGS_TEST
	``bool`` - Enable the testing module for the diagnostic logs cluster.

.. _CONFIG_NCS_SAMPLE_MATTER_DIAGNOSTIC_LOGS_REDIRECT:

CONFIG_NCS_SAMPLE_MATTER_DIAGNOSTIC_LOGS_REDIRECT
	``bool`` - Enable the redirection of logs to the diagnostic logs module.

  This option enables only redirection of logs from the ``chip`` module (as diagnostic network logs) and from the ``app`` module (as diagnostic end-user logs).

Migration of operational keys
=============================

You can configure the migration of operational keys using the following options:

.. _CONFIG_NCS_SAMPLE_MATTER_OPERATIONAL_KEYS_MIGRATION_TO_ITS:

CONFIG_NCS_SAMPLE_MATTER_OPERATIONAL_KEYS_MIGRATION_TO_ITS
  ``bool`` - Enable migration of the operational keys stored in the persistent storage to the PSA ITS secure storage.

  Use this feature when you update the firmware of in-field devices that run the Mbed TLS cryptography backend to new firmware based on PSA Crypto API.

.. _CONFIG_NCS_SAMPLE_MATTER_FACTORY_RESET_ON_KEY_MIGRATION_FAILURE:

CONFIG_NCS_SAMPLE_MATTER_FACTORY_RESET_ON_KEY_MIGRATION_FAILURE
  ``bool`` - Enable the device to perform a factory reset if the operational key for the fabric has not been migrated properly to the PSA ITS secure storage.

Persistent storage
==================

You can configure :ref:`ug_matter_persistent_storage` using the following options:

.. _CONFIG_NCS_SAMPLE_MATTER_PERSISTENT_STORAGE:

CONFIG_NCS_SAMPLE_MATTER_PERSISTENT_STORAGE
  ``bool`` - Enable Matter persistent storage support.

  You must also enable one or both of the following Kconfig options to select which backend is used:

  .. _CONFIG_NCS_SAMPLE_MATTER_SETTINGS_STORAGE_BACKEND:

  CONFIG_NCS_SAMPLE_MATTER_SETTINGS_STORAGE_BACKEND
    ``bool`` - Enable a Zephyr settings-based storage implementation for Matter applications.

  .. _CONFIG_NCS_SAMPLE_MATTER_SECURE_STORAGE_BACKEND:

  CONFIG_NCS_SAMPLE_MATTER_SECURE_STORAGE_BACKEND
    ``bool`` - Enable the ARM PSA Protected Storage API implementation that imitates Zephyr Settings' key-value data format.

    * If building with CMSE enabled (``*/ns``), the TF-M and Secure Domain PSA Protected Storage implementation is used by default.
    * If building with CMSE disabled (``*/cpuapp``), the Trusted Storage library must be used.

.. _CONFIG_NCS_SAMPLE_MATTER_STORAGE_MAX_KEY_LEN:

CONFIG_NCS_SAMPLE_MATTER_STORAGE_MAX_KEY_LEN
	``int`` - Set the maximum length (in bytes) of the key under which the asset can be stored.

If you enabled the secure ARM PSA Protected Storage API implementation using :ref:`CONFIG_NCS_SAMPLE_MATTER_SECURE_STORAGE_BACKEND<CONFIG_NCS_SAMPLE_MATTER_SECURE_STORAGE_BACKEND>`, also check and configure the following options:

.. _CONFIG_NCS_SAMPLE_MATTER_SECURE_STORAGE_MAX_ENTRY_NUMBER:

CONFIG_NCS_SAMPLE_MATTER_SECURE_STORAGE_MAX_ENTRY_NUMBER
	``int`` - Set the maximum number of entries that can be stored securely.

.. _CONFIG_NCS_SAMPLE_MATTER_SECURE_STORAGE_PSA_KEY_VALUE_OFFSET:

CONFIG_NCS_SAMPLE_MATTER_SECURE_STORAGE_PSA_KEY_VALUE_OFFSET
	``hex`` - Set the PSA key offset dedicated for the Matter application.

Test event triggers
===================

You can configure :ref:`test event triggers <ug_matter_test_event_triggers>` using the following options:

.. _CONFIG_NCS_SAMPLE_MATTER_TEST_EVENT_TRIGGERS:

CONFIG_NCS_SAMPLE_MATTER_TEST_EVENT_TRIGGERS
  ``bool`` - Enable support for test event triggers.

.. _CONFIG_NCS_SAMPLE_MATTER_TEST_EVENT_TRIGGERS_MAX:

CONFIG_NCS_SAMPLE_MATTER_TEST_EVENT_TRIGGERS_MAX
  ``int`` - Define the maximum number of event triggers.

.. _CONFIG_NCS_SAMPLE_MATTER_TEST_EVENT_TRIGGERS_REGISTER_DEFAULTS:

CONFIG_NCS_SAMPLE_MATTER_TEST_EVENT_TRIGGERS_REGISTER_DEFAULTS
  ``bool`` - Automatically register default event triggers, such as factory reset, device reboot, and OTA start query.

.. _CONFIG_NCS_SAMPLE_MATTER_TEST_EVENT_TRIGGERS_MAX_TRIGGERS_DELEGATES:

CONFIG_NCS_SAMPLE_MATTER_TEST_EVENT_TRIGGERS_MAX_TRIGGERS_DELEGATES
  ``int`` - Define the maximum number of implementations of the ``TestEventTriggerDelegate`` class to be registered in the nRF test event triggers class.

Matter watchdog
===============

You can configure the :ref:`ug_matter_device_watchdog` feature using the following options:

.. _CONFIG_NCS_SAMPLE_MATTER_WATCHDOG:

CONFIG_NCS_SAMPLE_MATTER_WATCHDOG
	``bool`` - Enable the watchdog feature for Matter applications.

.. _CONFIG_NCS_SAMPLE_MATTER_WATCHDOG_PAUSE_IN_SLEEP:

CONFIG_NCS_SAMPLE_MATTER_WATCHDOG_PAUSE_IN_SLEEP
  ``bool`` - Pause the watchdog feature while the CPU is in the idle state.

.. _CONFIG_NCS_SAMPLE_MATTER_WATCHDOG_PAUSE_ON_DEBUG:

CONFIG_NCS_SAMPLE_MATTER_WATCHDOG_PAUSE_ON_DEBUG
  ``bool`` - Pause the watchdog feature while the CPU is halted by the debugger.

.. _CONFIG_NCS_SAMPLE_MATTER_WATCHDOG_DEFAULT:

CONFIG_NCS_SAMPLE_MATTER_WATCHDOG_DEFAULT
  ``bool`` - Use the default watchdog objects that are created in the :file:`matter_init.cpp` file.
  These watchdog objects are dedicated for the Main and Matter threads, and initialized to value of the :ref:`CONFIG_NCS_SAMPLE_MATTER_WATCHDOG_DEFAULT_FEED_TIME<CONFIG_NCS_SAMPLE_MATTER_WATCHDOG_DEFAULT_FEED_TIME>` Kconfig option.

.. _CONFIG_NCS_SAMPLE_MATTER_WATCHDOG_DEFAULT_FEED_TIME:

CONFIG_NCS_SAMPLE_MATTER_WATCHDOG_DEFAULT_FEED_TIME
  ``int`` - Set the default interval (in milliseconds) for calling the feeding callback, if it exists.

.. _CONFIG_NCS_SAMPLE_MATTER_WATCHDOG_TIMEOUT:

CONFIG_NCS_SAMPLE_MATTER_WATCHDOG_TIMEOUT
  ``int`` - Set the default maximum time window (in milliseconds) for receiving the feeding signal.
  The feeding signal must be received from all created watchdog sources to reset the watchdog object's timer.

.. _CONFIG_NCS_SAMPLE_MATTER_WATCHDOG_EVENT_TRIGGERS:

CONFIG_NCS_SAMPLE_MATTER_WATCHDOG_EVENT_TRIGGERS
  ``bool`` - Enable the default test event triggers that are used for watchdog-testing purposes.

Snippets
********

Matter samples provide predefined :ref:`zephyr:snippets` for typical use cases.
The snippets are in the :file:`nrf/snippets` directory of the |NCS|.
For more information about using snippets, see :ref:`zephyr:using-snippets` in the Zephyr documentation.

Specify the corresponding snippet names in the :makevar:`SNIPPET` CMake option for the application configuration.
The following is an example command for the ``nrf52840dk/nrf52840`` board target that adds the ``diagnostic-logs`` snippet to the :ref:`matter_lock_sample` sample:

.. code-block::

   west build -b nrf52840dk/nrf52840 -- -Dlock_SNIPPET=diagnostic-logs

The following snippets are available:

* ``diagnostic-logs`` - Enables the set of configurations needed for full Matter diagnostic logs support.
  See :ref:`ug_matter_diagnostic_logs_snippet` in the Matter protocol section for more information.
