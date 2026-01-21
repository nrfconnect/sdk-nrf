.. |matter_name| replace:: Light Bulb
.. |matter_type| replace:: sample
.. |matter_dks_thread| replace:: ``nrf52840dk/nrf52840``, ``nrf5340dk/nrf5340/cpuapp``, ``nrf54l15dk/nrf54l15/cpuapp``, and ``nrf54lm20dk/nrf54lm20a/cpuapp``
.. |matter_dks_wifi| replace:: ``nrf54lm20dk/nrf54lm20a/cpuapp`` board target with the ``nrf7002eb2`` shield attached
.. |matter_dks_internal| replace:: nRF54LM20 DK
.. |matter_dks_tfm| replace:: ``nrf54l15dk/nrf54l15/cpuapp`` board target
.. |sample path| replace:: :file:`samples/matter/light_bulb`
.. |matter_qr_code_payload| replace:: MT:6FCJ142C00KA0648G00
.. |matter_pairing_code| replace:: 34970112332
.. |matter_qr_code_image| image:: /images/matter_qr_code_light_bulb.png
                          :width: 200px
                          :alt: QR code for commissioning the light bulb device

.. include:: /includes/matter/shortcuts.txt

.. _matter_light_bulb_sample:
.. _chip_light_bulb_sample:

Matter: Light bulb
##################

.. contents::
   :local:
   :depth: 2

This light bulb sample demonstrates the usage of the :ref:`Matter <ug_matter>` application layer to build a white dimmable light bulb device.

.. include:: /includes/matter/introduction/no_sleep_thread_ftd_wifi.txt

.. note::
    This sample is self-contained and can be tested on its own.
    However, it is required when testing the :ref:`Matter light switch <matter_light_switch_sample>` sample.

The sample can also communicate with `AWS IoT Core`_ over a Wi-Fi network using the nRF54LM20 DK with the nRF7002-EB II shield attached.
For more details, see the :ref:`matter_light_bulb_aws_iot` section.

Requirements
************

The sample supports the following development kits:

.. table-from-sample-yaml::

.. include:: /includes/matter/requirements/thread_wifi.txt

Overview
********

The sample uses buttons to test changing the light bulb and device states, and LEDs to show the state of these changes.
You can test it in the following ways:

* Standalone, by using a single DK that runs the light bulb application.
* Remotely over Thread or Wi-Fi, which requires more devices.

The remote control testing requires a Matter controller that you can configure either on a PC or a mobile device (for remote testing in a network).

.. include:: /includes/matter/overview/matter_quick_start.txt

Light bulb features
===================

The light bulb sample implements the following features:

* OnOff - The light bulb can be turned on and off.
* LevelControl - The light bulb can be dimmed to a specific level.
* `AWS IoT integration`_ - The light bulb can be configured to communicate with `AWS IoT Core`_ to control attributes in supported clusters on the device.

Use the ``click to show`` toggle to expand the content.

.. _matter_light_bulb_aws_iot_integration:

AWS IoT integration
-------------------

.. toggle::

   The sample can be configured to communicate with `AWS IoT Core`_ to control attributes in supported clusters on the device.
   After a connection has been established, the sample will mirror these attributes in the AWS IoT shadow document.
   This makes it possible to remotely control the device using the `AWS IoT Device Shadow Service`_.
   The supported attributes are ``OnOff`` from the ``OnOff`` cluster and ``CurrentLevel`` from the ``LevelControl`` cluster.

   The following figure illustrates the relationship between the AWS IoT integration layer and the light bulb sample:

   .. figure:: /images/aws_matter_integration.svg
      :alt: Sample implementation of the AWS IoT integration layer

      AWS IoT integration layer implementation diagram

   The following figure illustrates the interaction with the AWS IoT shadow Service:

   .. figure:: /images/aws_matter_interaction.svg
      :alt: Interaction with the AWS IoT shadow service

      AWS IoT Shadow and Matter interaction diagram

   .. note::

      The AWS IoT integration is available only for the nRF54LM20 DK with the nRF7002-EB II shield attached.

.. _matter_light_bulb_aws_iot:

Configuration
*************

.. include:: /includes/matter/configuration/intro.txt

The |matter_type| supports the following build configurations:

.. include:: /includes/matter/configuration/basic_internal.txt

Advanced configuration options
==============================

.. include:: /includes/matter/configuration/advanced/intro.txt
.. include:: /includes/matter/configuration/advanced/dfu.txt
.. include:: /includes/matter/configuration/advanced/tfm.txt
.. include:: /includes/matter/configuration/advanced/fem.txt
.. include:: /includes/matter/configuration/advanced/factory_data.txt
.. include:: /includes/matter/configuration/advanced/custom_board.txt
.. include:: /includes/matter/configuration/advanced/internal_memory.txt

AWS IoT setup and configuration
-------------------------------

.. toggle::

   To set up an AWS IoT instance and configure the sample, complete the following steps:

   1. Complete the setup and configuration described in the :ref:`lib_aws_iot` documentation to get the host name, device ID, and certificates used in the connection.
   #. Set the :kconfig:option:`CONFIG_AWS_IOT_BROKER_HOST_NAME` and :kconfig:option:`CONFIG_AWS_IOT_CLIENT_ID_STATIC` Kconfig options in the :file:`overlay-aws-iot-integration.conf` file.
   #. Import the certificates to the :file:`light_bulb/src/aws_iot_integration/certs` folder.

      The certificates will vary in size depending on the method you chose when generating the certificates.
      Due to this, you might need to increase the value of the :kconfig:option:`CONFIG_MBEDTLS_SSL_OUT_CONTENT_LEN` option to be able to establish a connection.
   #. |open_terminal_window_with_environment|
   #. Build the sample using the following command:

      .. code-block:: console

         west build -p -b nrf54lm20dk/nrf54lm20a/cpuapp -- -DSB_CONFIG_WIFI_NRF70=y -Dlight_bulb_SHIELD=nrf7002eb2 -DEXTRA_CONF_FILE="overlay-aws-iot-integration.conf"

   #. Flash the firmware and boot the sample.
   #. |connect_kit|
   #. |connect_terminal_ANSI|
   #. Commission the device to the Matter network.
   #. Observe that the device automatically connects to AWS IoT when an IP is obtained and the device is able to maintain the connection.
   #. Use the following bash function to populate the desired section of the shadow:

      .. code-block:: console

         function aws-update-desired() {
               aws iot-data update-thing-shadow --cli-binary-format raw-in-base64-out --thing-name my-thing --payload "{\"state\":{\"desired\":{\"onoff\":$1,\"level_control\":$2}}}" "output.txt"
         }

      You can also use ``aws-update-desired 0 0``, or ``aws-update-desired 1 128 (onoff, levelcontrol)``.
      Alternatively, you can alter the device shadow directly through the `AWS IoT console`_.

   #. Observe that the light bulb changes state.
      The local changes to the attributes always take precedence over what is set in the shadow's desired state.

User interface
**************

.. include:: /includes/matter/interface/intro.txt

First LED:
   .. include:: /includes/matter/interface/state_led.txt

Second LED:
   Shows the state of the light bulb.
   The following states are possible:

   * Solid On - The light bulb is on.
   * Off - The light bulb is off.

   Additionally, the LED starts blinking evenly (500 ms on/500 ms off) when the Identify command of the Identify cluster is received on the endpoint ``1``.
   The command's argument can be used to specify the duration of the effect.

First Button:
   .. include:: /includes/matter/interface/main_button.txt

Second Button:
   Changes the light bulb state to the opposite one.

.. include:: /includes/matter/interface/segger_usb.txt
.. include:: /includes/matter/interface/nfc.txt

Building and running
********************

.. include:: /includes/matter/building_and_running/intro.txt

|matter_ble_advertising_auto|

Advanced building options
=========================

.. include:: /includes/matter/building_and_running/advanced/intro.txt
.. include:: /includes/matter/building_and_running/advanced/building_nrf54lm20dk_7002eb2.txt
.. include:: /includes/matter/building_and_running/advanced/wifi_flash.txt

Testing
*******

.. include:: /includes/matter/testing/intro.txt

.. _prepare_light_bulb_for_testing:

Testing with CHIP Tool
======================

Complete the following steps to test the |matter_name| device using CHIP Tool:

.. |node_id| replace:: 1

.. include:: /includes/matter/testing/1_prepare_matter_network_thread_wifi.txt
.. include:: /includes/matter/testing/2_prepare_dk.txt
.. include:: /includes/matter/testing/3_commission_thread_wifi.txt

.. rst-class:: numbered-step

Turn on the light bulb
----------------------

Turn on the light bulb by running the following command:

.. parsed-literal::
   :class: highlight

   chip-tool onoff on |node_id| 1

Observe that the |Second LED| is on.

.. rst-class:: numbered-step

Turn off the light bulb
-----------------------

Turn off the light bulb by running the following command:

.. parsed-literal::
   :class: highlight

   chip-tool onoff off |node_id| 1

Observe that the |Second LED| is off.

Testing communication with another device
=========================================

After programming this sample and the :ref:`Matter light switch <matter_light_switch_sample>` sample to the development kits, complete the steps in the following steps to test communication between both devices.

.. rst-class:: numbered-step

Prepare the light switch device
-------------------------------

To prepare the light switch device, follow the first three steps in the :ref:`Matter light switch <matter_light_switch_sample>` sample :ref:`prepare_light_switch_for_testing` section.

.. note::

   In this guide, the light bulb device's node ID is ``1`` and the light switch device's node ID is ``2``.

.. include:: ../light_switch/README.rst
   :start-after: matter_light_switch_sample_testing_start
   :end-before: matter_light_switch_sample_testing_end

Testing with commercial ecosystem
=================================

.. include:: /includes/matter/testing/ecosystem.txt

Dependencies
************

This sample uses the Matter library that includes the |NCS| platform integration layer:

* `Matter`_

In addition, it uses the following |NCS| components:

* :ref:`dk_buttons_and_leds_readme`
* :ref:`nfc_uri`
* :ref:`lib_nfc_t2t`

The sample depends on the following Zephyr libraries:

* :ref:`zephyr:logging_api`
* :ref:`zephyr:kernel_api`
