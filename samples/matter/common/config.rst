.. _matter_samples_config:

Shared configurations in Matter samples
#######################################

.. _matter_samples_kconfig:

Configuration options
*********************

.. options-from-kconfig:: /../../../../../nrf/samples/matter/common/src/Kconfig
   :show-type:

Diagnostics logs
================

.. options-from-kconfig:: /../../../../../nrf/samples/matter/common/src/diagnostic/Kconfig
   :show-type:

Persistent storage
==================

.. options-from-kconfig:: /../../../../../nrf/samples/matter/common/src/persistent_storage/Kconfig
   :show-type:

Matter watchdog
===============

.. options-from-kconfig:: /../../../../../nrf/samples/matter/common/src/watchdog/Kconfig
   :show-type:

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

.. _matter_stack_config:

Shared configurations in Matter stack
#####################################

Base Configuration
******************

Zephyr configuration options (inherited by nRF Connect)
=======================================================

.. options-from-kconfig:: /../../../../../modules/lib/matter/config/zephyr/chip-module/Kconfig
   :show-type:

nRF Connect Configuration
=========================

.. options-from-kconfig:: /../../../../../modules/lib/matter/config/nrfconnect/chip-module/Kconfig
   :show-type:

Matter features
***************

.. options-from-kconfig:: /../../../../../modules/lib/matter/config/nrfconnect/chip-module/Kconfig.features
   :show-type:
