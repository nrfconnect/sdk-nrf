.. _nrf_modem_lib_wrapper:

Library wrapper
###############

.. contents::
   :local:
   :depth: 2

The library wrapper provides an encapsulation over the core Modem library functions such as initialization and shutdown.
The library wrapper is implemented in :file:`nrf/lib/nrf_modem_lib/nrf_modem_lib.c`.

The library wrapper encapsulates the :c:func:`nrf_modem_init` and :c:func:`nrf_modem_shutdown` calls of the Modem library with :c:func:`nrf_modem_lib_init` and :c:func:`nrf_modem_lib_shutdown` calls, respectively.
The library wrapper eases the task of initializing the Modem library by automatically passing the size and address of all the shared memory regions of the Modem library to the :c:func:`nrf_modem_init` call.

:ref:`partition_manager` is the component that reserves the RAM memory for the shared memory regions used by the Modem library.
For more information, see :ref:`partition_mgr_integration`.

The :kconfig:option:`CONFIG_NRF_MODEM_LIB_LOG_FW_VERSION_UUID` option can be enabled for printing logs of both FW version and UUID at the end of the library initialization step.

When using the Modem library in |NCS|, the library must be initialized and shutdown using the :c:func:`nrf_modem_lib_init` and :c:func:`nrf_modem_lib_shutdown` function calls, respectively.

.. _mlil_callbacks:

Modem library callbacks
***********************

The library wrapper provides callbacks for the modem initialization and shutdown operations, as well as macros for registering multiple callbacks on changes to the modem functional mode.

Modem initialization and shutdown
=================================

The application can set up a callback for the :c:func:`nrf_modem_lib_init` function using the :c:macro:`NRF_MODEM_LIB_ON_INIT` macro, and a callback for :c:func:`nrf_modem_lib_shutdown` function using the :c:macro:`NRF_MODEM_LIB_ON_SHUTDOWN` macro.
These compile-time callbacks allow any part of the application to perform any setup steps that require the modem to be in a certain state.
Furthermore, the callbacks ensure that the setup steps are repeated whenever another part of the application turns the modem on or off.
The callbacks registered using :c:macro:`NRF_MODEM_LIB_ON_INIT` are executed after the library is initialized.
The result of the initialization and the callback context are provided to these callbacks.

.. note::
  The callback can be used to perform modem and library configurations that require the modem to be turned on in offline mode.
  The callback cannot be used to change the modem's functional mode.
  Calls to :c:func:`lte_lc_connect` and ``CFUN`` AT calls are not allowed, and must be done after :c:func:`nrf_modem_lib_init` has returned.
  If a library needs to perform operations after the link is up, it can use the :ref:`lte_lc_readme` and subscribe to a :c:macro:`LTE_LC_ON_CFUN` callback.

Callbacks for the macro :c:macro:`NRF_MODEM_LIB_ON_INIT` must have the signature ``void callback_name(int ret, void *ctx)``, where ``ret`` is the result of the initialization and ``ctx`` is the context passed to the macro.
The callbacks registered using the :c:macro:`NRF_MODEM_LIB_ON_SHUTDOWN` macro are executed before the library is shut down.
The callback context is provided to these callbacks.
Callbacks for the macro :c:macro:`NRF_MODEM_LIB_ON_SHUTDOWN` must have the signature ``void callback_name(void *ctx)``, where ``ctx`` is the context passed to the macro.
See the :ref:`modem_callbacks_sample` sample for more information.

Modem functional mode changes
=============================

The application can set up a callback for AT CFUN calls using the :c:macro:`NRF_MODEM_LIB_ON_CFUN` macro.
This compile-time callback allows any part of the application to perform any steps that are required when the modem changes functional mode.
The callbacks registered using the :c:macro:`NRF_MODEM_LIB_ON_CFUN` macro are executed after the functional mode has been successfully set.
See the :ref:`modem_callbacks_sample` sample for more information.

.. important::
   The application should not call :c:func:`nrf_modem_at_cfun_handler_set` as that will override the handler set by the modem library integration layer.
   Instead, the application should use the :c:macro:`NRF_MODEM_LIB_ON_CFUN` macro to register functional mode changes.

.. note::
   The CFUN callback is not supported with :c:func:`nrf_modem_at_cmd_async`.
