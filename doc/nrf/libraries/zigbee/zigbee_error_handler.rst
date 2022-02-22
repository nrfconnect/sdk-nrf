.. _lib_zigbee_error_handler:

Zigbee error handler
####################

.. contents::
   :local:
   :depth: 2

The Zigbee error handler library provides a set of macros that can be used to assert on nrfxlib's :ref:`nrfxlib:zboss` API return codes.
The assertion is implemented by a call to the :c:func:`zb_osif_abort` function.
Additionally, you can enable the library to log the error code name before the assertion.

Functions that resolve the numeric error code to printable string are defined in the :file:`zb_error_to_string.c` source file in nrfxlib.

.. _lib_zigbee_error_handler_options:

Configuration
*************

To use this library, include its header in your application and pass the error code that should be checked using its macros.

Additionally, if you want the error code to be printed to log, enable the :kconfig:option:`CONFIG_LOG` and :kconfig:option:`CONFIG_ZBOSS_ERROR_PRINT_TO_LOG` Kconfig options.
The :kconfig:option:`CONFIG_ZBOSS_ERROR_PRINT_TO_LOG` option automatically enables the :kconfig:option:`CONFIG_ZIGBEE_ERROR_TO_STRING_ENABLED` Kconfig option, which obtains the ZBOSS error code name.

The library also provides a macro for checking the value returned by the :c:func:`bdb_start_top_level_commissioning` function.

API documentation
*****************

| Header file: :file:`include/zigbee/zigbee_error_handler.h`
| Source file: :file:`../nrfxlib/zboss/src/zb_error/zb_error_to_string.c`

.. doxygengroup:: zb_error
   :project: nrf
   :members:
