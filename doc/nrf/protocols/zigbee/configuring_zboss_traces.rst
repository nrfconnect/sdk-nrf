.. _ug_zigbee_configuring_zboss_traces:

Configuring ZBOSS traces in |NCS|
#################################

.. contents::
   :local:
   :depth: 2

The :ref:`nrfxlib:zboss` (ZBOSS stack) comes included in the |NCS| in a set of precompiled libraries, which can complicate the debugging process.
To help with that, the ZBOSS stack can be configured to print trace logs that allow you to trace the stack behavior.
This page describes how to enable and configure ZBOSS trace logs.

Trace logs are printed in binary form and require access to stack source files to be decoded into log messages.
Enabling trace logs can help with the debugging process even if you cannot decode them (due to lack of access to stack source files).
This is because of the information they can provide about the stack behavior whenever an issue is found or reproduced.

If any issues are found with the ZBOSS stack, trace logs can be enabled to collect this information while reproducing the issue.
A ticket can then be created in `DevZone`_ with the log file attached for assistance with the debugging process, and for decoding trace log files.

.. note::
     ZBOSS trace logs are meant to be used for debugging ZBOSS based applications.
     These libraries are not certified and should not be used for production firmware or end products.


Requirements
************

To collect and view the log files, you will need the following:

* Software that allows for reading serial ports.
  For example, if you are using Windows you can use :ref:`PuTTY <putty>`.

* Correct COM port.

   * For traces using the UART, the J-Link COM port is used. The development kit is assigned to a COM port (Windows) or a ttyACM device (Linux), which is visible in the system's Device Manager.

   * For traces using USB, a virtual COM port (a serial port emulated over USB) is used. You can set :kconfig:option:`CONFIG_USB_DEVICE_PRODUCT` to help identify the COM port in the system's Device Manager.

* An additional USB cable if trace logs through USB are enabled (see :ref:`ug_zigbee_configuring_zboss_traces_using_usb`).


.. rst-class:: numbered-step

Switch to ZBOSS libraries with compiled-in trace logs
*****************************************************

Set the Kconfig option :kconfig:option:`CONFIG_ZIGBEE_ENABLE_TRACES` to switch to ZBOSS libraries with compiled-in trace logs.

The ZBOSS stack comes in a precompiled form and trace logs are not compiled-in by default.
An additional set of ZBOSS libraries are available in nrfxlib, which does have trace logs compiled-in.


.. rst-class:: numbered-step

Select which ZBOSS trace logs to print
**************************************

Complete the following steps:

1. Select from which subsystems you would like to receive logs by configuring the ZBOSS trace mask with the Kconfig option :kconfig:option:`CONFIG_ZBOSS_TRACE_MASK`.
   Trace masks can be created by adding up masks of subsystems to receive the trace logs from.
   For available subsystems, see :file:`nrfxlib/zboss/production/include/zb_trace.h`.

#. Select the level of logs you want to receive.
   Configure ZBOSS trace level by selecting one of the following levels:

   * Error trace logs level - Set :kconfig:option:`CONFIG_ZBOSS_TRACE_LOG_LEVEL_ERR`
   * Warning trace logs level - Set :kconfig:option:`CONFIG_ZBOSS_TRACE_LOG_LEVEL_WRN`
   * Info trace logs level - Set :kconfig:option:`CONFIG_ZBOSS_TRACE_LOG_LEVEL_INF`
   * Debug trace logs level - Set :kconfig:option:`CONFIG_ZBOSS_TRACE_LOG_LEVEL_DBG`

If you do not want to receive trace logs, turn them off by setting the Kconfig option :kconfig:option:`CONFIG_ZBOSS_TRACE_LOG_LEVEL_OFF`.

Each of the following levels on the list also includes the previous one.
See :ref:`zigbee_ug_logging_stack_logs` to read more about trace logs.


.. rst-class:: numbered-step

Configure how to print ZBOSS trace logs
***************************************

The :ref:`zigbee_osif_zboss_osif_serial` offers a few backends to choose from for printing ZBOSS trace logs.
It is recommended to use the Zigbee serial logger, as it is the most efficient.
To enable it, set the Kconfig option :kconfig:option:`CONFIG_ZBOSS_TRACE_BINARY_LOGGING`.

Optional: Increasing the size of the ring buffer
   You can increase size of the ring buffer that temporarily stores the trace logs.
   To do this, use :kconfig:option:`CONFIG_ZBOSS_TRACE_LOGGER_BUFFER_SIZE` to assign a value for size of the buffer.
   This can prevent losing some of the logs in demanding scenarios such as high network traffic, multiple devices being configured or joined, and so on.
   See :ref:`Zigbee serial logger <zigbee_osif_zigbee_async_serial>` for more information.

Trace logs using UART (default)
===============================

When :kconfig:option:`CONFIG_ZBOSS_TRACE_BINARY_LOGGING` is selected, trace logs are printed using the UART by default.
To configure trace logs using the UART, complete the following steps:

1. Set the :kconfig:option:`CONFIG_ZBOSS_TRACE_UART_LOGGING` Kconfig option.

#. Provide the ZBOSS tracing serial device in Devicetree like this:

   .. code-block:: devicetree

      chosen {
          ncs,zboss-trace-uart = &uart1;
      };

#. Configure the UART device that you want to use to be connected to the onboard J-Link instead of ``UART_0``, by extending the DTS overlay file for the selected board with the following:

   .. code-block:: devicetree

      &pinctrl {
         uart0_default_alt: uart0_default_alt {
            group1 {
               psels = <NRF_PSEL(UART_TX, 1, 2)>,
                       <NRF_PSEL(UART_RX, 1, 1)>;
            };
         };

         uart0_sleep_alt: uart0_sleep_alt {
            group1 {
               psels = <NRF_PSEL(UART_TX, 1, 2)>,
                       <NRF_PSEL(UART_RX, 1, 1)>;
               low-power-enable;
            };
         };

         uart1_default_alt: uart1_default_alt {
            group1 {
               psels = <NRF_PSEL(UART_TX, 0, 6)>,
                       <NRF_PSEL(UART_RX, 0, 8)>,
                       <NRF_PSEL(UART_RTS, 0, 5)>,
                       <NRF_PSEL(UART_CTS, 0, 7)>;
            };
         };

         uart1_sleep_alt: uart1_sleep_alt {
            group1 {
               psels = <NRF_PSEL(UART_TX, 0, 6)>,
                       <NRF_PSEL(UART_RX, 0, 8)>,
                       <NRF_PSEL(UART_RTS, 0, 5)>,
                       <NRF_PSEL(UART_CTS, 0, 7)>;
               low-power-enable;
            };
         };
      };

      &uart1 {
         pinctrl-0 = <&uart1_default_alt>;
         pinctrl-1 = <&uart1_sleep_alt>;
         pinctrl-names = "default", "sleep";
      };

      &uart0 {
         pinctrl-0 = <&uart0_default_alt>;
         pinctrl-1 = <&uart0_sleep_alt>;
         pinctrl-names = "default", "sleep";
      };

   .. note::
      By connecting the UART device to the on-board J-Link, trace logs can be read directly from the J-Link COM port.
      As a consequence, the UART device used by the logger is disconnected and application logs cannot be accessed from the J-Link COM port.


Optional: Increasing the UART throughput
   You can also increase the UART throughput by changing the baudrate.
   Some of the trace logs will be dropped if the throughput is too low.
   By default, the UART baudrate is set to ``115200``.
   To increase the baudrate to ``1000000``, add the ``current-speed = <1000000>;`` property to the ``uart1`` node in the DTS overlay file.
   This can be done like the following:

   .. code-block:: devicetree

      &pinctrl {
         uart1_default_alt: uart1_default_alt {
            group1 {
               psels = <NRF_PSEL(UART_TX, 0, 6)>,
                       <NRF_PSEL(UART_RX, 0, 8)>,
                       <NRF_PSEL(UART_RTS, 0, 5)>,
                       <NRF_PSEL(UART_CTS, 0, 7)>;
            };
         };

         uart1_sleep_alt: uart1_sleep_alt {
            group1 {
               psels = <NRF_PSEL(UART_TX, 0, 6)>,
                       <NRF_PSEL(UART_RX, 0, 8)>,
                       <NRF_PSEL(UART_RTS, 0, 5)>,
                       <NRF_PSEL(UART_CTS, 0, 7)>;
               low-power-enable;
            };
         };
      };

      &uart1 {
         current-speed = <1000000>;
         pinctrl-0 = <&uart1_default_alt>;
         pinctrl-1 = <&uart1_sleep_alt>;
         pinctrl-names = "default", "sleep";
      };

.. _ug_zigbee_configuring_zboss_traces_using_usb:

Trace logs using native USB
===========================

Trace logs can also be configured to use a native USB.
This is useful because trace logs will be printed through a separate virtual COM port so that the console logs can still be read through the J-Link COM port.
For applications that relay on the UART connection through the J-Link COM port, for example the Network co-processor (NCP) sample, trace logs can only be configured through USB (COM port emulated over USB).
See the :ref:`Zigbee NCP <zigbee_ncp_sample>` sample page for how to configure trace logs for USB in this case.

.. note::
   Before proceeding with the following steps, first check if your Zigbee application already has USB enabled or is currently using a USB.
   If your application is already using a virtual COM port via native USB, use a device name that is different than the default ``CDC_ACM_0`` to create new virtual COM port for printing trace logs.
   For example, if ``CDC_ACM_0`` is already present, then create a virtual COM port named ``CDC_ACM_1``, and so on.
   Additionally, the Kconfig option :kconfig:option:`CONFIG_USB_COMPOSITE_DEVICE` must be set if there are multiple virtual COM ports configured.

   See the :ref:`Zigbee NCP <zigbee_ncp_sample>` sample page as an example where one virtual COM port instance is already configured, and another must be created.

To configure trace logs using native USB, complete the following steps:

1. Set the Kconfig option :kconfig:option:`CONFIG_ZBOSS_TRACE_USB_CDC_LOGGING`.
   This also enables the necessary USB Kconfig options.

#. Create a virtual COM port that will be used for printing ZBOSS trace logs by extending the DTS overlay file for the selected board with the following:

   .. code-block:: devicetree

      &zephyr_udc0 {
         cdc_acm_uart0: cdc_acm_uart0 {
            compatible = "zephyr,cdc-acm-uart";
            label = "CDC_ACM_0";
         };
      };

   .. note::
      For the ZBOSS trace logs to be printed correctly through the USB, it is recommended to avoid using the USB autosuspend.

#. Provide the ZBOSS tracing serial device in Devicetree like this:

   .. code-block:: devicetree

      chosen {
          ncs,zboss-trace-uart = &cdc_acm_uart0;
      };
