.. _ug_zigbee_configuring_libraries:

Configuring Zigbee libraries in |NCS|
#####################################

The Zigbee protocol in |NCS| can be customized by enabling and configuring several libraries.

This page lists options and steps required for configuring the available libraries.

.. _ug_zigbee_configuring_components_handler:

Configuring default signal handler
**********************************

The default signal handler provides a default logic to handle ZBOSS stack signals.

The default signal handler can be used by calling the :cpp:func:`zigbee_default_signal_handler()` in the application's :cpp:func:`zboss_signal_handler()` implementation.
For more information, see :ref:`lib_zigbee_signal_handler`.

.. _ug_zigbee_configuring_components_ota:

Configuring DFU OTA
*******************

(short overview of what DFU OTA is)
For more information about DFU OTA, see :ref:`lib_zigbee_dfu_ota`.

The DFU OTA can be enabled and configured using the following Kconfig options:

* (mandatory option 1) - This option must be enabled to use the DFU OTA.
* (optional option 1) - This option allows to do XYZ.
* (optional option 2) - This option allows to do XYZ.

When configuring DFU OTA, take into account the following principles:

* Requirement 1
* Warning 1
