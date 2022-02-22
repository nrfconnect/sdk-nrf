.. _ug_zigbee_other_ecosystems:

Configuring Zigbee samples for other ecosystems
###############################################

.. contents::
   :local:
   :depth: 2

You can configure :ref:`zigbee_samples` in |NCS| that you program onto Nordic Semiconductor's hardware platforms to make them work within Zigbee networks of other ecosystems, such as Amazon Alexa, IKEA TRÅDFRI, or Philips Hue.
This is only possible for Zigbee networks of other ecosystems that include the Trust Center.

Supported types of Zigbee networks
**********************************

Zigbee samples in |NCS| use the centralized type of Zigbee network.

This type of Zigbee network is created by a Zigbee Coordinator that usually acts as the Trust Center.
In this network, only the Trust Center can issue encryption keys and allow Routers and End Devices into the network.
The Coordinator is usually implemented on a hub or a gateway, such as Amazon Echo.

Configuring Zigbee samples for device type used
***********************************************

To make Zigbee samples in |NCS| work within Zigbee networks of other ecosystems, it might be required to change Zigbee sample configuration, based on the device types you are using.
The following configuration scenarios are possible:

Connecting Nordic device to a third-party Coordinator
  If you are connecting a Nordic Semiconductor's device programmed with a Zigbee sample to a third-party Zigbee Coordinator, set the :kconfig:option:`CONFIG_ZIGBEE_CHANNEL_SELECTION_MODE_MULTI` Kconfig option for the Zigbee sample in |NCS|.
  This will allow your device to scan all channels to find the Coordinator's Zigbee network.

Connecting a third-party Zigbee 3.0 device to a Nordic-based Coordinator
  If you are connecting a third-party Zigbee device that is compatible with Zigbee 3.0 to a Zigbee Coordinator that is programmed onto a Nordic Semiconductor's device, no additional configuration is needed.

Connecting a third-party Zigbee legacy device to a Nordic-based Coordinator
  If you are connecting a third-party Zigbee device that is not compatible with Zigbee 3.0 to a Zigbee Coordinator that is programmed onto a Nordic Semiconductor's device, you must enable the legacy mode on the Coordinator by calling :c:func:`zb_bdb_set_legacy_device_support`.

For the scenarios with the Nordic-based Coordinator, you can select the 802.15.4 channel on which the Coordinator will form the network by setting the :kconfig:option:`CONFIG_ZIGBEE_CHANNEL` Kconfig option.
Alternatively, you can enable the :kconfig:option:`CONFIG_ZIGBEE_CHANNEL_SELECTION_MODE_MULTI` Kconfig option to allow the Coordinator to use the best channel with least RF interference.

Connecting the device to other ecosystems
*****************************************

After configuring the sample for other ecosystems, programming it and turning on the device, check the sample log output to see if joining (or forming) the Zigbee network is successful.

Checking connection status
==========================

If the following message or similar is shown, the joining was not successful:

.. code-block::

   I: Network steering was not successful (status: -1)

The connection process can fail for different reasons.
The following instructions can help you troubleshoot the connection problem:

1. Make sure that the Coordinator node is up and running.
#. Make sure that the network is accepting new devices.
   The most immediate way to verify this is to check if the **LED3** on the Zigbee Coordinator node.
   If the LED is on, the network is configured correctly and it is open to new nodes.

   .. note::
      Each ecosystem can also offer its own method for checking the network configuration.
      For example, on the Amazon Alexa ecosystem, you can say "Alexa, find my devices".

#. Make sure that the device is scanning through all channels to find the Zigbee network.
   For example, verify that the :kconfig:option:`CONFIG_ZIGBEE_CHANNEL_SELECTION_MODE_MULTI` Kconfig option is correctly set.
#. Check if the device is compatible with Zigbee 3.0.
   If it is *not* compatible, enable the legacy mode by including a call to `zb_bdb_set_legacy_device_support`_.
