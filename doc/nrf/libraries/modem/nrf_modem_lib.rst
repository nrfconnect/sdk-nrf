.. _nrf_modem_lib_readme:

Modem library integration layer
###############################

.. contents::
   :local:
   :depth: 2


The Modem library integration layer handles the integration of the Modem library into |NCS|.
The integration layer is constituted by the library wrapper and functionalities like socket offloading, OS abstraction, memory reservation by the Partition manager, handling modem traces, and diagnostics.

Library wrapper
***************

The library wrapper provides an encapsulation over the core Modem library functions such as initialization and shutdown.
The library wrapper is implemented in :file:`nrf/lib/nrf_modem_lib/nrf_modem_lib.c`.

The library wrapper encapsulates the :c:func:`nrf_modem_init` and :c:func:`nrf_modem_shutdown` calls of the Modem library with :c:func:`nrf_modem_lib_init` and :c:func:`nrf_modem_lib_shutdown` calls, respectively.
The library wrapper eases the task of initializing the Modem library by automatically passing the size and address of all the shared memory regions of the Modem library to the :c:func:`nrf_modem_init` call.

:ref:`partition_manager` is the component that reserves the RAM memory for the shared memory regions used by the Modem library.
For more information, see :ref:`partition_mgr_integration`.

The library wrapper can initialize the Modem library during system initialization using :c:macro:`SYS_INIT`.
The :kconfig:option:`CONFIG_NRF_MODEM_LIB_SYS_INIT` Kconfig option can be used to control this setting.
Some libraries in |NCS|, such as the :ref:`lte_lc_readme` have similar configuration options to initialize during system initialization and these options depend on the configuration option of the integration layer.
If your application performs an update of the nRF9160 modem firmware, you can disable this functionality to have full control on the initialization of the library.

:kconfig:option:`CONFIG_NRF_MODEM_LIB_LOG_FW_VERSION_UUID` can be enabled for printing logs of both FW version and UUID at the end of the library initialization step.

When using the Modem library in |NCS|, the library must be initialized and shutdown using the :c:func:`nrf_modem_lib_init` and :c:func:`nrf_modem_lib_shutdown` function calls, respectively.

Callbacks
=========

The library wrapper also provides callbacks for the initialization and shutdown operations.
The application can set up a callback for :c:func:`nrf_modem_lib_init` function using the :c:macro:`NRF_MODEM_LIB_ON_INIT` macro, and a callback for :c:func:`nrf_modem_lib_shutdown` function using the :c:macro:`NRF_MODEM_LIB_ON_SHUTDOWN` macro.
These compile-time callbacks allow any part of the application to perform any setup steps that require the modem to be in a certain state.
Furthermore, the callbacks ensure that the setup steps are repeated whenever another part of the application turns the modem on or off.
The callbacks registered using :c:macro:`NRF_MODEM_LIB_ON_INIT` are executed after the library is initialized.
The result of the initialization and the callback context are provided to these callbacks.
Callbacks for the macro :c:macro:`NRF_MODEM_LIB_ON_INIT` must have the signature ``void callback_name(int ret, void *ctx)``, where ``ret`` is the result of the initialization and ``ctx`` is the context passed to the macro.
The callbacks registered using :c:macro:`NRF_MODEM_LIB_ON_SHUTDOWN` are executed before the library is shut down.
The callback context is provided to these callbacks.
Callbacks for the macro :c:macro:`NRF_MODEM_LIB_ON_SHUTDOWN` must have the signature ``void callback_name(void *ctx)``, where ``ctx`` is the context passed to the macro.
See the :ref:`modem_callbacks_sample` sample for more information.


Socket offloading
*****************

Zephyr Socket API offers the :ref:`socket offloading functionality <zephyr:net_socket_offloading>` to redirect or *offload* function calls to BSD socket APIs such as ``socket()`` and ``send()``.
The integration layer utilizes this functionality to offload the socket API calls to the Modem library and thus eases the task of porting the networking code to the nRF9160 by providing a wrapper for Modem library's native socket API such as :c:func:`nrf_socket` and :c:func:`nrf_send`.

The socket offloading functionality in the integration layer is implemented in :file:`nrf/lib/nrf_modem_lib/nrf91_sockets.c`.

Modem library socket API sets errnos as defined in :file:`nrf_errno.h`.
The socket offloading support in the integration layer in |NCS| converts those errnos to the errnos that adhere to the selected C library implementation.

The socket offloading functionality is enabled by default.
To disable the functionality, disable the :kconfig:option:`CONFIG_NET_SOCKETS_OFFLOAD` Kconfig option in your project configuration.
If you disable the socket offloading functionality, the socket calls will no longer be offloaded to the nRF9160 modem firmware.
Instead, the calls will be relayed to the native Zephyr TCP/IP implementation.
This can be useful to switch between an emulator and a real device while running networking code on these devices.
Note that the even if the socket offloading is disabled, Modem library's own socket APIs such as :c:func:`nrf_socket` and :c:func:`nrf_send` remain available.

OS abstraction layer
********************

The Modem library requires the implementation of an OS abstraction layer, which is an interface over the operating system functionalities such as interrupt setup, threads, and heap.
The integration layer provides an implementation of the OS abstraction layer using |NCS| components.
The OS abstraction layer is implemented in the :file:`nrfxlib/nrf_modem/include/nrf_modem_os.c`.

The behavior of the functions in the OS abstraction layer is dependent on the |NCS| components that are used in their implementation.
This is relevant for functions such as :c:func:`nrf_modem_os_shm_tx_alloc`, which uses :ref:`Zephyr's Heap implementation <zephyr:heap_v2>` to dynamically allocate memory.
In this case, the characteristics of the allocations made by these functions depend on the heap implementation by Zephyr.

.. _modem_trace_module:

Modem trace module
******************
To enable the tracing functionality, enable the :kconfig:option:`CONFIG_NRF_MODEM_LIB_TRACE` Kconfig in your project configuration.
The module is implemented in :file:`nrf/lib/nrf_modem_lib/nrf_modem_lib_trace.c` and consists of a thread that initializes, deinitializes, and forwards modem traces to a backend that can be selected by enabling any one of the following Kconfig options:

* :kconfig:option:`CONFIG_NRF_MODEM_LIB_TRACE_BACKEND_UART` to send modem traces over UARTE1
* :kconfig:option:`CONFIG_NRF_MODEM_LIB_TRACE_BACKEND_RTT` to send modem traces over SEGGER RTT

To reduce the amount of trace data sent from the modem, a different trace level can be selected.
Complete the following steps to configure the modem trace level at compile time:

#. Enable the :kconfig:option:`CONFIG_NRF_MODEM_LIB_TRACE_LEVEL_OVERRIDE` option in your project configuration.
#. Enable any one of the following Kconfig options by setting it to ``y`` in your project configuration:

   * :kconfig:option:`CONFIG_NRF_MODEM_LIB_TRACE_LEVEL_OFF`
   * :kconfig:option:`CONFIG_NRF_MODEM_LIB_TRACE_LEVEL_FULL`
   * :kconfig:option:`CONFIG_NRF_MODEM_LIB_TRACE_LEVEL_LTE_AND_IP`
   * :kconfig:option:`CONFIG_NRF_MODEM_LIB_TRACE_LEVEL_IP_ONLY`
   * :kconfig:option:`CONFIG_NRF_MODEM_LIB_TRACE_LEVEL_COREDUMP_ONLY`

The application can use the :c:func:`nrf_modem_lib_trace_level_set` function to set the desired trace level.
Passing ``NRF_MODEM_LIB_TRACE_LEVEL_OFF`` to the :c:func:`nrf_modem_lib_trace_level_set` function disables trace output.

During tracing, the integration layer ensures that modem traces are always flushed before the Modem library is re-initialized (including when the modem has crashed).
The application can synchronize with the flushing of modem traces by calling the :c:func:`nrf_modem_lib_trace_processing_done_wait` function.

To enable the measurement of the modem trace backend bitrate, enable the :kconfig:option:`CONFIG_NRF_MODEM_LIB_TRACE_BACKEND_BITRATE` Kconfig in your project configuration.
After enabling this Kconfig option, the application can use the :c:func:`nrf_modem_lib_trace_backend_bitrate_get` function to retrieve the rolling average bitrate of the modem trace backend, measured over the period defined by the :kconfig:option:`CONFIG_NRF_MODEM_LIB_TRACE_BACKEND_BITRATE_PERIOD_MS` Kconfig option.
To enable logging of the modem trace backend bitrate, enable the :kconfig:option:`CONFIG_NRF_MODEM_LIB_TRACE_BACKEND_BITRATE_LOG` Kconfig.
The logging happens at an interval set by the :kconfig:option:`CONFIG_NRF_MODEM_LIB_TRACE_BACKEND_BITRATE_LOG_PERIOD_MS` Kconfig option.
If the difference in the values of the :kconfig:option:`CONFIG_NRF_MODEM_LIB_TRACE_BACKEND_BITRATE_PERIOD_MS` Kconfig option and the :kconfig:option:`CONFIG_NRF_MODEM_LIB_TRACE_BACKEND_BITRATE_LOG_PERIOD_MS` Kconfig option is very high, you can sometimes observe high variation in measurements due to the short period over which the rolling average is calculated.

To enable logging of the modem trace bitrate, enable the :kconfig:option:`CONFIG_NRF_MODEM_LIB_TRACE_BITRATE_LOG` Kconfig.

.. _adding_custom_modem_trace_backends:

Adding custom trace backends
============================

You can add custom trace backends if the existing trace backends are not sufficient.
At any time, only one trace backend can be compiled with the application.
The value of the :kconfig:option:`CONFIG_NRF_MODEM_LIB_TRACE_BACKEND` Kconfig option determines which trace backend is compiled.
The :ref:`modem_trace_backend_sample` sample demonstrates how a custom trace backend can be added to an application.

Complete the following steps to add a custom trace backend:

1. Place the files that have the custom trace backend implementation in a library or an application you create.
   For example, the implementation of the UART trace backend (default) can be found in the :file:`nrf/lib/nrf_modem_lib/trace_backends/uart/uart.c` file.

#. Add a C file implementing the interface in :file:`nrf/include/modem/trace_backend.h` header file.

   .. code-block:: c

      /* my_trace_medium.c */

      #include <modem/trace_medium.h>

      int trace_medium_init(void)
      {
           /* initialize transport medium here */
           return 0;
      }

      int trace_medium_deinit(void)
      {
           /* optional deinitialization code here */
           return 0;
      }

      int trace_medium_write(const void *data, size_t len)
      {
           /* forward data over custom transport here */
           /* return number of bytes written or negative error code on failure */
           return 0;
      }

#. Create or modify a :file:`Kconfig` file to extend the choice :kconfig:option:`NRF_MODEM_LIB_TRACE_BACKEND` with another option.

   .. code-block:: Kconfig

      if NRF_MODEM_LIB_TRACE

      # Extends the choice with another backend
      choice NRF_MODEM_LIB_TRACE_BACKEND

      config NRF_MODEM_LIB_TRACE_BACKEND_MY_TRACE_BACKEND
              bool "My trace backend"
              help
                Optional description of my
                trace backend.

      endchoice

      endif

#. Create or modify a :file:`CMakeLists.txt` file, adding the custom trace backend sources only if the custom trace backend option has been chosen.

   .. code-block:: cmake

      if(CONFIG_NRF_MODEM_LIB_TRACE)

      zephyr_library()

      # Only add 'custom' backend to compilation when selected.
      zephyr_library_sources_ifdef(
        CONFIG_NRF_MODEM_LIB_TRACE_BACKEND_MY_TRACE_BACKEND
        path/to/my_trace_backend.c
      )

      endif()

#. Include the :file:`Kconfig` file and the :file:`CMakeLists.txt` file to the build.
#. Add the following Kconfig options to your application's :file:`prj.conf` file to use the custom modem trace backend:

   .. code-block:: none

      CONFIG_NRF_MODEM_LIB_TRACE=y
      CONFIG_NRF_MODEM_LIB_TRACE_BACKEND_MY_TRACE_BACKEND=y

.. _modem_trace_backend_uart_custom_board:

Sending traces over UART1 on a custom board
===========================================

When sending modem traces over UART1 on a custom board, configuration must be added for the UART1 device in the devicetree.
This is done by adding the following code snippet to the board devicetree or overlay file, where the pin numbers (``0``, ``1``, ``14``, and ``15``) must be updated to match your board.

.. code-block:: dts

   &pinctrl {
      uart1_default: uart1_default {
         group1 {
            psels = <NRF_PSEL(UART_TX, 0, 1)>,
               <NRF_PSEL(UART_RTS, 0, 14)>;
         };
         group2 {
            psels = <NRF_PSEL(UART_RX, 0, 0)>,
               <NRF_PSEL(UART_CTS, 0, 15)>;
            bias-pull-up;
         };
      };

      uart1_sleep: uart1_sleep {
         group1 {
            psels = <NRF_PSEL(UART_TX, 0, 1)>,
               <NRF_PSEL(UART_RX, 0, 0)>,
               <NRF_PSEL(UART_RTS, 0, 14)>,
               <NRF_PSEL(UART_CTS, 0, 15)>;
            low-power-enable;
         };
      };
   };

   &uart1 {
      ...
      pinctrl-0 = <&uart1_default>;
      pinctrl-1 = <&uart1_sleep>;
      pinctrl-names = "default", "sleep";
      ...
   };

The UART trace backends allow the pins and UART1 interrupt priority to be set using the devicetree.
Other configurations set in the devicetree, such as the current speed, are overwritten by the UART trace backends.

.. note::

   When one of the UART trace backends is enabled by either the Kconfig option :kconfig:option:`CONFIG_NRF_MODEM_LIB_TRACE_BACKEND_UART` or :kconfig:option:`CONFIG_NRF_MODEM_LIB_TRACE_BACKEND_UART_SYNC`, it initializes the UART1 driver, regardless of its status in the devicetree.

Modem fault handling
********************
If a fault occurs in the modem, the application is notified through the fault handler function that is registered with the Modem library during initialization.
This lets the application read the fault reason (in some cases the modem's program counter) and take the appropriate action.

On initialization (using :c:func:`nrf_modem_lib_init`), the Modem library integration layer registers the :c:func:`nrf_modem_fault_handler` function through the Modem library initialization parameters.
The behavior of the :c:func:`nrf_modem_fault_handler` function is controlled with the three following KConfig options:

* :kconfig:option:`CONFIG_NRF_MODEM_LIB_ON_FAULT_DO_NOTHING` - This option lets the fault handler log the Modem fault and return (default).
* :kconfig:option:`CONFIG_NRF_MODEM_LIB_ON_FAULT_RESET_MODEM` - This option lets the fault handler schedule a workqueue task to reinitialize the modem and Modem library.
* :kconfig:option:`CONFIG_NRF_MODEM_LIB_ON_FAULT_APPLICATION_SPECIFIC` - This option lets the fault handler function :c:func:`nrf_modem_fault_handler` be defined by the application, outside of the Modem library integration layer.

Implementing a custom fault handler
===================================

If you want to implement a custom fault handler, enable the :kconfig:option:`CONFIG_NRF_MODEM_LIB_ON_FAULT_APPLICATION_SPECIFIC` Kconfig option and provide an implementation of the :c:func:`nrf_modem_fault_handler` function, considering the following points:

* The fault handler is called in an interrupt context.
* Re-initialization of the Modem library must be done outside of the fault handler.

.. _partition_mgr_integration:

Partition manager integration
*****************************

The Modem library, which runs on the application core, shares an area of RAM memory with the nRF9160 modem core.
During the initialization, the Modem library accepts the boundaries of this area of RAM and configures the communication with the modem core accordingly.

However, it is the responsibility of the application to reserve that RAM during linking, so that this memory area is not used for other purposes and remain dedicated for use by the Modem library.

In |NCS|, the application can configure the size of the memory area dedicated to the Modem library through the integration layer.
The integration layer provides a set of Kconfig options that help the application reserve the required amount of memory for the Modem library by integrating with another |NCS| component, the :ref:`partition_manager`.

The RAM area that the Modem library shares with the nRF9160 modem core is divided into the following four regions:

* Control
* RX
* TX
* Trace

The size of the RX, TX and the Trace regions can be configured by the following Kconfig options of the integration layer:

* :kconfig:option:`CONFIG_NRF_MODEM_LIB_SHMEM_RX_SIZE` for the RX region
* :kconfig:option:`CONFIG_NRF_MODEM_LIB_SHMEM_TX_SIZE` for the TX region
* :kconfig:option:`CONFIG_NRF_MODEM_LIB_SHMEM_TRACE_SIZE` for the Trace region

The size of the Control region is fixed.
The Modem library exports the size value through :kconfig:option:`CONFIG_NRF_MODEM_SHMEM_CTRL_SIZE`.
This value is automatically passed by the integration layer to the library during the initialization through :c:func:`nrf_modem_lib_init`.

When the application is built using CMake, the :ref:`partition_manager` automatically reads the Kconfig options of the integration layer.
Partition manager decides about the placement of the regions in RAM and reserves memory according to the given size.
As a result, the Partition manager generates the following parameters:

* ``PM_NRF_MODEM_LIB_CTRL_ADDRESS`` - Address of the Control region
* ``PM_NRF_MODEM_LIB_TX_ADDRESS`` - Address of the TX region
* ``PM_NRF_MODEM_LIB_RX_ADDRESS`` - Address of the RX region
* ``PM_NRF_MODEM_LIB_TRACE_ADDRESS`` - Address of the Trace region

Partition manager also generates the following additional parameters:

* ``PM_NRF_MODEM_LIB_CTRL_SIZE`` - Size of the Control region
* ``PM_NRF_MODEM_LIB_TX_SIZE`` - Size of the TX region
* ``PM_NRF_MODEM_LIB_RX_SIZE`` - Size of the RX region
* ``PM_NRF_MODEM_LIB_TRACE_SIZE`` - Size of the Trace region

These parameters will have identical values as the ``CONFIG_NRF_MODEM_LIB_SHMEM_*_SIZE`` configuration options.

When the Modem library is initialized by the integration layer in |NCS|, the integration layer automatically passes the boundaries of each shared memory region to the Modem library during the :c:func:`nrf_modem_lib_init` call.

Diagnostic functionality
************************

The Modem library integration layer in |NCS| provides some memory diagnostic functionality that is enabled by the :kconfig:option:`CONFIG_NRF_MODEM_LIB_MEM_DIAG` option.

The application can retrieve runtime statistics for the library and TX memory region heaps by enabling the :kconfig:option:`CONFIG_NRF_MODEM_LIB_MEM_DIAG` option and calling the :c:func:`nrf_modem_lib_diag_stats_get` function.
The application can schedule a periodic report of the runtime statistics of the library and TX memory region heaps, by enabling the :kconfig:option:`CONFIG_NRF_MODEM_LIB_MEM_DIAG_DUMP` option.
The application can log the allocations on the Modem library heap and the TX memory region by enabling the :kconfig:option:`CONFIG_NRF_MODEM_LIB_MEM_DIAG_ALLOC` option.

API documentation
*****************

| Header file: :file:`include/modem/nrf_modem_lib.h`, :file:`include/modem/nrf_modem_lib_trace.h`
| Source file: :file:`lib/nrf_modem_lib.c`

.. doxygengroup:: nrf_modem_lib
   :project: nrf
   :members:

.. doxygengroup:: nrf_modem_lib_trace
   :project: nrf
   :members:
