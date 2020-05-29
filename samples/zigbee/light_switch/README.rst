.. _zigbee_light_switch_sample:

Zigbee: Light switch
####################

The Zigbee light switch sample can be used to change the state of light sources on other devices within the same Zigbee network.

You can use this sample together with the :ref:`Zigbee network coordinator <zigbee_network_coordinator_sample>` and the :ref:`Zigbee light bulb <zigbee_light_bulb_sample>` to set up a basic Zigbee network.

You can also enable the :ref:`nus_service_readme` for this sample to support dynamic concurrent switching between two protocols, Bluetooth LE and Zigbee.
See `Multiprotocol light switch with NUS`_ for details.

Overview
********

The light switch sample demonstrates the Zigbee End Device role and implements the Dimmer Light Switch profile.

Once the light switch is successfully commissioned, it sends a broadcast message to find devices with the implemented Level Control and On/Off clusters.
The light switch remembers the device network address from the first response, at which point it can be controlled with the development kit buttons.

Sleepy End Device behavior
===================================

The light switch supports the :ref:`zigbee_ug_sed` that enables the sleepy behavior for the end device, for a significant conservation of energy.

The sleepy behavior can be enabled by pressing **Button 3** while the light switch sample is booting.

Multiprotocol light switch with NUS
===================================

When the :ref:`nus_service_readme` is enabled, the light switch device operates through pressing buttons on the DK on the Zigbee network, and through the Nordic UART Service on the Bluetooth LE network.
Both networks are independent from each other.

To support both protocols at the same time, the Zigbee stack uses the :ref:`zephyr:ieee802154_interface` radio during the inactive time of the Bluetooth LE radio (using :ref:`nrfxlib:mpsl`'s Timeslot API).
Depending on the Bluetooth LE connection interval, the nRF52 development kits can spend up to 99% of the radio time on the Zigbee protocol.

Transmitting and receiving data when using this example does not break connection from any of the used radio protocols, either Bluetooth LE or Zigbee.

Source file setup
=================

This sample is split into two source files:

* The :file:`main` file to handle initialization and light switch basic behavior.
* An additional :file:`nus_cmd` file for handling NUS commands.

Requirements
************

* One or more of the following development kits:

  * |nRF52840DK|
  * |nRF52833DK|

* The :ref:`zigbee_network_coordinator_sample` sample programmed on one separate device.
* The :ref:`zigbee_light_bulb_sample` sample programmed on one or more separate devices.

You can mix different development kits.

Additional requirements for NUS
===============================

* A phone or tablet with the `nRF Toolbox`_ application installed.

  .. note::
    The `Testing`_ instructions refer to nRF Toolbox, but similar applications can be used as well, for example `nRF Connect for Mobile`_.

.. _zigbee_light_switch_user_interface:

User interface
**************

LED 1:
    When using :ref:`nus_service_readme` in the multiprotocol configuration, this LED indicates whether a Bluetooth LE Central is connected to the NUS service.
    When connected, the LED is on and solid.

LED 3:
    Indicates whether the device is connected to a Zigbee network.
    When connected, the LED is on and solid.

LED 4:
    Indicates that the light switch has found a light bulb to control.
    When the light bulb is found, the LED is on and solid.

Button 1:
    When the light bulb has been turned off, pressing this button once turns it back on.

    Pressing this button for a longer period of time increases the brightness of the light bulb.

Button 2:
    After the successful commissioning (light switch's **LED 3** turned on), pressing this button once turns off the light bulb connected to the network (light bulb's **LED 4**).

    Pressing this button for a longer period of time decreases the brightness of the **LED 4** of the connected light bulb.

.. note::
    If the brightness level is decreased to the minimum, the effect of turning on the light bulb might not be noticeable.

Button 3:
    When pressed while resetting the board, enables the :ref:`zigbee_ug_sed`.

UART command assignments:
    The following command assignments are configured and used in nRF Toolbox when using :ref:`nus_service_readme` in the multiprotocol configuration:

    +---------+----------------------------------------------------------+
    | Command | Effect                                                   |
    +=========+==========================================================+
    | n       | Turns on the Zigbee light bulb.                          |
    +---------+----------------------------------------------------------+
    | f       | Turns off the Zigbee light bulb.                         |
    +---------+----------------------------------------------------------+
    | t       | Toggles the Zigbee light bulb on or off.                 |
    +---------+----------------------------------------------------------+
    | i       | Increases the brightness level of the Zigbee light bulb. |
    +---------+----------------------------------------------------------+
    | d       | Decreases the brightness level of the Zigbee light bulb. |
    +---------+----------------------------------------------------------+

    If more than one light bulb is available in the network, these commands apply to all light bulbs in the network.
    See `Testing multiprotocol light switch`_ for details.

Building and running
********************

.. |sample path| replace:: :file:`samples/zigbee/light_switch`

|enable_zigbee_before_testing|

.. include:: /includes/build_and_run.txt

To enable multiprotocol light switch function, build with the :file:`overlay-mpsl-ble.conf` overlay file.

.. code-block:: console

   west build -p -b nrf52840dk_nrf52840 -- -DCONF_FILE="prj.conf overlay-mpsl-ble.conf"

See :ref:`gs_programming_board_names` for the board name to use instead of the ``nrf52840dk_nrf52840``.

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

Testing multiprotocol light switch
----------------------------------

To start interacting with the development kits using the nRF Toolbox application, complete the following steps:

1. On the phone or tablet, open the nRF Toolbox application.
#. Set up nRF Toolbox by completing the following steps:

   .. tabs::

      .. tab:: 1. Start UART

         Tap :guilabel:`UART` to open the UART application in nRF Toolbox.

         .. figure:: /images/nrftoolbox_dynamic_zigbee_uart_1.png
            :alt: UART application in nRF Toolbox

            UART application in nRF Toolbox

      .. tab:: 2. Configure commands

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

      .. tab:: 3. Connect to device

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

* Zigbee subsystem:

  * :file:`zb_nrf_platform.h`
  * :file:`zigbee_helpers.h`
  * :file:`zb_error_handler.h`

* :ref:`dk_buttons_and_leds_readme`

This sample uses the following `nrfxlib`_ libraries:

* ZBOSS Zigbee stack

In addition, it uses the following Zephyr libraries:

* :file:`include/zephyr.h`
* :file:`include/device.h`
* :ref:`zephyr:logging_api`
* :ref:`zephyr:pwm_api`
