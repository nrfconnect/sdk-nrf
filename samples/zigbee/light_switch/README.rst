.. _zigbee_light_switch_sample:

Zigbee: Light switch
####################

.. contents::
   :local:
   :depth: 2

The :ref:`Zigbee <ug_zigbee>` light switch sample can be used to change the state of light sources on other devices within the same Zigbee network.

You can use this sample together with the :ref:`Zigbee network coordinator <zigbee_network_coordinator_sample>` and the :ref:`Zigbee light bulb <zigbee_light_bulb_sample>` to set up a basic Zigbee network.

This sample supports the optional `Sleepy End Device behavior`_ and :ref:`zigbee_light_switch_sample_nus`.
It also supports :ref:`lib_zigbee_fota`.
See :ref:`zigbee_light_switch_activating_variants` for details about how to enable these variants.

Requirements
************

The sample supports the following development kits:

.. table-from-rows:: /includes/sample_board_rows.txt
   :header: heading
   :rows: nrf52840dk_nrf52840, nrf52833dk_nrf52833, nrf5340dk_nrf5340_cpuapp, nrf21540dk_nrf52840

You can use one or more of the development kits listed above and mix different development kits.

For this sample to work, the following samples also need to be programmed:

* The :ref:`Zigbee network coordinator <zigbee_network_coordinator_sample>` sample on one separate device.
* The :ref:`zigbee_light_bulb_sample` sample on one or more separate devices.

Multiprotocol Bluetooth LE extension requirements
=================================================

If you enable the :ref:`zigbee_light_switch_sample_nus`, make sure you have a phone or a tablet with the `nRF Toolbox`_ application installed.

.. note::
    The `Testing`_ instructions refer to nRF Toolbox, but similar applications can be used as well, for example `nRF Connect for Mobile`_.

Overview
********

The light switch sample demonstrates the Zigbee End Device role and implements the Dimmer Light Switch profile.

Once the light switch is successfully commissioned, it sends a broadcast message to find devices with the implemented Level Control and On/Off clusters.
The light switch remembers the device network address from the first response, at which point it can be controlled with the development kit buttons.

Sleepy End Device behavior
==========================

The light switch supports the :ref:`zigbee_ug_sed` that enables the sleepy behavior for the end device, for a significant conservation of energy.

The sleepy behavior can be enabled by pressing **Button 3** while the light switch sample is booting.

.. _zigbee_light_switch_sample_nus:

Multiprotocol Bluetooth LE extension
====================================

This optional extension demonstrates dynamic concurrent switching between two protocols, Bluetooth LE and Zigbee.
It uses :ref:`nus_service_readme` library.

When this extension is enabled, you can use:

* Buttons on the light switch device to operate on the Zigbee network
* Nordic UART Service to operate on the Bluetooth LE network

Both networks are independent from each other.

To support both protocols at the same time, the Zigbee stack uses the :ref:`zephyr:ieee802154_interface` radio during the inactive time of the Bluetooth LE radio (using :ref:`nrfxlib:mpsl`'s Timeslot API).
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

To activate the optional extensions supported by this sample, modify :makevar:`OVERLAY_CONFIG` in the following manner:

* For the variant that supports :ref:`lib_zigbee_fota`, set :file:`overlay-fota.conf`.
  Alternatively, you can :ref:`configure Zigbee FOTA manually <ug_zigbee_configuring_components_ota>`.

  .. note::
     The :file:`overlay-fota.conf` file can be used only for nRF52840 DK.

* For the Multiprotocol Bluetooth LE extension, set :file:`overlay-multiprotocol_ble.conf`.
  Check :ref:`gs_programming_board_names` for the board name to use instead of the ``nrf52840dk_nrf52840``.

See :ref:`cmake_options` for instructions on how to add this option.
For more information about using configuration overlay files, see :ref:`zephyr:important-build-vars` in the Zephyr documentation.

FEM support
===========

.. |fem_file_path| replace:: :file:`samples/zigbee/common`

.. include:: /includes/sample_fem_support.txt

.. _zigbee_light_switch_user_interface:

User interface
**************

LED 3:
    On and solid when the device is connected to a Zigbee network.

LED 4:
    On and solid when the light switch has found a light bulb to control.

Button 1:
    When the light bulb is turned off, turn it back on.

    Pressing this button for a longer period of time increases the brightness of the light bulb.

Button 2:
    Turn off the light bulb connected to the network (light bulb's **LED 4**).
    This option is available after the successful commissioning (light switch's **LED 3** turned on).

    Pressing this button for a longer period of time decreases the brightness of the **LED 4** of the connected light bulb.

.. note::
    If the brightness level is at the minimum level, the effect of turning on the light bulb might not be noticeable.

Sleepy End Device behavior assignments
======================================

Button 3:
    When pressed while resetting the kit, enable the :ref:`zigbee_ug_sed`.

Multiprotocol Bluetooth LE extension assignments
================================================

LED 1:
    On and solid when a Bluetooth LE Central is connected to the NUS service.
    Available when using :ref:`nus_service_readme` in the multiprotocol configuration.

UART command assignments:
    The following command assignments are configured and used in nRF Toolbox when :ref:`zigbee_light_switch_testing_ble`:

    * ``n`` - Turn on the Zigbee light bulb.
    * ``f`` - Turn off the Zigbee light bulb.
    * ``t`` - Toggle the Zigbee light bulb on or off.
    * ``i`` - Increase the brightness level of the Zigbee light bulb.
    * ``d`` - Decrease the brightness level of the Zigbee light bulb.

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

After programming the sample to your development kits, test it by performing the following steps:

1. Turn on the development kit that runs the network coordinator sample.
   When **LED 3** turns on, this development kit has become the Coordinator of the Zigbee network.
#. Turn on the development kit that runs the light bulb sample.
   When **LED 3** turns on, the light bulb has become a Router inside the network.

   .. tip::
        If **LED 3** does not turn on, press **Button 1** on the Coordinator to reopen the network.

#. Turn on the development kit that runs the light switch sample.
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

         1. Tap the :guilabel:`EDIT` button in the top right corner of the application.
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

         #. Tap the :guilabel:`DONE` button in the top right corner of the application.

      .. tab:: c. Connect to device

         Tap :guilabel:`CONNECT` and select the ``Zigbee_Switch`` device from the list of devices.

         .. figure:: /images/nrftoolbox_dynamic_zigbee_uart_3.png
            :alt: nRF Toolbox - UART application view after establishing connection

            The UART application of nRF Toolbox after establishing the connection

         .. note::
            Observe that **LED 1** on the light switch node is solid, which indicates that the Bluetooth LE connection is established.
   ..

#. In nRF Toolbox, tap the buttons you assigned:

   a. Tap the ``n`` and ``f`` command buttons to turn the LED on the Zigbee light bulb node on and off, respectively.
   #. Tap the ``t`` command button two times to toggle the LED on the Zigbee light bulb node on and off.
   #. Tap the ``i`` and ``d`` command buttons to make adjustments to the brightness level.

You can now control the devices either with the buttons on the development kits or with the NUS UART command buttons in the nRF Toolbox application.

Sample output
-------------

The sample logging output can be observed through a serial port.
For more details, see :ref:`putty`.

Dependencies
************

This sample uses the following |NCS| libraries:

* :file:`include/zigbee/zigbee_error_handler.h`
* :ref:`lib_zigbee_application_utilities`
* Zigbee subsystem:

  * :file:`zb_nrf_platform.h`

* :ref:`dk_buttons_and_leds_readme`

This sample uses the following `sdk-nrfxlib`_ libraries:

* :ref:`nrfxlib:zboss`

In addition, it uses the following Zephyr libraries:

* :file:`include/zephyr.h`
* :file:`include/device.h`
* :ref:`zephyr:logging_api`
* :ref:`zephyr:pwm_api`

The following dependencies are added by the multiprotocol Bluetooth LE extension:

* :ref:`nrfxlib:softdevice_controller`
* :ref:`nus_service_readme`
* Zephyr's :ref:`zephyr:bluetooth_api`:

  * ``include/bluetooth/bluetooth.h``
  * ``include/bluetooth/gatt.h``
  * ``include/bluetooth/hci.h``
  * ``include/bluetooth/uuid.h``
  * ``include/bluetooth/services/nus.h``
