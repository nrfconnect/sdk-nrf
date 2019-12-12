.. _cloud_client:

nRF9160: Cloud Client
#####################

This sample connects to, and communicates with a compatible cloud service using the respective cloud backend firmware library.
The sample connects via cellular network (LTE) and publishes a custom string in intervals or upon a button trigger, to the cloud service.

Overview
********

The Cloud Client sample demonstrates how the generic :ref:`cloud_api_readme` can be used to interface with multiple cloud backends.
The current version of the sample supports the following libraries as cloud backends:

 -  :ref:`lib_nrf_cloud`
 -  :ref:`lib_aws_iot`

To swap between the supported libraries, change the option :option:`CONFIG_CLOUD_BACKEND` to match the configuration string of a compatible cloud backend.
The identifying string for the different cloud backends are listed in the following table:

.. list-table::
   :header-rows: 1
   :align: center

   * - Cloud Backend
     - Configuration String
   * - NRF Cloud
     - NRF_CLOUD
   * - AWS IoT
     - AWS_IOT

Requirements
************

* One of the following development boards:

 * |Thingy91|
 * |nRF9160DK|

Building and running
********************

.. |sample path| replace:: :file:`samples/nrf9160/cloud_client`
.. include:: /includes/build_and_run.txt
.. include:: /includes/spm.txt

Setup
*****

For configuring the different cloud backends, refer to the documentation on :ref:`lib_nrf_cloud` and :ref:`lib_aws_iot`.
Each cloud backend has specific setup steps that must be executed before it can be used.

.. note::
   The nRF9160 DK and Thingy:91 come pre-flashed with the certificates required for a connection to `nRF Cloud`_.
   No extra steps are required to use the Cloud client sample with nRF Cloud.


Configurations
**************

The configurations used in the sample are listed below. They are located in ``cloud_client/src/prj.conf``.


.. option:: CONFIG_CLOUD_BACKEND

Decides the cloud backend to be used.

.. option:: CONFIG_CLOUD_PUBLICATION_SEQUENTIAL

Publishes a message to cloud sequentially.

.. option:: CONFIG_CLOUD_PUBLICATION_BUTTON_PRESS

Publishes a message to cloud upon a button press.

.. option:: CONFIG_CLOUD_MESSAGE

Modifies the message published to the cloud service.

.. option:: CONFIG_CLOUD_MESSAGE_PUBLICATION_INTERVAL

Modifies the interval within which the message is published to the cloud service.

.. note::
   To output data in the terminal window located in the `nRF Cloud`_ web interface, the data format must be in JSON format.

Functionality and Supported Technologies
****************************************

The communication protocol supported by the sample is dependent on the cloud backend that is used.

Functions
=========
The sample uses the following functions:

* :cpp:func:`cloud_get_binding()` : Binds to a desired cloud backend using a identifiable string.


* :cpp:func:`cloud_init()` : Sets up the cloud connection.


* :cpp:func:`cloud_connect()` : Connects to the cloud service.


* :cpp:func:`cloud_ping()` : Pings the cloud service.


* :cpp:func:`cloud_input()` : Retrieves data from the cloud service.


* :cpp:func:`cloud_send()` : Sends data to the cloud service.


Cloud events used in the sample
===============================
The sample uses the following cloud events:

* :cpp:enumerator:`CLOUD_EVT_CONNECTED <cloud_api::CLOUD_EVT_CONNECTED>` : Connected to the cloud service.


* :cpp:enumerator:`CLOUD_EVT_READY<cloud_api::CLOUD_EVT_READY>` : Ready for cloud communication.


* :cpp:enumerator:`CLOUD_EVT_DISCONNECTED<cloud_api::CLOUD_EVT_DISCONNECTED>` : Disconnected from the cloud service.


* :cpp:enumerator:`CLOUD_EVT_DATA_RECEIVED<cloud_api::CLOUD_EVT_DATA_RECEIVED>` : Data received from the cloud service.

.. note::
   Not all functionalities present in the generic cloud API are used by the different cloud backends.


Dependencies
************

This sample uses the following |NCS| libraries and drivers:

    * :ref:`lib_nrf_cloud`
    * :ref:`lib_aws_iot`
    * :ref:`dk_buttons_and_leds_readme`
    * :ref:`cloud_api_readme`
    * ``lib/bsd_lib``
    * ``lib/lte_link_control``

In addition, it uses the Secure Partition Manager sample:

* :ref:`secure_partition_manager`
