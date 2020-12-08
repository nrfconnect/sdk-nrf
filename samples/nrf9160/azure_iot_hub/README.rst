.. _azure_iot_hub:

nRF9160: Azure IoT Hub
######################

.. contents::
   :local:
   :depth: 2

The Azure IoT Hub sample shows the communication of an nRF9160-based device with an `Azure IoT Hub`_ instance.
This sample uses the :ref:`lib_azure_iot_hub` library to communicate with the IoT hub.


Requirements
************

The sample supports the following development kits:

.. table-from-rows:: /includes/sample_board_rows.txt
   :header: heading
   :rows: thingy91_nrf9160ns, nrf9160dk_nrf9160ns

.. include:: /includes/spm.txt

Overview
********

The sample supports the direct connection of an IoT device that is already registered to an Azure IoT Hub instance.
Alternatively, it supports the provisioning of the device using `Azure IoT Hub Device Provisioning Service (DPS)`_ to an IoT hub.
See the documentation on :ref:`lib_azure_iot_hub` library for more information.

The sample periodically publishes telemetry messages (events) to the connected Azure IoT Hub instance.
By default, telemetry messages are sent every 20 seconds.
The default interval can be configured by setting the device twin property ``desired.telemetryInterval``, which will be interpreted by the sample in units of seconds.
The format of a telemetry message is shown below:

.. parsed-literal::
   :class: highlight

   {
     "temperature": 25.2,
     "timestamp": 151325
   }

where ``temperature`` is a value between ``25.0`` and ``25.9``, and ``timestamp`` is the uptime of the device in milliseconds.

The sample has implemented the handling of `Azure IoT Hub direct method`_ with the name ``led``.
If the device receives a direct method invocation with the name ``led`` and payload ``1`` or ``0``, LED 1 on the device is turned on or off, depending on the payload.
On Thingy:91, the LED turns red if the payload is ``1``.


Setup
=====

For the sample to work as intended, you must setup and configure an Azure IoT Hub instance.
See :ref:`configure_options_azure_iot` for information on the configuration options that can be used to create an Azure IoT Hub instance.
Also, for a successful TLS connection to the Azure IoT Hub instance, the device needs to have certificates provisioned.
See :ref:`prereq_connect_to_azure_iot_hub` for information on provisioning the certificates.

.. _configure_options_azure_iot:

Additional Configuration
========================

Check and configure the following library options that are used by the sample:

* :option:`CONFIG_AZURE_IOT_HUB_DEVICE_ID` - Sets the Azure IoT Hub device ID. Alternatively, enable :option:`CONFIG_AZURE_IOT_HUB_DEVICE_ID_APP` option and set the device ID at run time in :c:struct:`azure_iot_hub_config` passed to the :c:func:`azure_iot_hub_init` function.
* :option:`CONFIG_AZURE_IOT_HUB_HOSTNAME` - Sets the Azure IoT Hub host name. If DPS is used, the sample assumes that the IoT hub host name is unknown, and the configuration is ignored.
* :option:`CONFIG_AZURE_IOT_HUB_DPS` - Enables Azure IoT Hub DPS.
* :option:`CONFIG_AZURE_IOT_HUB_DPS_ID_SCOPE` - Sets the Azure IoT Hub DPS ID scope.

.. note::

   The sample sets the option :option:`CONFIG_MQTT_KEEPALIVE` to the maximum allowed value, 1767 seconds (29.45 minutes) as specified by Azure IoT Hub.
   This is to limit the IP traffic between the device and the Azure IoT Hub message broker for supporting a low power sample.
   However, note that in certain LTE networks, the NAT timeout can be considerably lower than 1767 seconds.
   So as a recommendation, and to prevent the likelihood of getting disconnected unexpectedly, the option :option:`CONFIG_MQTT_KEEPALIVE` must be set to the lowest of the aforementioned timeout limits (Maximum allowed MQTT keepalive and NAT timeout).

Building and running
********************

.. |sample path| replace:: :file:`samples/nrf9160/azure_iot_hub`
.. include:: /includes/build_and_run_nrf9160.txt
.. include:: /includes/spm.txt

Testing
=======

Microsoft has created `Azure IoT Explorer`_ to interact and test devices connected to an Azure IoT Hub instance.
Optionally, follow the instructions at `Azure IoT Explorer`_ to install and configure the tool and use it as mentioned in the below instructions.

|test_sample|

1. |connect_kit|
#. |connect_terminal|
#. Reset the development kit.
#. Observe the log output and verify that it is similar to the :ref:`sampoutput_azure_iot`.
#. Use the `Azure IoT Explorer`_, or log in to the `Azure Portal`_.
#. Select the connected IoT hub and then your device.
#. Change the device twin's *desired* property ``telemetryInterval`` to a new value, for instance ``10``, and save the updated device twin.
   If it does not exist, you can add the *desired* property.
#. Observe that the device receives the updated ``telemetryInterval`` value, applies it, and starts sending new telemetry events every 10 seconds.
#. Verify that the ``reported`` object in the device twin now has a ``telemetryInterval`` property with the correct value reported back from the device.
#. In the `Azure IoT Explorer`_ or device page in `Azure Portal`_, navigate to the :guilabel:`Direct method` tab.
#. Enter ``led`` as the method name. In the ``payload`` field, enter the value ``1`` (or ``0``) and click :guilabel:`Invoke method`.
#. Observe that LED 1 on the development kit lights up (or switches off if ``0`` is entered as the payload).
   If you are using `Azure IoT Explorer`_, you can observe a notification in the top right corner stating if the direct method was successfully invoked based on the report received from the device.
#. If you are using the `Azure IoT Explorer`_, navigate to the :guilabel:`Telemetry` tab and click :guilabel:`start`.
#. Observe that the event messages from the device are displayed in the terminal within the specified telemetry interval.

.. _sampoutput_azure_iot:

Sample Output
=============

When the sample runs, the device boots, and the sample displays an output identical to the following output in the terminal over UART:

.. code-block:: console

	*** Booting Zephyr OS build v2.3.0-rc1-ncs1-1453-gf41496cd30d5  ***
	Azure IoT Hub sample started
	Connecting to LTE network
	Connected to LTE network
	AZURE_IOT_HUB_EVT_CONNECTING
	AZURE_IOT_HUB_EVT_CONNECTED
	AZURE_IOT_HUB_EVT_READY
	AZURE_IOT_HUB_EVT_TWIN_RECEIVED
	No 'telemetryInterval' object in the device twin
	Sending event:
	{"temperature":25.9,"timestamp":16849}
	Event was successfully sent
	Next event will be sent in 20 seconds

	...

	AZURE_IOT_HUB_EVT_TWIN_DESIRED_RECEIVED
	New telemetry interval has been applied: 60
	AZURE_IOT_HUB_EVT_TWIN_RESULT_SUCCESS, ID: 42740
	Sending event:
	{"temperature":25.5,"timestamp":47585}
	Event was successfully sent
	Next event will be sent in 60 seconds


Dependencies
************

This sample uses the following |NCS| libraries and drivers:

* :ref:`lib_azure_iot_hub`
* :ref:`lte_lc_readme`

It uses the following `sdk-nrfxlib`_ library:

* :ref:`nrfxlib:nrf_modem`

In addition, it uses the following sample:

* :ref:`secure_partition_manager`
