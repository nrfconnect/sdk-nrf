.. _ug_matter_gs_transmission_power:

Configuring transmission power
##############################

.. contents::
   :local:
   :depth: 2

You can configure the radio transmitter power (TX power) of your device to increase or decrease the wireless communication range of your device.
The radio transmitter power configuration must be modified separately for every wireless technology used in the Matter applications.

TX power settings can have a significant impact on the device's power consumption when :ref:`ug_matter_device_low_power_configuration`.

.. _ug_matter_gs_transmission_power_default:

Default TX power
****************

The following table lists the default TX power values.

+-------------------------------------------------------------------------+--------------------------+------------------------------------------------------+-----------------------------------------------------------------+
| Sample                                                                  | Board name               | Output power for Thread (dBm)                        | Output power for Bluetooth® LE (dBm)                            |
+=========================================================================+==========================+======================================================+=================================================================+
| :ref:`Light Bulb (FTD) <matter_light_bulb_sample>`                      | nrf52840dk               | 8                                                    | 0                                                               |
|                                                                         +--------------------------+------------------------------------------------------+-----------------------------------------------------------------+
|                                                                         | nrf5340dk                | 3                                                    | 0                                                               |
|                                                                         +--------------------------+------------------------------------------------------+-----------------------------------------------------------------+
|                                                                         | nrf7002dk                | 3                                                    | 0                                                               |
|                                                                         +--------------------------+------------------------------------------------------+-----------------------------------------------------------------+
|                                                                         | nrf21540dk               | 20                                                   | 0                                                               |
|                                                                         +--------------------------+------------------------------------------------------+-----------------------------------------------------------------+
|                                                                         | nrf54l15dk               | 8                                                    | 0                                                               |
|                                                                         +--------------------------+------------------------------------------------------+-----------------------------------------------------------------+
|                                                                         | nrf54lm20dk              | 8                                                    | 0                                                               |
+-------------------------------------------------------------------------+--------------------------+------------------------------------------------------+-----------------------------------------------------------------+
| :ref:`Light Switch (SED) <matter_light_switch_sample>`                  | nrf52840dk               | 0                                                    | 0                                                               |
|                                                                         +--------------------------+------------------------------------------------------+-----------------------------------------------------------------+
|                                                                         | nrf5340dk                | 0                                                    | 0                                                               |
|                                                                         +--------------------------+------------------------------------------------------+-----------------------------------------------------------------+
|                                                                         | nrf7002dk                | 0                                                    | 0                                                               |
|                                                                         +--------------------------+------------------------------------------------------+-----------------------------------------------------------------+
|                                                                         | nrf21540dk               | 0                                                    | 0                                                               |
|                                                                         +--------------------------+------------------------------------------------------+-----------------------------------------------------------------+
|                                                                         | nrf54l15dk               | 0                                                    | 0                                                               |
|                                                                         +--------------------------+------------------------------------------------------+-----------------------------------------------------------------+
|                                                                         | nrf54lm20dk              | 0                                                    | 0                                                               |
+-------------------------------------------------------------------------+--------------------------+------------------------------------------------------+-----------------------------------------------------------------+
| :ref:`Lock (SED) <matter_lock_sample>`                                  | nrf52840dk               | 0                                                    | 0                                                               |
|                                                                         +--------------------------+------------------------------------------------------+-----------------------------------------------------------------+
|                                                                         | nrf5340dk                | 0                                                    | 0                                                               |
|                                                                         +--------------------------+------------------------------------------------------+-----------------------------------------------------------------+
|                                                                         | nrf7002dk                | 0                                                    | 0                                                               |
|                                                                         +--------------------------+------------------------------------------------------+-----------------------------------------------------------------+
|                                                                         | nrf21540dk               | 0                                                    | 0                                                               |
|                                                                         +--------------------------+------------------------------------------------------+-----------------------------------------------------------------+
|                                                                         | nrf54l15dk               | 0                                                    | 0                                                               |
|                                                                         +--------------------------+------------------------------------------------------+-----------------------------------------------------------------+
|                                                                         | nrf54lm20dk              | 0                                                    | 0                                                               |
+-------------------------------------------------------------------------+--------------------------+------------------------------------------------------+-----------------------------------------------------------------+
| :ref:`Manufacturer Specific (MTD) <matter_manufacturer_specific_sample>`| nrf52840dk               | 8                                                    | 0                                                               |
|                                                                         +--------------------------+------------------------------------------------------+-----------------------------------------------------------------+
|                                                                         | nrf5340dk                | 8                                                    | 0                                                               |
|                                                                         +--------------------------+------------------------------------------------------+-----------------------------------------------------------------+
|                                                                         | nrf7002dk                | 3                                                    | 0                                                               |
|                                                                         +--------------------------+------------------------------------------------------+-----------------------------------------------------------------+
|                                                                         | nrf54l15dk               | 8                                                    | 0                                                               |
|                                                                         +--------------------------+------------------------------------------------------+-----------------------------------------------------------------+
|                                                                         | nrf54lm20dk              | 8                                                    | 0                                                               |
+-------------------------------------------------------------------------+--------------------------+------------------------------------------------------+-----------------------------------------------------------------+
| :ref:`Template (MTD) <matter_template_sample>`                          | nrf52840dk               | 8                                                    | 0                                                               |
|                                                                         +--------------------------+------------------------------------------------------+-----------------------------------------------------------------+
|                                                                         | nrf5340dk                | 3                                                    | 0                                                               |
|                                                                         +--------------------------+------------------------------------------------------+-----------------------------------------------------------------+
|                                                                         | nrf7002dk                | 3                                                    | 0                                                               |
|                                                                         +--------------------------+------------------------------------------------------+-----------------------------------------------------------------+
|                                                                         | nrf21540dk               | 20                                                   | 0                                                               |
|                                                                         +--------------------------+------------------------------------------------------+-----------------------------------------------------------------+
|                                                                         | nrf54l15dk               | 8                                                    | 0                                                               |
|                                                                         +--------------------------+------------------------------------------------------+-----------------------------------------------------------------+
|                                                                         | nrf54lm20dk              | 8                                                    | 0                                                               |
+-------------------------------------------------------------------------+--------------------------+------------------------------------------------------+-----------------------------------------------------------------+
| :ref:`Thermostat (MTD) <matter_thermostat_sample>`                      | nrf52840dk               | 8                                                    | 0                                                               |
|                                                                         +--------------------------+------------------------------------------------------+-----------------------------------------------------------------+
|                                                                         | nrf5340dk                | 3                                                    | 0                                                               |
|                                                                         +--------------------------+------------------------------------------------------+-----------------------------------------------------------------+
|                                                                         | nrf54l15dk               | 8                                                    | 0                                                               |
|                                                                         +--------------------------+------------------------------------------------------+-----------------------------------------------------------------+
|                                                                         | nrf54lm20dk              | 8                                                    | 0                                                               |
+-------------------------------------------------------------------------+--------------------------+------------------------------------------------------+-----------------------------------------------------------------+
| :ref:`Smoke CO Alarm (SED) <matter_smoke_co_alarm_sample>`              | nrf52840dk               | 0                                                    | 0                                                               |
|                                                                         +--------------------------+------------------------------------------------------+-----------------------------------------------------------------+
|                                                                         | nrf5340dk                | 0                                                    | 0                                                               |
|                                                                         +--------------------------+------------------------------------------------------+-----------------------------------------------------------------+
|                                                                         | nrf54l15dk               | 0                                                    | 0                                                               |
|                                                                         +--------------------------+------------------------------------------------------+-----------------------------------------------------------------+
|                                                                         | nrf54lm20dk              | 0                                                    | 0                                                               |
+-------------------------------------------------------------------------+--------------------------+------------------------------------------------------+-----------------------------------------------------------------+
| :ref:`Window Covering (SSED) <matter_window_covering_sample>`           | nrf52840dk               | 0                                                    | 0                                                               |
|                                                                         +--------------------------+------------------------------------------------------+-----------------------------------------------------------------+
|                                                                         | nrf5340dk                | 0                                                    | 0                                                               |
|                                                                         +--------------------------+------------------------------------------------------+-----------------------------------------------------------------+
|                                                                         | nrf21540dk               | 0                                                    | 0                                                               |
|                                                                         +--------------------------+------------------------------------------------------+-----------------------------------------------------------------+
|                                                                         | nrf54l15dk               | 0                                                    | 0                                                               |
|                                                                         +--------------------------+------------------------------------------------------+-----------------------------------------------------------------+
|                                                                         | nrf54lm20dk              | 0                                                    | 0                                                               |
+-------------------------------------------------------------------------+--------------------------+------------------------------------------------------+-----------------------------------------------------------------+
| :ref:`Temperature Sensor (SED) <matter_temperature_sensor_sample>`      | nrf52840dk               | 0                                                    | 0                                                               |
|                                                                         +--------------------------+------------------------------------------------------+-----------------------------------------------------------------+
|                                                                         | nrf5340dk                | 0                                                    | 0                                                               |
|                                                                         +--------------------------+------------------------------------------------------+-----------------------------------------------------------------+
|                                                                         | nrf54l15dk               | 0                                                    | 0                                                               |
|                                                                         +--------------------------+------------------------------------------------------+-----------------------------------------------------------------+
|                                                                         | nrf54lm20dk              | 0                                                    | 0                                                               |
+-------------------------------------------------------------------------+--------------------------+------------------------------------------------------+-----------------------------------------------------------------+
| :ref:`Contact Sensor (SED) <matter_contact_sensor_sample>`              | nrf52840dk               | 0                                                    | 0                                                               |
|                                                                         +--------------------------+------------------------------------------------------+-----------------------------------------------------------------+
|                                                                         | nrf5340dk                | 0                                                    | 0                                                               |
|                                                                         +--------------------------+------------------------------------------------------+-----------------------------------------------------------------+
|                                                                         | nrf54l15dk               | 0                                                    | 0                                                               |
|                                                                         +--------------------------+------------------------------------------------------+-----------------------------------------------------------------+
|                                                                         | nrf54lm20dk              | 0                                                    | 0                                                               |
+-------------------------------------------------------------------------+--------------------------+------------------------------------------------------+-----------------------------------------------------------------+
| :ref:`Closure (FTD) <matter_closure_sample>`                            | nrf52840dk               | 8                                                    | 0                                                               |
|                                                                         +--------------------------+------------------------------------------------------+-----------------------------------------------------------------+
|                                                                         | nrf5340dk                | 3                                                    | 0                                                               |
|                                                                         +--------------------------+------------------------------------------------------+-----------------------------------------------------------------+
|                                                                         | nrf54l15dk               | 8                                                    | 0                                                               |
|                                                                         +--------------------------+------------------------------------------------------+-----------------------------------------------------------------+
|                                                                         | nrf54lm20dk              | 8                                                    | 0                                                               |
+-------------------------------------------------------------------------+--------------------------+------------------------------------------------------+-----------------------------------------------------------------+

.. _ug_matter_gs_transmission_power_thread:

Changing TX power for Thread
****************************

To change the radio TX power used by the Thread protocol, set the :kconfig:option:`CONFIG_OPENTHREAD_DEFAULT_TX_POWER` Kconfig option to the desired dBm value in a range from ``-40`` to ``20`` dBm.

The following table lists the maximum output power values in dBm for each board.
The maximum value of 20 dBm is only recommended for devices that are using :ref:`radio Front-End Modules <ug_radio_fem>`.

+--------------------------+-----------------------------------------------------------------------------+
| Board name               | Min - max TX power (dBm)                                                    |
+==========================+=============================================================================+
| nrf52840dk               | -20 to +8                                                                   |
+--------------------------+-----------------------------------------------------------------------------+
| nrf5340dk                | -40 to +3                                                                   |
+--------------------------+-----------------------------------------------------------------------------+
| nrf7002dk                | -40 to +3                                                                   |
+--------------------------+-----------------------------------------------------------------------------+
| nrf21540dk               | -40 to +20 (:ref:`more information <ug_matter_gs_transmission_power_fem>`)  |
+--------------------------+-----------------------------------------------------------------------------+
| nrf54l15dk               | -8 to +8                                                                    |
+--------------------------+-----------------------------------------------------------------------------+
| nrf54lm20dk              | -8 to +8                                                                    |
+--------------------------+-----------------------------------------------------------------------------+

.. note::

   For nRF54L Series SoCs, the maximum TX power depends on the package variant.
   CSP package variants have a maximum TX power of 8 dBm, while for the QFN package variants it is 7 dBm.

You can provide the desired value also as a CMake argument when building the sample.

.. tabs::

   .. group-tab:: nRF Connect for VS Code

      To build a Matter sample with a custom Thread TX power in the nRF Connect for VS Code IDE, add the :kconfig:option:`CONFIG_OPENTHREAD_DEFAULT_TX_POWER` Kconfig option variable and the dBm value to the :term:`build configuration`'s :guilabel:`Extra CMake arguments` and rebuild the build configuration.
      For example, if you want to build for the ``nrf52840dk/nrf52840`` board target with the default Thread TX power equal to 2 dBm, add ``-DCONFIG_OPENTHREAD_DEFAULT_TX_POWER=2``.

      See `nRF Connect for VS Code extension pack <How to work with build configurations_>`_ documentation for more information.

   .. group-tab:: Command line

      To build a Matter sample with a custom Thread TX power from the command line, add the :kconfig:option:`CONFIG_OPENTHREAD_DEFAULT_TX_POWER` Kconfig option variable and the dBm value to the build command.
      For example, if you want to build for the ``nrf52840dk/nrf52840`` board target with the default Thread TX power equal to 2 dBm, run the following command:

      .. code-block:: console

         west build -b nrf52840dk/nrf52840 -- -DCONFIG_OPENTHREAD_DEFAULT_TX_POWER=2

..

.. _ug_matter_gs_transmission_power_bluetooth:

Changing TX power for Bluetooth LE
**********************************

To change the radio TX power used by Zephyr's Bluetooth LE controller, set the :kconfig:option:`CONFIG_BT_CTLR_TX_PWR` Kconfig option to the desired value.
However, you cannot set this config value directly, as it obtains the value from the selected ``CONFIG_BT_CTLR_TX_PWR_MINUS_<X>`` or ``CONFIG_BT_CTLR_TX_PWR_PLUS_<X>``, where *<X>* is replaced by the desired power value, in an irregular dBm range from ``-40`` to ``3`` or ``8`` dBm (depending on the SoC).
For example, to set Bluetooth LE TX power to +5 dBM, set the :kconfig:option:`CONFIG_BT_CTLR_TX_PWR_PLUS_5` Kconfig option to ``y``.

Check the :ref:`Kconfig Reference <kconfig-search>` for the full list of possible values for `CONFIG_BT_CTLR_TX_PWR_MINUS`_ and `CONFIG_BT_CTLR_TX_PWR_PLUS`_, as well as their dependencies.
The only exception is the value of 0 dBm, which is set with the :kconfig:option:`CONFIG_BT_CTLR_TX_PWR_0` Kconfig option.

The following table lists the minimum and maximum output power values in dBm for each board.

+--------------------------+-----------------------------------------------------------------------------------------------------------------+
| Board name               | Min - max TX power (dBm)                                                                                        |
+==========================+=================================================================================================================+
| nrf52840dk               | -20 to +8 (:kconfig:option:`CONFIG_BT_CTLR_TX_PWR_MINUS_20` to :kconfig:option:`CONFIG_BT_CTLR_TX_PWR_PLUS_8`)  |
+--------------------------+-----------------------------------------------------------------------------------------------------------------+
| nrf5340dk                | -40 to +3 (:kconfig:option:`CONFIG_BT_CTLR_TX_PWR_MINUS_40` to :kconfig:option:`CONFIG_BT_CTLR_TX_PWR_PLUS_3`)  |
+--------------------------+-----------------------------------------------------------------------------------------------------------------+
| nrf7002dk                | -40 to +3 (:kconfig:option:`CONFIG_BT_CTLR_TX_PWR_MINUS_40` to :kconfig:option:`CONFIG_BT_CTLR_TX_PWR_PLUS_3`)  |
+--------------------------+-----------------------------------------------------------------------------------------------------------------+
| nrf54l15dk               | -8 to +8 (:kconfig:option:`CONFIG_BT_CTLR_TX_PWR_MINUS_8` to :kconfig:option:`CONFIG_BT_CTLR_TX_PWR_PLUS_8`)    |
+--------------------------+-----------------------------------------------------------------------------------------------------------------+
| nrf54lm20dk              | -8 to +8 (:kconfig:option:`CONFIG_BT_CTLR_TX_PWR_MINUS_8` to :kconfig:option:`CONFIG_BT_CTLR_TX_PWR_PLUS_8`)    |
+--------------------------+-----------------------------------------------------------------------------------------------------------------+
| nrf21540dk               | :ref:`Handled automatically by the FEM driver <ug_matter_gs_transmission_power_fem>`                            |
+--------------------------+-----------------------------------------------------------------------------------------------------------------+

.. note::

   For nRF54L Series SoCs, the maximum TX power depends on the package variant.
   CSP package variants have a maximum TX power of 8 dBm, while for the QFN package variants it is 7 dBm.

For multicore boards, the configuration must be applied to the network core image.
You can do this by either editing the :file:`prj.conf` file or building the sample with an additional argument, as described in the following tabs.

.. tabs::

   .. group-tab:: nRF Connect for VS Code

      To build a Matter sample with a custom Bluetooth LE TX power in the nRF Connect for VS Code IDE, add the desired :kconfig:option:`CONFIG_BT_CTLR_TX_PWR` Kconfig option for the network core to the build configuration's :guilabel:`Extra CMake arguments` and rebuild the build configuration.
      To build for the network core, make sure to add the ``childImageName_`` parameter between ``-D`` and the name of the Kconfig option.
      The parameter name varies depending on the devices you are building for.
      For example:

      * If you want to build for Thread devices for the ``nrf5340dk/nrf5340/cpuapp`` board target with a Bluetooth LE TX power equal to 3 dBm, add ``-Dipc_radio_CONFIG_BT_CTLR_TX_PWR_PLUS_3=y`` as the CMake argument.
      * If you want to build for Wi-Fi® devices for the ``nrf7002dk/nrf5340/cpuapp`` board target with a Bluetooth LE TX power equal to 3 dBm, add ``-Dhci_ipc_CONFIG_BT_CTLR_TX_PWR_PLUS_3=y`` as the CMake argument.

      See `nRF Connect for VS Code extension pack <How to work with build configurations_>`_ documentation for more information.

   .. group-tab:: Command line

      To build a Matter sample with a custom Bluetooth LE TX power from the command line, add the desired :kconfig:option:`CONFIG_BT_CTLR_TX_PWR` Kconfig option for the network core to the build command.
      To build for the network core, make sure to add the ``childImageName_`` parameter between ``-D`` and the name of the Kconfig option.
      The parameter name varies depending on the devices you are building for.
      For example:

      * If you want to build for Thread devices for the ``nrf5340dk/nrf5340/cpuapp`` board target with a Bluetooth LE TX power equal to 3 dBm, run the following command:

        .. code-block:: console

           west build -b nrf5340dk/nrf5340/cpuapp -- -Dipc_radio_CONFIG_BT_CTLR_TX_PWR_PLUS_3=y

      * If you want to build for Wi-Fi® devices for the ``nrf7002dk/nrf5340/cpuapp`` board target with a Bluetooth LE TX power equal to 3 dBm, run the following command:

        .. code-block:: console

           west build -b nrf7002dk/nrf5340/cpuapp -- -Dhci_ipc_CONFIG_BT_CTLR_TX_PWR_PLUS_3=y

..

.. _ug_matter_gs_transmission_power_wifi:

Changing TX power for Wi-Fi
***************************

Changing TX power for the Wi-Fi protocol is currently not supported.

The maximum TX power for Wi-Fi depends on the frequency band and the modulation used.
See `Electrical specification for nRF7002`_ for reference values.

.. _ug_matter_gs_transmission_power_fem:

Changing TX power for FEM
*************************

The Matter application can support optional :ref:`radio Front-End Modules <ug_radio_fem>`.
When you work with Matter over Thread, you can control the TX power of the device by configuring the FEM's TX gain.

By default, the TX FEM gain is handled automatically by the FEM driver.
After setting the desired TX output power, for example using the :kconfig:option:`CONFIG_OPENTHREAD_DEFAULT_TX_POWER` Kconfig option, the radio driver configures the FEM gain to reach the desired value.
However, you can disable this feature and set the FEM gain TX power value manually.
For information about how to do this, read the :ref:`ug_radio_fem` page, in particular :ref:`ug_radio_fem_sw_support_mpsl_fem_output`.

The RX FEM gain is set to 13 dB by default, so the signal received at the antenna port will gain 13 dB and it will be provided to the SoC.
