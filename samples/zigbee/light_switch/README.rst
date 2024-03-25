.. _zigbee_light_switch_sample:

Zigbee: Light switch
####################

.. contents::
   :local:
   :depth: 2

You can use the :ref:`Zigbee <ug_zigbee>` Light switch sample to change the state of light sources on other devices within the same Zigbee network.

You can use it together with the :ref:`Zigbee Network coordinator <zigbee_network_coordinator_sample>` and the :ref:`Zigbee Light bulb <zigbee_light_bulb_sample>` samples to set up a basic Zigbee network.

This sample supports the optional `Sleepy End Device behavior`_ and :ref:`zigbee_light_switch_sample_nus`.
It also supports :ref:`lib_zigbee_fota`.
See :ref:`zigbee_light_switch_activating_variants` for details about how to enable these variants.

Requirements
************

The sample supports the following development kits:

.. table-from-sample-yaml::

You can use one or more of the development kits listed above and mix different development kits.

To test this sample, you also need to program the following samples:

* The :ref:`Zigbee Network coordinator <zigbee_network_coordinator_sample>` sample on one separate device.
* The :ref:`zigbee_light_bulb_sample` sample on one or more separate devices.

Multiprotocol Bluetooth LE extension requirements
=================================================

If you enable the :ref:`zigbee_light_switch_sample_nus`, make sure you have a phone or a tablet with the `nRF Toolbox`_ application installed.

.. note::
    The `Testing`_ instructions refer to nRF Toolbox, but you can also use similar applications, for example `nRF Connect for Mobile`_.

Overview
********

The Light switch sample demonstrates the Zigbee End Device role and implements the Dimmer Switch device specification, as defined in the Zigbee Home Automation public application profile.

Once the light switch is successfully commissioned, it sends a broadcast message to find devices with the implemented Level Control and On/Off clusters.
The light switch remembers the device network address from the first response.
At this point, you can start using the buttons on the development kit to control the clusters on the newly found devices.

Additionally, the light switch sample powers down unused RAM sections to lower power consumption in the sleep state.

Sleepy End Device behavior
==========================

The light switch supports the :ref:`zigbee_ug_sed` that enables the sleepy behavior for the end device, for a significant conservation of energy.

To enable the sleepy behavior, press **Button 3** while the light switch sample is booting.
This is required only when device is joining the network for the first time.
After restarting the device, it will boot with the sleepy behavior enabled.

.. _zigbee_light_switch_sample_nus:

Multiprotocol Bluetooth LE extension
====================================

This optional extension demonstrates dynamic concurrent switching between two protocols, Bluetooth® LE and Zigbee.
It uses the :ref:`nus_service_readme` library.

When this extension is enabled, you can use:

* Buttons on the light switch device to operate on the Zigbee network
* Nordic UART Service to operate on the Bluetooth LE network

Both networks are independent from each other.

To support both protocols at the same time, the Zigbee stack uses the :ref:`zephyr:ieee802154_interface` radio during the inactive time of the Bluetooth LE radio (using the Timeslot API of the :ref:`nrfxlib:mpsl`).
Depending on the Bluetooth LE connection interval, the nRF52 development kits can spend up to 99% of the radio time on the Zigbee protocol.

Transmitting and receiving data when using this example does not break connection from any of the used radio protocols, either Bluetooth LE or Zigbee.

For more information about the multiprotocol feature, see :ref:`ug_multiprotocol_support`.

.. _zigbee_light_switch_configuration:

Configuration
*************

|config|

Source file setup
=================

This sample is split into the following source files:

* The :file:`main` file to handle initialization and light switch basic behavior.
* An additional :file:`nus_cmd` file for handling NUS commands.

.. _zigbee_light_switch_activating_variants:

Configuration files for sample extensions
=========================================

The sample provides predefined configuration files for optional extensions.
You can find the configuration files in the :file:`samples/zigbee/light_switch` directory.

Activating optional extensions
------------------------------

To activate the :ref:`lib_zigbee_fota`, use the :file:`prj_fota.conf` configuration file.
For example, when building from the command line, use the following command:

.. code-block:: console

   west build samples/zigbee/light_switch -b nrf52840dk/nrf52840 -- -DFILE_SUFFIX=fota

Alternatively, you can :ref:`configure Zigbee FOTA manually <ug_zigbee_configuring_components_ota>`.

.. note::
   You can use the :file:`prj_fota.conf` file only with a development kit that contains the nRF52840 or nRF5340 SoC.

To activate the Multiprotocol Bluetooth LE extension, set :makevar:`EXTRA_CONF_FILE` to the :file:`overlay-multiprotocol_ble.conf`.
For example, when building from the command line, use the following command:

.. code-block:: console

   west build samples/zigbee/light_switch -b nrf52840dk/nrf52840 -- -DEXTRA_CONF_FILE='overlay-multiprotocol_ble.conf'


For the board name to use instead of the ``nrf52840dk/nrf52840``, see :ref:`programming_board_names`.

See :ref:`cmake_options` for instructions on how to add flags to your build.
For more information about using configuration overlay files, see :ref:`zephyr:important-build-vars` in the Zephyr documentation.

FEM support
===========

.. include:: /includes/sample_fem_support.txt

.. _zigbee_light_switch_user_interface:

User interface
**************

LED 3:
    Lit and solid when the device is connected to a Zigbee network.

LED 4:
    Lit and solid when the light switch has found a light bulb to control.

Button 1:
    When the light bulb is turned off, turn it back on.

    Pressing this button for a longer period of time increases the brightness of the light bulb.

Button 2:
    Turn off the light bulb connected to the network (light bulb's **LED 4**).
    This option is available after the successful commissioning (light switch's **LED 3** turned on).

    Pressing this button for a longer period of time decreases the brightness of the **LED 4** of the connected light bulb.

Button 4:
    When pressed for five seconds, it initiates the `factory reset of the device <Resetting to factory defaults_>`_.
    The length of the button press can be edited using the :kconfig:option:`CONFIG_FACTORY_RESET_PRESS_TIME_SECONDS` Kconfig option from :ref:`lib_zigbee_application_utilities`.
    Releasing the button within this time does not trigger the factory reset procedure.

.. note::
    If the brightness level is at the minimum level, you may not notice the effect of turning on the light bulb.

FOTA behavior assignments
=========================

LED 2:
    Indicates the OTA activity.
    Used only if the FOTA support is enabled.

Sleepy End Device behavior assignments
======================================

Button 3:
    When pressed while resetting the kit, enables the :ref:`zigbee_ug_sed`.

Multiprotocol Bluetooth LE extension assignments
================================================

LED 1:
    Lit and solid when a Bluetooth LE Central is connected to the NUS service.
    Available when using :ref:`nus_service_readme` in the multiprotocol configuration.

UART command assignments:
    The following command assignments are configured and used in nRF Toolbox when :ref:`zigbee_light_switch_testing_ble`:

    * ``n`` - Turn on the Zigbee Light bulb.
    * ``f`` - Turn off the Zigbee Light bulb.
    * ``t`` - Toggle the Zigbee Light bulb on or off.
    * ``i`` - Increase the brightness level of the Zigbee Light bulb.
    * ``d`` - Decrease the brightness level of the Zigbee Light bulb.

    If more than one light bulb is available in the network, these commands apply to all light bulbs in the network.
    See :ref:`zigbee_light_switch_testing_ble` for details.

Building and running
********************

.. |sample path| replace:: :file:`samples/zigbee/light_switch`

|enable_zigbee_before_testing|

.. include:: /includes/build_and_run.txt

.. _zigbee_light_switch_testing:

Testing
=======

After programming the sample to your development kits, complete the following steps to test it:

1. Turn on the development kit that runs the Network coordinator sample.

   When **LED 3** turns on, this development kit has become the Coordinator of the Zigbee network.

#. Turn on the development kit that runs the Light bulb sample.

   When **LED 3** turns on, the light bulb has become a Router inside the network.

   .. note::
        If **LED 3** does not turn on, press **Button 1** on the Coordinator to reopen the network.

#. Turn on the development kit that runs the Light switch sample.

   When **LED 3** turns on, the light switch has become an End Device, connected directly to the Coordinator.

#. Wait until **LED 4** on the light switch node turns on.

   This LED indicates that the light switch found a light bulb to control.

You can now use buttons on the development kit to control the light bulb, as described in :ref:`zigbee_light_switch_user_interface`.

.. _zigbee_light_switch_testing_ble:

Testing multiprotocol Bluetooth LE extension
--------------------------------------------

To test the multiprotocol Bluetooth LE extension, complete the following steps after the standard `Testing`_ procedure:

#. Set up nRF Toolbox by completing the following steps:

   .. tabs::

      .. tab:: a. Start UART

         Tap :guilabel:`UART` to open the UART application in nRF Toolbox.

         .. figure:: /images/nrftoolbox_dynamic_zigbee_uart_1.png
            :alt: UART application in nRF Toolbox

            UART application in nRF Toolbox

      .. tab:: b. Configure commands

         Configure the UART commands by completing the following steps:

         1. Tap :guilabel:`EDIT` in the top right corner of the application.
            The button configuration window appears.
         #. Create the active application buttons by completing the following steps:

            a. Bind the ``n`` command to one of the buttons, with EOL set to LF and an icon of your choice.
            #. Bind the ``f`` command to one of the buttons, with EOL set to LF and an icon of your choice.
            #. Bind the ``t`` command to one of the buttons, with EOL set to LF and an icon of your choice.
            #. Bind the ``d`` command to one of the buttons, with EOL set to LF and an icon of your choice.
            #. Bind the ``i`` command to one of the buttons, with EOL set to LF and an icon of your choice.

            .. figure:: /images/nrftoolbox_dynamic_zigbee_uart_2.png
               :alt: Configuring buttons in nRF Toolbox - UART application

               Configuring buttons in the UART application of nRF Toolbox

         #. Tap :guilabel:`DONE` in the top right corner of the application.

      .. tab:: c. Connect to device

         Tap :guilabel:`CONNECT` and select the **Zigbee_Switch** device from the list of devices.

         .. figure:: /images/nrftoolbox_dynamic_zigbee_uart_3.png
            :alt: nRF Toolbox - UART application view after establishing connection

            The UART application of nRF Toolbox after establishing the connection

         .. note::
            Observe that **LED 1** on the light switch node is solid, which indicates that the Bluetooth LE connection is established.
   ..

#. In nRF Toolbox, tap the buttons you assigned:

   a. Tap the :guilabel:`n` and :guilabel:`f` command buttons to turn the LED on the Zigbee Light bulb node on and off, respectively.
   #. Tap the :guilabel:`t` command button two times to toggle the LED on the Zigbee Light bulb node on and off.
   #. Tap the :guilabel:`i` and :guilabel:`d` command buttons to make adjustments to the brightness level.

You can now control the devices either with the buttons on the development kits or with the NUS UART command buttons in the nRF Toolbox application.

Sample output
-------------

You can observe the sample logging output through a serial port after connecting with a terminal emulator (for example, nRF Connect Serial Terminal).
See :ref:`test_and_optimize` for the required settings and steps.

Dependencies
************

This sample uses the following |NCS| libraries:

* :ref:`lib_ram_pwrdn`
* :ref:`lib_zigbee_error_handler`
* :ref:`lib_zigbee_application_utilities`
* Zigbee subsystem:

  * :file:`zb_nrf_platform.h`

* :ref:`dk_buttons_and_leds_readme`

It uses the following `sdk-nrfxlib`_ libraries:

* :ref:`nrfxlib:zboss` |zboss_version| (`API documentation`_)

In addition, it uses the following Zephyr libraries:

* :file:`include/zephyr.h`
* :file:`include/device.h`
* :ref:`zephyr:logging_api`

The following dependencies are added by the multiprotocol Bluetooth LE extension:

* :ref:`nrfxlib:softdevice_controller`
* :ref:`nus_service_readme`
* Zephyr's :ref:`zephyr:bluetooth_api`:

  * ``include/bluetooth/bluetooth.h``
  * ``include/bluetooth/gatt.h``
  * ``include/bluetooth/hci.h``
  * ``include/bluetooth/uuid.h``
  * ``include/bluetooth/services/nus.h``
