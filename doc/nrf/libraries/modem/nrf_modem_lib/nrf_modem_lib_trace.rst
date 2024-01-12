.. _modem_trace_module:

Modem trace module
##################

.. contents::
   :local:
   :depth: 2

To enable the tracing functionality, enable the :kconfig:option:`CONFIG_NRF_MODEM_LIB_TRACE` Kconfig in your project configuration.
The module is implemented in :file:`nrf/lib/nrf_modem_lib/nrf_modem_lib_trace.c` and consists of a thread that initializes, deinitializes, and forwards modem traces to a backend.
The trace backend can be selected in one of the following ways:

* Adding the ``nrf91-modem-trace-uart`` snippet to send modem traces over UART.
  See :ref:`nrf91_modem_trace_uart_snippet` for more details.
* Enabling the :kconfig:option:`CONFIG_NRF_MODEM_LIB_TRACE_BACKEND_RTT` Kconfig option to send modem traces over SEGGER RTT.
* Enabling the :kconfig:option:`CONFIG_NRF_MODEM_LIB_TRACE_BACKEND_FLASH` Kconfig option to write modem traces to external flash.

To reduce the amount of trace data sent from the modem, a different trace level can be selected.
Complete the following steps to configure the modem trace level at compile time:

#. Enable the :kconfig:option:`CONFIG_NRF_MODEM_LIB_TRACE_LEVEL_OVERRIDE` option in your project configuration.
#. Enable any one of the following Kconfig options by setting it to ``y`` in your project configuration:

.. _trace_level_options:

   * :kconfig:option:`CONFIG_NRF_MODEM_LIB_TRACE_LEVEL_OFF`
   * :kconfig:option:`CONFIG_NRF_MODEM_LIB_TRACE_LEVEL_FULL`
   * :kconfig:option:`CONFIG_NRF_MODEM_LIB_TRACE_LEVEL_LTE_AND_IP`
   * :kconfig:option:`CONFIG_NRF_MODEM_LIB_TRACE_LEVEL_IP_ONLY`
   * :kconfig:option:`CONFIG_NRF_MODEM_LIB_TRACE_LEVEL_COREDUMP_ONLY`

The application can use the :c:func:`nrf_modem_lib_trace_level_set` function to set the desired trace level.
Passing :c:enumerator:`NRF_MODEM_LIB_TRACE_LEVEL_OFF` to the :c:func:`nrf_modem_lib_trace_level_set` function disables trace output.

.. note::
   The modem stores the current trace level on passing the ``AT+CFUN=0`` command.
   If the trace level stored in the modem is :c:enumerator:`NRF_MODEM_LIB_TRACE_LEVEL_OFF`, the application must enable traces in the modem using the :c:func:`nrf_modem_lib_trace_level_set` function or by enabling any of the :ref:`aforementioned Kconfig options <trace_level_options>`.

During tracing, the integration layer ensures that modem traces are always flushed before the Modem library is re-initialized (including when the modem has crashed).
The application can synchronize with the flushing of modem traces by calling the :c:func:`nrf_modem_lib_trace_processing_done_wait` function.

For trace backends that support storing of trace data, the application can be notified using the :c:func:`nrf_modem_lib_trace_callback` function if the trace storage becomes full.
The :c:func:`nrf_modem_lib_trace_callback` must be defined in the application if the :kconfig:option:`CONFIG_NRF_MODEM_TRACE_FLASH_NOSPACE_SIGNAL` Kconfig option is enabled.
In this case, the application is responsible for reading the trace data with the :c:func:`nrf_modem_lib_trace_read` function if required, before clearing the trace backend storage by calling the :c:func:`nrf_modem_lib_trace_clear` function.
It is not necessary to turn off modem tracing.
However, it is expected that the modem will drop traces when the backend becomes full.

To enable the measurement of the modem trace backend bitrate, enable the :kconfig:option:`CONFIG_NRF_MODEM_LIB_TRACE_BACKEND_BITRATE` Kconfig in your project configuration.
After enabling this Kconfig option, the application can use the :c:func:`nrf_modem_lib_trace_backend_bitrate_get` function to retrieve the rolling average bitrate of the modem trace backend, measured over the period defined by the :kconfig:option:`CONFIG_NRF_MODEM_LIB_TRACE_BACKEND_BITRATE_PERIOD_MS` Kconfig option.
To enable logging of the modem trace backend bitrate, enable the :kconfig:option:`CONFIG_NRF_MODEM_LIB_TRACE_BACKEND_BITRATE_LOG` Kconfig option.
The logging happens at an interval set by the :kconfig:option:`CONFIG_NRF_MODEM_LIB_TRACE_BACKEND_BITRATE_LOG_PERIOD_MS` Kconfig option.
If the difference in the values of the :kconfig:option:`CONFIG_NRF_MODEM_LIB_TRACE_BACKEND_BITRATE_PERIOD_MS` and :kconfig:option:`CONFIG_NRF_MODEM_LIB_TRACE_BACKEND_BITRATE_LOG_PERIOD_MS` Kconfig options is very high, you can sometimes observe high variation in measurements due to the short period over which the rolling average is calculated.

To enable logging of the modem trace bitrate, use the :kconfig:option:`CONFIG_NRF_MODEM_LIB_TRACE_BITRATE_LOG` Kconfig option.

.. _modem_trace_flash_backend:

Modem trace flash backend
*************************

The flash backend stores :ref:`modem traces <modem_trace_module>` to the external flash storage on the nRF91 Series DK.

First, set up the :ref:`external flash <nrf9160_external_flash>` for your application.
You can then set the following configuration options for the application to decide how to handle when the flash is full:

   * :kconfig:option:`CONFIG_NRF_MODEM_TRACE_FLASH_NOSPACE_SIGNAL` - To get notified with a callback when the flash is full, and the application erases or sends the data to the cloud.
   * :kconfig:option:`CONFIG_NRF_MODEM_TRACE_FLASH_NOSPACE_ERASE_OLDEST` - To automatically erase the oldest sector in the flash circular buffer.
     The erase operation takes some time.
     If the operation takes too long, traces are dropped by the modem.

You can also increase heap and stack sizes when using the modem trace flash backend by setting values for the following configuration options:

* :kconfig:option:`CONFIG_HEAP_MEM_POOL_SIZE` = ``2048``
* :kconfig:option:`CONFIG_MAIN_STACK_SIZE` = ``4096``
* :kconfig:option:`CONFIG_SYSTEM_WORKQUEUE_STACK_SIZE` = ``4096``
* :kconfig:option:`CONFIG_NRF_MODEM_LIB_TRACE_STACK_SIZE` = ``4096``

The modem trace flash backend has some additional configuration options:

* :kconfig:option:`CONFIG_FCB` - Required for the flash circular buffer used in the backend.
* :kconfig:option:`CONFIG_NRF_MODEM_LIB_TRACE_BACKEND_FLASH_PARTITION_SIZE` - Defines the space to be used for the modem trace partition.
  The external flash size on the nRF9160 DK is 8 MB (equal to ``0x800000`` in HEX).

It is also recommended to enable high drive mode and high-performance mode in devicetree.
High drive is to ensure that the communication with the flash device is reliable at high speed.
High-performance mode is a feature in the flash device that allows it to write and erase faster than in low-power mode.
See the :ref:`external flash <nrf9160_external_flash>` documentation for more details.
The trace backend needs to handle trace data at ~1 Mbps to avoid filling up the buffer in the modem.
If the modem buffer is full, the modem drops modem traces until the buffer has space available again.

.. _modem_trace_backend_uart_nrf91dk:

.. modem_lib_sending_traces_UART_start

Sending traces over UART on an nRF91 Series DK
==============================================

To send modem traces over UART on an nRF91 Series DK, configuration must be added for the UART device in the devicetree and Kconfig.
This is done by adding the :ref:`modem trace UART snippet <nrf91_modem_trace_uart_snippet>` when building and programming.

Use the `Cellular Monitor`_ app for capturing and analyzing modem traces.

TF-M logging must use the same UART as the application.
For more details, see :ref:`shared TF-M logging <tfm_enable_share_uart>`.

.. modem_lib_sending_traces_UART_end

.. _modem_trace_backend_uart_custom_board:

Sending traces over UART on a custom board
==========================================

To send modem traces over UART on a custom board, configuration must be added for the UART device in the devicetree.
This is done by adding the following code snippet to the board devicetree or overlay file, where the pin numbers (``0``, ``1``, ``14``, and ``15``) must be updated to match your board.
The snippet uses UART1. However, any free UART instance can be selected.

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
      status = "okay";
      current-speed = <1000000>;
      pinctrl-0 = <&uart1_default>;
      pinctrl-1 = <&uart1_sleep>;
      pinctrl-names = "default", "sleep";
   };

   / {
      chosen {
         nordic,modem-trace-uart = &uart1;
      };
   };

This is in addition to selecting the :kconfig:option:`CONFIG_NRF_MODEM_LIB_TRACE`, :kconfig:option:`CONFIG_NRF_MODEM_LIB_TRACE_BACKEND_UART`, :kconfig:option:`CONFIG_UART_ASYNC_API`, and :kconfig:option:`CONFIG_SERIAL` Kconfig options.

Modem tracing with RTT
**********************

.. note::

   Modem tracing with RTT is experimental.

Following are the requirements to perform tracing with RTT:

* An nRF91 Series DK with SEGGER J-Link on-Board or an external SEGGER J-Link
* J-Link RTT logger software application

To enable modem traces with RTT, enable the :kconfig:option:`CONFIG_NRF_MODEM_LIB_TRACE_BACKEND_RTT` and :kconfig:option:`CONFIG_USE_SEGGER_RTT` Kconfig options, with the :kconfig:option:`CONFIG_NRF_MODEM_LIB_TRACE` Kconfig option.

The traces can be captured using the J-Link RTT logger software.
This produces a RAW binary trace file with a ``.log`` extension.
The RAW binary trace file can be converted to PCAP with the :guilabel:`Open trace file in Wireshark` option in the `Cellular Monitor`_ app of `nRF Connect for Desktop`_.
By default, files with the ``.log`` extension are not shown.

.. _adding_custom_modem_trace_backends:

Adding custom trace backends
****************************

You can add custom trace backends if the existing trace backends are not sufficient.
At any time, only one trace backend can be compiled with the application.
The value of the :kconfig:option:`CONFIG_NRF_MODEM_LIB_TRACE_BACKEND` Kconfig option determines which trace backend is compiled.
The :ref:`modem_trace_backend_sample` sample demonstrates how you can add a custom trace backend to an application.

Complete the following steps to add a custom trace backend:

1. Place the files that have the custom trace backend implementation in a library or an application you create.
   For example, the implementation of the UART trace backend (default) can be found in the :file:`nrf/lib/nrf_modem_lib/trace_backends/uart/uart.c` file.

#. Add a C file implementing the interface in the :file:`nrf/include/modem/trace_backend.h` header file.

   .. code-block:: c

      /* my_trace_backend.c */

      #include <modem/trace_backend.h>

      int trace_backend_init(void)
      {
           /* initialize transport backend here */
           return 0;
      }

      int trace_backend_deinit(void)
      {
           /* optional deinitialization code here */
           return 0;
      }

      int trace_backend_write(const void *data, size_t len)
      {
           /* forward or store trace data here */
           /* return the number of bytes written or stored, or a negative error code on failure */
           return 0;
      }

      size_t trace_backend_data_size(void)
      {
         /* If trace data is stored when calling `trace_backend_write()`
          * this function returns the size of the stored trace data.
          *
          * If not applicable for the trace backend, set to NULL in the `trace_backend` struct.
          */
      }

      int trace_backend_read(uint8_t *buf, size_t len)
      {
         /* If trace data is stored when calling `trace_backend_write()`
          * this function allows the application to read back the trace data.
          *
          * If not applicable for the trace backend, set to NULL in the `trace_backend` struct.
          */
      }

      int trace_backend_clear(void)
      {
         /* This function allows the backend to clear all stored traces in the backend. For instance
          * this can be erasing a flash partition to prepare for writing new data.
          *
          * If not applicable for the trace backend, set to NULL in the `trace_backend` struct.
          */
      }

      int trace_backend_suspend(void)
      {
         /* This function allows the trace module to suspend the trace backend. When suspended,
          * the backend cannot be used by the trace module until it is resumed by calling
          * `trace_backend_resume()`.
          *
          * If not applicable for the trace backend, set to NULL in the `trace_backend` struct.
          */
      }

      int trace_backend_resume(void)
      {
         /* This function allows the trace module to resume the trace backend after it is suspended.
          *
          * If not applicable for the trace backend, set to NULL in the `trace_backend` struct.
          */
      }

      struct nrf_modem_lib_trace_backend trace_backend = {
         .init = trace_backend_init,
         .deinit = trace_backend_deinit,
         .write = trace_backend_write,
         .data_size = trace_backend_data_size, /* Set to NULL if not applicable. */
         .read = trace_backend_read, /* Set to NULL if not applicable. */
         .clear = trace_backend_clear, /* Set to NULL if not applicable. */
         .suspend = trace_backend_suspend, /* Set to NULL if not applicable. */
         .resume = trace_backend_resume, /* Set to NULL if not applicable. */
      };

#. Create or modify a :file:`Kconfig` file to extend the choice :kconfig:option:`CONFIG_NRF_MODEM_LIB_TRACE_BACKEND` with another option.

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
