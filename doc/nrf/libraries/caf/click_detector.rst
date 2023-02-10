.. _caf_click_detector:

CAF: Click detector module
##########################

.. contents::
   :local:
   :depth: 2

The |click_detector| of the :ref:`lib_caf` (CAF) is responsible for generating a ``click_event`` when one of the known `Click types`_ is recorded for the button defined in the module configuration.

Configuration
*************

To use the module, you must enable the following Kconfig options:

* :kconfig:option:`CONFIG_CAF_CLICK_DETECTOR` - This option enables the |click_detector|.
* :kconfig:option:`CONFIG_CAF_BUTTON_EVENTS` - This option enables the ``button_event`` that is required for the |click_detector| to work.

In addition to :kconfig:option:`CONFIG_CAF_CLICK_DETECTOR`, the following Kconfig options are available for the module:

* :kconfig:option:`CONFIG_CAF_CLICK_DETECTOR_DEF_PATH`
* :kconfig:option:`CONFIG_CAF_CLICK_DETECTOR_PM_EVENTS`

Adding module configuration file
================================

In addition to setting the Kconfig options, you must also add a module configuration file that contains an array of :c:struct:`click_detector_config`.

To do so, complete the following steps:

1. Add a file that defines the following information for every ``key_id`` that should be handled by the |click_detector| in an array of :c:struct:`click_detector_config`:

   * :c:member:`click_detector_config.key_id` - ID of the selected key.
   * :c:member:`click_detector_config.consume_button_event` - Whether the ``button_event`` with the given :c:member:`click_detector_config.key_id` should be consumed by the module.

   For example, the file content could look like follows:

   .. code-block:: c

        #include <caf/key_id.h>
        #include <caf/click_detector.h>

        static const struct click_detector_config click_detector_config[] = {
                {
                        .key_id = KEY_ID(0x00, 0x01),
                        .consume_button_event = false,
                },
        };

   .. note::
        The :c:macro:`KEY_ID` macro is defined in :file:`include/caf/key_id.h`.

#. Specify the location of the file with the :kconfig:option:`CONFIG_CAF_CLICK_DETECTOR_DEF_PATH` Kconfig option.

.. note::
   The configuration file should be included only by the configured module.
   Do not include the configuration file in other source files.

Implementation details
**********************

Tracing of key states is implemented using a periodically submitted work (:c:struct:`k_work_delayable`).
The work updates the states of traced keys and sends ``click_event`` when one of the `Click types`_ is recorded.
The work is not submitted if there is no key for which the state should be updated.

Click types
===========

Click types refer to the way a button can be pressed.
The module records the following click types:

* :c:enumerator:`CLICK_SHORT` - Button pressed and released after a short time.
* :c:enumerator:`CLICK_NONE` - Button pressed and held for a period of time that is too long for :c:enumerator:`CLICK_SHORT`, but too short for :c:enumerator:`CLICK_LONG`.
* :c:enumerator:`CLICK_LONG` - Button pressed and held for a long period of time.
* :c:enumerator:`CLICK_DOUBLE` - Two sequences of the button press and release in a short time interval.

The exact values of time intervals for click types are defined in the :file:`subsys/caf/modules/click_detector.c` file.

Power management states
=======================

If the option :kconfig:option:`CONFIG_CAF_CLICK_DETECTOR_PM_EVENTS` is enabled, the module can react to power management events.
The module stops tracing of key states when ``power_down_event`` is received.
The module starts operating again when ``wake_up_event`` is received.
