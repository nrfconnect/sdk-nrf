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

The ZBOSS OSIF layer is automatically enabled when you enable the ZBOSS library with the :kconfig:`CONFIG_ZIGBEE` Kconfig option.

You can also configure the following OSIF-related Kconfig options:

* :kconfig:`CONFIG_ZIGBEE_APP_CB_QUEUE_LENGTH` - Defines the length of the application callback and alarm queue.
  This queue is used to pass application callbacks and alarms from other threads or interrupts to the ZBOSS main loop context.
* :kconfig:`CONFIG_ZIGBEE_DEBUG_FUNCTIONS` - Includes functions to suspend and resume the ZBOSS thread.

  .. note::
      These functions are useful for debugging, but they can cause instability of the device.

* :kconfig:`CONFIG_ZIGBEE_HAVE_SERIAL` - Enables the UART serial abstract for the ZBOSS OSIF layer and allows to configure the serial glue layer.
* :kconfig:`CONFIG_ZIGBEE_USE_BUTTONS` - Enables the buttons abstract for the ZBOSS OSIF layer.
  You can use this option if you want to test ZBOSS examples directly in the |NCS|.
* :kconfig:`CONFIG_ZIGBEE_USE_DIMMABLE_LED` - Dimmable LED (PWM) abstract for the ZBOSS OSIF layer.
  You can use this option if you want to test ZBOSS examples directly in the |NCS|.
* :kconfig:`CONFIG_ZIGBEE_USE_LEDS` - LEDs abstract for the ZBOSS OSIF layer.
  You can use this option if you want to test ZBOSS examples directly in the |NCS|.
* :kconfig:`CONFIG_ZIGBEE_USE_SOFTWARE_AES` - Configures the ZBOSS OSIF layer to use the software encryption.

Additionally, the following Kconfig option is available when setting :ref:`zigbee_ug_logging_logger_options`:

* :kconfig:`CONFIG_ZBOSS_OSIF_LOG_LEVEL` - Configures the custom logger options for the ZBOSS OSIF layer.

API documentation
*****************

| Header files: :file:`subsys/zigbee/osif/zb_nrf_platform.h`
| Source files: :file:`subsys/zigbee/osif/`

.. doxygengroup:: zigbee_zboss_osif
   :project: nrf
   :members:
