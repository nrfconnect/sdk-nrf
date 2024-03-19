.. _lib_zigbee_osif:

Zigbee ZBOSS OSIF
#################

.. contents::
   :local:
   :depth: 2

The Zigbee ZBOSS OSIF layer subsystem acts as the linking layer between the :ref:`nrfxlib:zboss` and the |NCS|.

.. _zigbee_osif_configuration:

Configuration
*************

The ZBOSS OSIF layer implements a series of functions used by ZBOSS and is included in the |NCS|'s Zigbee subsystem.

The ZBOSS OSIF layer is automatically enabled when you enable the ZBOSS library with the :kconfig:option:`CONFIG_ZIGBEE` Kconfig option.

You can also configure the following OSIF-related Kconfig options:

* :kconfig:option:`CONFIG_ZIGBEE_APP_CB_QUEUE_LENGTH` - Defines the length of the application callback and alarm queue.
  This queue is used to pass application callbacks and alarms from other threads or interrupts to the ZBOSS main loop context.
* :kconfig:option:`CONFIG_ZIGBEE_DEBUG_FUNCTIONS` - Includes functions to suspend and resume the ZBOSS thread.

  .. note::
      These functions are useful for debugging, but they can cause instability of the device.

* :kconfig:option:`CONFIG_ZBOSS_RESET_ON_ASSERT` - Configures the ZBOSS OSIF layer to reset the device when a ZBOSS assert occurs.
  This option is enabled by default.
  Use it for production-ready applications.
* :kconfig:option:`CONFIG_ZBOSS_HALT_ON_ASSERT` - Configures the ZBOSS OSIF layer to halt the device when a ZBOSS assert occurs.
  Use this option only for testing and debugging your application.
* :kconfig:option:`CONFIG_ZIGBEE_HAVE_SERIAL` - Enables the UART serial abstract for the ZBOSS OSIF layer and allows to configure the serial glue layer.
  For more information, see the :ref:`zigbee_osif_zboss_osif_serial` section.

  .. note::
      Serial abstract must be enabled when using the precompiled ZBOSS libraries since they have dependencies on it.

* :kconfig:option:`CONFIG_ZIGBEE_USE_BUTTONS` - Enables the buttons abstract for the ZBOSS OSIF layer.
  You can use this option if you want to test ZBOSS examples directly in the |NCS|.
* :kconfig:option:`CONFIG_ZIGBEE_USE_DIMMABLE_LED` - Dimmable LED (PWM) abstract for the ZBOSS OSIF layer.
  You can use this option if you want to test ZBOSS examples directly in the |NCS|.
* :kconfig:option:`CONFIG_ZIGBEE_USE_LEDS` - LEDs abstract for the ZBOSS OSIF layer.
  You can use this option if you want to test ZBOSS examples directly in the |NCS|.
* :kconfig:option:`CONFIG_ZIGBEE_USE_SOFTWARE_AES` - Configures the ZBOSS OSIF layer to use the software encryption.
* :kconfig:option:`CONFIG_ZIGBEE_NVRAM_PAGE_COUNT` - Configures the number of ZBOSS NVRAM logical pages.
* :kconfig:option:`CONFIG_ZIGBEE_NVRAM_PAGE_SIZE` - Configures the size of the RAM-based ZBOSS NVRAM.
  This option is used only if the device does not have NVRAM storage.
* :kconfig:option:`CONFIG_ZIGBEE_TIME_COUNTER` - Configures the ZBOSS OSIF layer to use a dedicated timer-based counter as the Zigbee time source.
* :kconfig:option:`CONFIG_ZIGBEE_TIME_KTIMER` - Configures the ZBOSS OSIF layer to use Zephyr's system time as the Zigbee time source.

Additionally, the following Kconfig option is available when setting :ref:`zigbee_ug_logging_logger_options`:

* :kconfig:option:`CONFIG_ZBOSS_OSIF_LOG_LEVEL` - Configures the custom logger options for the ZBOSS OSIF layer.

.. _zigbee_osif_zboss_osif_serial:

ZBOSS OSIF serial abstract
**************************

Setting the :kconfig:option:`CONFIG_ZIGBEE_HAVE_SERIAL` option enables the serial abstract for the ZBOSS OSIF layer.

The ZBOSS OSIF serial implements sets of backend functions that are used by the ZBOSS stack for serial communication:

* Zigbee async serial
* Zigbee serial logger
* Zigbee logger

These backend functions serve one or both of the following purposes:

* Logging ZBOSS traces - Used for handling stack logs that are useful for debugging and are provided in binary format.
* Handling NCP communication with the host device - Used only for the :ref:`NCP architecture <ug_zigbee_platform_design_ncp>`.

The following table shows which sets of functions serve which purpose.

.. _osif_table:

+----------------------------+---------------+---------------+----------+
|                            | Async serial  | Serial logger | Logger   |
+============================+===============+===============+==========+
| Logging ZBOSS traces       | -             | -             | -        |
+----------------------------+---------------+---------------+----------+
| Handling NCP communication | -             |               |          |
+----------------------------+---------------+---------------+----------+

For more information about configuring ZBOSS stack logs, see :ref:`zigbee_ug_logging_stack_logs`.

.. _zigbee_osif_zigbee_async_serial:

Zigbee async serial
===================

The Zigbee async serial is the only backend that the ZBOSS OSIF serial supports for handling the NCP communication.
This set of functions uses :ref:`Zephyr UART API <zephyr:uart_api>` and can be configured to use UART peripheral or USB CDC ACM device.
The data received is internally buffered.

You can also use the Zigbee async serial for logging ZBOSS traces.
When enabled, it logs ZBOSS traces in the binary format.
In such case, the transmission data is also buffered.

Zigbee async serial configuration options
-----------------------------------------

To configure this set of functions, use the following options:

* :kconfig:option:`CONFIG_ZIGBEE_HAVE_ASYNC_SERIAL` - This option enables Zigbee async serial.
* :kconfig:option:`CONFIG_ZIGBEE_UART_SUPPORTS_FLOW_CONTROL` - This option should be set if serial device supports flow control.
* :kconfig:option:`CONFIG_ZIGBEE_UART_RX_BUF_LEN` - This option enables and configures the size of internal RX and TX buffer.
* :kconfig:option:`CONFIG_ZBOSS_TRACE_BINARY_NCP_TRANSPORT_LOGGING` - This option enables logging ZBOSS traces in binary format with Zigbee async serial.

The Zigbee ZBOSS OSIF layer serial device needs to be provided in devicetree as follows:

.. code-block:: devicetree

   chosen {
       ncs,zigbee-uart = &uart0;
   };

Zigbee serial logger
====================

This set of functions uses Zephyr's :ref:`UART API <zephyr:uart_api>` and can be configured to use either the UART peripheral or the USB CDC ACM device.
Data is buffered internally in ring buffer and printed in the binary format.
This ring buffer has a size of 4096 bytes by default.

Zigbee serial logger configuration options
------------------------------------------

Use the following options to configure the Zigbee serial logger:

* :kconfig:option:`CONFIG_ZBOSS_TRACE_BINARY_LOGGING` - This option enables logging ZBOSS traces with Zigbee serial logger.
* :kconfig:option:`CONFIG_ZBOSS_TRACE_UART_LOGGING` - This option selects the UART serial backend.
* :kconfig:option:`CONFIG_ZBOSS_TRACE_USB_CDC_LOGGING` - This option selects the USB CDC ACM serial backend.

   .. note::
      See :ref:`zephyr:usb_device_cdc_acm` for more information about how to configure USB CDC ACM instance for logging ZBOSS trace messages.

* :kconfig:option:`CONFIG_ZBOSS_TRACE_LOGGER_BUFFER_SIZE` - This option specifies the size of the internal ring buffer.

The ZBOSS tracing serial device needs to be provided in Devicetree like this:

.. code-block:: devicetree

   chosen {
       ncs,zboss-trace-uart = &uart1;
   };

Zigbee logger
=============

This set of functions uses Zephyr's :ref:`zephyr:logging_api` API for logging hexdumps of received binary data.
Data is buffered internally in ring buffer.

Zigbee logger configuration options
-----------------------------------

Use the following options to configure the Zigbee logger:

* :kconfig:option:`CONFIG_ZBOSS_TRACE_HEXDUMP_LOGGING` - This option enables Logging ZBOSS Traces with Zigbee logger.
* :kconfig:option:`CONFIG_ZBOSS_TRACE_LOGGER_BUFFER_SIZE` - This option specifies size of internal ring buffer.

API documentation
*****************

| Header files: :file:`subsys/zigbee/osif/zb_nrf_platform.h`
| Source files: :file:`subsys/zigbee/osif/`

.. doxygengroup:: zigbee_zboss_osif
   :project: nrf
   :members:
