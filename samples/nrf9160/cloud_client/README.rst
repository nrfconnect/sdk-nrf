.. _cloud_client:

nRF9160: Cloud client
#####################

.. contents::
   :local:
   :depth: 2

This sample connects to, and communicates with a compatible cloud service using the respective cloud backend firmware library.
The sample connects via cellular network (LTE) and publishes a custom string in intervals or upon a button trigger, to the cloud service.

Overview
********

The Cloud client sample demonstrates how the generic :ref:`cloud_api_readme` can be used to interface with multiple cloud backends.
The current version of the sample supports the following libraries as cloud backends:

*  :ref:`lib_nrf_cloud`
*  :ref:`lib_aws_iot`
*  :ref:`lib_azure_iot_hub`

To swap between the supported libraries, change the option :option:`CONFIG_CLOUD_BACKEND` to match the configuration string of a compatible cloud backend.
The identification strings for the different cloud backends are listed in the following table:

.. list-table::
   :header-rows: 1
   :align: center

   * - Cloud Backend
     - Configuration String
   * - nRF Connect for Cloud
     - "NRF_CLOUD"
   * - AWS IoT
     - "AWS_IOT"
   * - Azure IoT Hub
     - "AZURE_IOT_HUB"

Requirements
************

The sample supports the following development kits:

.. table-from-rows:: /includes/sample_board_rows.txt
   :header: heading
   :sample-yaml-rows:

Setup
*****

For configuring the different cloud backends, refer to the documentation on :ref:`lib_nrf_cloud`, :ref:`lib_aws_iot`, and :ref:`lib_azure_iot_hub`.
Each cloud backend has specific setup steps that must be executed before it can be used.

.. note::
   The nRF9160 DK and Thingy:91 come preprogrammed with the certificates required for a connection to `nRF Connect for Cloud`_.
   No extra steps are required to use the Cloud client sample with nRF Connect for Cloud.


Configurations
**************

The configurations used in the sample are listed below. They can be added to ``cloud_client/prj.conf``.

.. options-from-kconfig::
   :prefix: "This option "
   :suffix: .
   :show-type:
   :only-visible:

.. note::
   To output data in the terminal window located in the `nRF Connect for Cloud`_ web interface, the data format must be in JSON format.

.. note::
   The sample sets the option :option:`CONFIG_MQTT_KEEPALIVE` to the maximum allowed value that is specified by the configured cloud backend.
   This is to limit the IP traffic between the device and the message broker of the cloud provider for supporting a low power sample.
   However, note that in certain LTE networks, the NAT timeout can be considerably lower than the maximum allowed MQTT keepalive.
   So as a recommendation, and to prevent the likelihood of getting disconnected unexpectedly, the option :option:`CONFIG_MQTT_KEEPALIVE` must be set to the lowest of the aforementioned timeout limits (Maximum allowed MQTT keepalive and NAT timeout).

Functionality and Supported Technologies
****************************************

The communication protocol supported by the sample is dependent on the cloud backend that is used.

Functions
=========
The sample uses the following functions:

* :c:func:`cloud_get_binding` : Binds to a desired cloud backend using an identifiable string.


* :c:func:`cloud_init` : Sets up the cloud connection.


* :c:func:`cloud_connect` : Connects to the cloud service.


* :c:func:`cloud_ping` : Pings the cloud service.


* :c:func:`cloud_input` : Retrieves data from the cloud service.


* :c:func:`cloud_send` : Sends data to the cloud service.


Cloud events used in the sample
===============================
The sample uses the following cloud events:

* :c:enumerator:`CLOUD_EVT_CONNECTED` : Connected to the cloud service.


* :c:enumerator:`CLOUD_EVT_READY` : Ready for cloud communication.


* :c:enumerator:`CLOUD_EVT_DISCONNECTED` : Disconnected from the cloud service.


* :c:enumerator:`CLOUD_EVT_DATA_RECEIVED` : Data received from the cloud service.

.. note::
   Not all functionalities present in the generic cloud API are used by the different cloud backends.

Building and running
********************

.. |sample path| replace:: :file:`samples/nrf9160/cloud_client`
.. include:: /includes/build_and_run.txt
.. include:: /includes/spm.txt

Testing
=======

Before testing, ensure that your device is already set up with your nRF Connect for Cloud account.
After programming the sample to your device, test it by performing the following steps:

1. Open a web browser and navigate to the correct device in `nRF Connect for Cloud`_.
#. Connect the USB cable and power on or reset your device.
#. Open a terminal emulator and observe that the sample has started.
   Wait until the "I: CLOUD_EVT_READY" status appears in the terminal.

   .. code-block:: console

      I: Cloud client has started
      I: Connecting to LTE network, this may take several minutes...
      +CEREG: 2,"7725","0138E000",7,0,0,"11100000","11100000"
      +CSCON: 1
      +CEREG: 1,"7725","0138E000",7,,,"00000010","00000110"
      I: Network registration status: Connected - home network
      I: Connected to LTE network
      I: Connecting to cloud
      I: CLOUD_EVT_CONNECTED
      I: CLOUD_EVT_DATA_RECEIVED
      I: Data received from cloud: {"desired":{"pairing":{"state":"paired","topics":{"d2c":..
      I: CLOUD_EVT_PAIR_DONE
      I: CLOUD_EVT_READY

    The device is now connected to nRF Connect for Cloud.

#. Press button 1 on the device and observe that the following output is displayed in the terminal:

   .. code-block:: console

      I: Publishing message: {"state":{"reported":{"message":"Hello Internet of Things!"}}}
      +CSCON: 1

#. Observe that the following status appears in the terminal pane for the connected device in nRF Connect for Cloud:

   .. code-block:: console

      "Received": {
         "state": {
            "reported": {
               "message": "Hello Internet of Things!"
            }
         }
      }



Dependencies
************

This sample uses the following |NCS| libraries and drivers:

* :ref:`lib_nrf_cloud`
* :ref:`lib_aws_iot`
* :ref:`dk_buttons_and_leds_readme`
* :ref:`cloud_api_readme`
* :ref:`lte_lc_readme`

It uses the following `sdk-nrfxlib`_ library:

* :ref:`nrfxlib:nrf_modem`

In addition, it uses the following sample:

* :ref:`secure_partition_manager`
