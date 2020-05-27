.. _liblwm2m_carrier_readme:

LwM2M carrier
#############

You can use the LwM2M carrier library in your nRF91 application in order to connect to the LwM2M operator network.
Integrating this library into an application causes the device to connect to the operator's device management platform autonomously.

Some examples of carrier device management platforms are listed below:

* `Verizon's Thingspace`_
* `AT&T's IoT Platform`_

This library uses LwM2M protocol to communicate with device manager platforms, but it does not expose an LwM2M API to your application. If you want to use LwM2M for other purposes, see :ref:`lwm2m_interface`.

The :ref:`lwm2m_carrier` sample demonstrates how to run this library in an application.
The LwM2M carrier library is also used in the :ref:`asset_tracker` application.

Application integration
***********************

The LwM2M carrier library has an OS abstraction layer.
See :file:`lwm2m_os.h`.
This makes the LwM2M carrier library independent of the underlying implementation of primitives such as timers, non-volatile storage, and heap allocation.
The OS abstraction is fully implemented for the |NCS|, and it would have to be ported if used on other systems.

To run the library in an application, you must implement the application with the API of the library.
You can enable the module using the :option:`CONFIG_LWM2M_CARRIER` Kconfig option.

The :ref:`lwm2m_carrier` sample project configuration (:file:`nrf/samples/nrf9160/lwm2m_carrier/prj.conf`) contains all the configurations that are needed by the LwM2M carrier library.

A bootstrap server provides the application with the configuration needed to connect to the device management servers.
During self-testing of the certification process, you can provide the initialization parameter :cpp:type:`lwm2m_carrier_config_t` to overwrite the carrier default settings as described below:

 * Specify the URI of the bootstrap server by using the :option:`CONFIG_LWM2M_CARRIER_USE_CUSTOM_BOOTSTRAP_URI` and :option:`CONFIG_LWM2M_CARRIER_CUSTOM_BOOTSTRAP_URI` Kconfig options.
 * Specify the `Pre-Shared Key (PSK)`_ by using the :option:`CONFIG_LWM2M_CARRIER_USE_CUSTOM_BOOTSTRAP_PSK` and :option:`CONFIG_LWM2M_CARRIER_CUSTOM_BOOTSTRAP_PSK` Kconfig options.
 * Specify whether to connect to certification servers or production servers by using the :option:`CONFIG_LWM2M_CARRIER_CERTIFICATION_MODE` Kconfig option.

A PSK *Identity* is automatically generated and handled by the LwM2M carrier library.
It is used for the bootstrap server transactions.
A PSK *Key*  should be provided through the :option:`CONFIG_LWM2M_CARRIER_USE_CUSTOM_BOOTSTRAP_PSK` option only if the carrier explicitly states to not use the carrier default.

.. note::
   A change of the bootstrap server URI between builds does not trigger a new bootstrap.
   The bootstrap process is intended to happen only once, unless it is initiated from the server.
   To redo the bootstrap process, you must erase the flash and then load your application again.

For a production build, the :c:func:`lwm2m_carrier_init` function should always be initialized without parameters.
After calling the :cpp:func:`lwm2m_carrier_init` function, your application can call the non-returning function :cpp:func:`lwm2m_carrier_run` in its own thread.
Both these functions are called in :file:`nrf\\lib\\bin\\lwm2m_carrier\\os\\lwm2m_carrier.c`, which is included into the project when you enable the LwM2M carrier library.

The :cpp:func:`lwm2m_carrier_event_handler` function must be implemented by your application.
This is shown in the :ref:`lwm2m_carrier` sample.
A weak implementation is included in :file:`nrf\\lib\\bin\\lwm2m_carrier\\os\\lwm2m_carrier.c`.

See :file:`nrf\\lib\\bin\\lwm2m_carrier\\include\\lwm2m_carrier.h` for all the events and API.


LwM2M carrier library events
============================

:c:macro:`LWM2M_CARRIER_EVENT_BSDLIB_INIT`
   This event indicates that the :ref:`bsdlib` is initialized and can be used.
   (See :ref:`req_appln_limitations`).

:c:macro:`LWM2M_CARRIER_EVENT_CONNECTING`, :c:macro:`LWM2M_CARRIER_EVENT_CONNECTED`, :c:macro:`LWM2M_CARRIER_EVENT_DISCONNECTING`, :c:macro:`LWM2M_CARRIER_EVENT_DISCONNECTED`
   These events indicate that the device is connecting to or disconnecting from the LTE network.
   They occur during the bootstrapping process, startup, and during FOTA.

:c:macro:`LWM2M_CARRIER_EVENT_BOOTSTRAPPED`
   This event indicates that the bootstrap sequence is complete, and that the device is ready to be registered.
   This event is typically seen during the first boot-up.

:c:macro:`LWM2M_CARRIER_EVENT_READY`
   This event indicates that the device has connected successfully to the carrier's device management server.

:c:macro:`LWM2M_CARRIER_EVENT_DEFERRED`
   This event indicates that the connection to the device management server has failed.
   The :c:macro:`LWM2M_CARRIER_EVENT_DEFERRED` event appears instead of the :c:macro:`LWM2M_CARRIER_EVENT_READY` event.
   The :c:member:`timeout` parameter supplied with this event determines when the LwM2M carrier library will retry the connection.

:c:macro:`LWM2M_CARRIER_DEFERRED_NO_REASON`
   The application need not take any special action.
   If :c:member:`timeout` is 24 hours, the application can proceed with other activities until the retry takes place.

:c:macro:`LWM2M_CARRIER_DEFERRED_PDN_ACTIVATE`
   This event indicates problem with the SIM card, or temporary network problems.
   If this persists, contact your carrier.

:c:macro:`LWM2M_CARRIER_DEFERRED_BOOTSTRAP_CONNECT`
   The DTLS handshake with the bootstrap server has failed.
   If the application is using a custom PSK, verify that the format is correct.

:c:macro:`LWM2M_CARRIER_DEFERRED_BOOTSTRAP_SEQUENCE`
   The bootstrap sequence is incomplete.
   The server failed either to acknowledge the request by the library, or to send objects to the library.
   Confirm that the carrier is aware of the IMEI.

:c:macro:`LWM2M_CARRIER_DEFERRED_SERVER_NO_ROUTE`, :c:macro:`LWM2M_CARRIER_DEFERRED_BOOTSTRAP_NO_ROUTE`
   There is a routing problem in the carrier network.
   If this event persists, contact the carrier.

:c:macro:`LWM2M_CARRIER_DEFERRED_SERVER_CONNECT`
   This event indicates that the DTLS handshake with the server has failed.
   This typically happens if the bootstrap sequence has failed on the carrier side.

:c:macro:`LWM2M_CARRIER_DEFERRED_SERVER_REGISTRATION`
   The server registration has not completed, and the server does not recognize the connecting device.
   If this event persists, contact the carrier.

:c:macro:`LWM2M_CARRIER_EVENT_FOTA_START`
   This event indicates that the modem update has started.
   The application should immediately terminate any open TLS sessions.
   See :ref:`req_appln_limitations`.

:c:macro:`LWM2M_CARRIER_EVENT_REBOOT`
   If the application is not ready to reboot, it must return non-zero and then reboot at the earliest convenient time.

:c:macro:`LWM2M_CARRIER_EVENT_ERROR`
   This event indicates an error.
   The event data struct :c:type:`lwm2m_carrier_event_error_t` contains the information about the error (:cpp:member:`code` and :cpp:member:`value`).

   :c:macro:`LWM2M_CARRIER_ERROR_CONNECT_FAIL`
      This error is generated from the :cpp:func:`lte_lc_init_and_connect` function.
      It indicates possible problems with the SIM card, or insufficient network coverage.
      See :cpp:member:`value` field of the event.

   :c:macro:`LWM2M_CARRIER_ERROR_DISCONNECT_FAIL`
      This error is generated from the :cpp:func:`lte_lc_offline` function.
      See :cpp:member:`value` field of the event.

   :c:macro:`LWM2M_CARRIER_ERROR_BOOTSTRAP`
      This error is generated from the :cpp:func:`modem_key_mgmt_write` function, if the :cpp:member:`value` field is negative.
      If the :cpp:member:`value` field is 0, it indicates that the bootstrap sequence has failed.
      If this error persists, contact your carrier.

   :c:macro:`LWM2M_CARRIER_ERROR_FOTA_PKG`
      This error indicates that the update package has been rejected.
      The integrity check has failed because of a wrong package sent from the server, or a wrong package received by client.
      The :cpp:member:`value` field will have an error of type :c:type:`nrf_dfu_err_t` from the file :file:`nrfxlib\\bsdlib\\include\\nrf_socket.h`.

   :c:macro:`LWM2M_CARRIER_ERROR_FOTA_PROTO`
      This error indicates a protocol error.
      There might be unexpected HTTP header contents.
      The server might not support partial content requests.

   :c:macro:`LWM2M_CARRIER_ERROR_FOTA_CONN`
      This error indicates a connection problem.
      Either the server host name could not be resolved, or the remote server refused the connection.

   :c:macro:`LWM2M_CARRIER_ERROR_FOTA_CONN_LOST`
      This error indicates a loss of connection, or an unexpected closure of connection by the server.

   :c:macro:`LWM2M_CARRIER_ERROR_FOTA_FAIL`
      This error indicates a failure in applying a valid update.
      If this error persists, create a ticket in `DevZone`_ with the modem trace.




Device Objects
==============

The following values that reflect the state of the device must be kept up to date by the application:

* Available Power Sources
* Power Source Voltage
* Power Source Current
* Battery Level
* Battery Status
* Memory Total
* Error Code
* Device Type (Defaults to ``Smart Device`` if not set)
* Software Version (Defaults to ``LwM2M <libversion>``. For example, ``LwM2M 0.8.0`` for release 0.8.0.)
* Hardware Version (Defaults to ``1.0`` if not set)

For example, the carrier device management platform can observe the battery level of your device.
The application uses the :cpp:func:`lwm2m_carrier_battery_level_set` function to indicate the current battery level of the device to the carrier.

.. _req_appln_limitations:

Requirements and Application limitations
****************************************

Below are some of the requirements and limitations of the application while running this module.

* The application should not call the :cpp:func:`bsdlib_init` function.

   * The LwM2M carrier library initializes and uses the :ref:`bsdlib`.
     This library is needed to track the modem FOTA states.


* The LwM2M carrier library controls the LTE link.

   * This is needed for storing keys to the modem, which requires disconnecting from an LTE link and connecting to it.
   * The application should not connect to the LTE link between a :c:macro:`LWM2M_CARRIER_EVENT_DISCONNECTING` event and :c:macro:`LWM2M_CARRIER_EVENT_CONNECTED` event.

* The LwM2M carrier library uses a TLS session for FOTA.
  TLS handshakes performed by the application might fail if the LwM2M carrier library is performing one at the same time.

   * If the application is in a TLS session and a :c:type:`LWM2M_CARRIER_EVENT_FOTA_START` event is sent to the application, the application must immediately end the TLS session.
   * TLS becomes available again upon the next :c:type:`LWM2M_CARRIER_EVENT_READY` or :c:type:`LWM2M_CARRIER_EVENT_DEFERRED` event.
   * The application should implement a retry mechanism so that the application can perform the TLS handshake later.
   * Another alternative is to supply TLS from a source other than the modem, for example `Mbed TLS`_.

* The LwM2M carrier library uses both the DTLS sessions made available through the modem. Therefore, the application can not run any DTLS sessions.

* The LwM2M carrier library provisions the necessary security credentials to the security tags 11, 25, 26, 27, 28.
  These tags should not be used by the application.

* The LwM2M carrier library uses the following NVS record key range: 0xCA00 - 0xCAFF.
  This range should not be used by the application.

Certification and Version Dependencies
**************************************

Every released version of the LwM2M carrier library is considered for certification with the carriers.
The library is certified together with specific versions of the modem firmware and the |NCS|.
Refer to the :ref:`liblwm2m_carrier_changelog` to check the certification status of a particular version of the library, and to see the version of the |NCS| it was released with.

.. note::

   Your final product will need certification from the carrier.
   You can contact the carrier for more information on the respective device certification program.

.. _versiondep_table:

Version Dependency table
========================

See the following table for the certification status of the library and the related version dependencies:

+-----------------+---------------+---------------+---------------+
| LwM2M carrier   | Modem version | |NCS| release | Certification |
| library version |               |               |               |
+=================+===============+===============+===============+
| 0.9.0           | 1.2.1         | 1.3.0         |               |
+-----------------+---------------+---------------+---------------+
| 0.8.1+build1    | 1.1.0         | 1.2.0         | Verizon       |
+-----------------+---------------+---------------+---------------+
| 0.8.0           | 1.1.0         | 1.1.0         |               |
+-----------------+---------------+---------------+---------------+
| 0.6.0           | 1.0.1         | 1.0.0         |               |
+-----------------+---------------+---------------+---------------+


LwM2M carrier library changelog
*******************************

See the changelog for the latest updates in the LwM2M carrier library, and for a list of known issues and limitations.

.. toctree::
    :maxdepth: 1

    doc/CHANGELOG.rst

Message Sequence Charts
***********************

The below message sequence chart shows the initialization of the LwM2M carrier library:


.. msc::
    hscale = "1.1";
    Application,"LwM2M carrier Library";
    Application=>"LwM2M carrier Library"       [label="lwm2m_carrier_init()"];
    Application<<="LwM2M carrier Library"      [label="LWM2M_CARRIER_EVENT_BSDLIB_INIT"];
    Application<<="LwM2M carrier Library"      [label="LWM2M_CARRIER_EVENT_CONNECTING"];
    Application<<="LwM2M carrier Library"      [label="LWM2M_CARRIER_EVENT_CONNECTED"];
    Application<<"LwM2M carrier Library"       [label="Success"];
    Application=>"LwM2M carrier Library"       [label="lwm2m_carrier_run()"];
    |||;
    "LwM2M carrier Library" :> "LwM2M carrier Library" [label="(no return)"];
    ...;



The following message sequence chart shows the bootstrap process:

.. msc::
    hscale = "1.1";
    Application,"LwM2M carrier Library";
    |||;
    ---                                       [label="Device is not bootstrapped yet"];
    |||;
    Application<<="LwM2M carrier Library"      [label="LWM2M_CARRIER_EVENT_DISCONNECTING"];
    Application<<="LwM2M carrier Library"      [label="LWM2M_CARRIER_EVENT_DISCONNECTED"];
    "LwM2M carrier Library" rbox "LwM2M carrier Library" [label="Write keys to modem"];
    Application<<="LwM2M carrier Library"      [label="LWM2M_CARRIER_EVENT_CONNECTING"];
    Application<<="LwM2M carrier Library"      [label="LWM2M_CARRIER_EVENT_CONNECTED"];
    Application<<="LwM2M carrier Library"      [label="LWM2M_CARRIER_EVENT_BOOTSTRAPPED"];
    Application<<="LwM2M carrier Library"      [label="LWM2M_CARRIER_EVENT_READY"];


The following message sequence chart shows FOTA updates:

.. msc::
    hscale = "1.1";
    Application,"LwM2M carrier Library";
    |||;
    ---                                       [label="Carrier initiates modem update"];
    |||;
    Application<<="LwM2M carrier Library"      [label="LWM2M_CARRIER_EVENT_FOTA_START"];
    ...;
    Application<<="LwM2M carrier Library"      [label="LWM2M_CARRIER_EVENT_DISCONNECTING"];
    Application<<="LwM2M carrier Library"      [label="LWM2M_CARRIER_EVENT_DISCONNECTED"];
    Application<<="LwM2M carrier Library"      [label="LWM2M_CARRIER_EVENT_CONNECTING"];
    Application<<="LwM2M carrier Library"      [label="LWM2M_CARRIER_EVENT_CONNECTED"];
    Application<<="LwM2M carrier Library"      [label="LWM2M_CARRIER_EVENT_READY"];
    ...;



API documentation
*****************

| Header files: :file:`lib/bin/lwm2m_carrier/include`
| Source files: :file:`lib/bin/lwm2m_carrier`

LWM2M carrier library API
=========================

.. doxygengroup:: lwm2m_carrier_api
   :project: nrf
   :members:

LWM2M carrier library events
----------------------------

.. doxygengroup:: lwm2m_carrier_event
   :project: nrf
   :members:

LWM2M OS layer
==============

.. doxygengroup:: lwm2m_carrier_os
   :project: nrf
   :members:
