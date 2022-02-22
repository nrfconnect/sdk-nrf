.. _lib_fatal_error:

Fatal error handler
###################

.. contents::
   :local:
   :depth: 2

The |NCS| provides an implementation of the fatal error handler that overrides Zephyr's default fatal error handling implementation.

Overview
********

The library defines the :c:func:`k_sys_fatal_error_handler()` function, which is declared in Zephyr's :ref:`fatal error handling API <zephyr:fatal>`.
Its implementation in the |NCS| is standardized for most samples.

Configuration
*************

When building for an embedded target, the default behavior of the :c:func:`k_sys_fatal_error_handler()` function in case of a fatal error is to reboot the application.
You can modify the default behavior of the library not to reboot the application.
To have the application enter an endless loop, change the :kconfig:option:`CONFIG_RESET_ON_FATAL_ERROR` Kconfig option to ``n``.

Library files
*************

This library can be found under :file:`lib/fatal_error/` in the |NCS| folder structure.
The Zephyr file it overrides is :file:`fatal.c` under :file:`zephyr/kernel/` in the |NCS| folder structure.

API documentation
*****************

.. note::
   This library is an implementation of Zephyr's :ref:`fatal error handling API <zephyr:fatal>` and does not have a separate API documentation in the |NCS|.

| Header file: :file:`zephyr/include/fatal.h`
| Source file: :file:`lib/fatal_error/fatal_error.c`
