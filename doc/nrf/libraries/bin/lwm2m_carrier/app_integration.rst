.. _lwm2m_app_int:

Application integration
#######################

.. contents::
   :local:
   :depth: 2

The LwM2M carrier library has an OS abstraction layer.
See :file:`lwm2m_os.h`.
This abstraction layer makes the LwM2M carrier library independent of the |NCS| modules and underlying implementation of primitives such as timers, non-volatile storage, and heap allocation.
It provides an abstraction of the following modules:

* |NCS| modules:

  .. lwm2m_osal_mod_list_start

  * ``drivers/lte_link_control``
  * :ref:`lib_download_client`
  * :ref:`modem_key_mgmt`
  * :ref:`at_cmd_readme`
  * :ref:`at_cmd_parser_readme`
  * :ref:`at_notif_readme`
  * :ref:`sms_readme`
  * :ref:`pdn_readme`

  .. lwm2m_osal_mod_list_end

* Zephyr modules:

  * :ref:`zephyr:kernel_api` (``include/kernel.h``)
  * :ref:`zephyr:nvs_api`

The OS abstraction layer is fully implemented for the |NCS|, and it would have to be ported if used with other RTOS or on other systems.

To run the library in an application, you must implement the application with the API of the library.
You can enable the module using the :kconfig:`CONFIG_LWM2M_CARRIER` Kconfig option.

The :ref:`lwm2m_carrier` sample project configuration (:file:`nrf/samples/nrf9160/lwm2m_carrier/prj.conf`) contains all the configurations that are needed by the LwM2M carrier library.

You can provide the initialization parameter :c:type:`lwm2m_carrier_config_t` to overwrite the carrier default settings with the Kconfig options described below:


* :kconfig:`CONFIG_LWM2M_CARRIER_CERTIFICATION_MODE`:

  * This configuration specifies if the LwM2M carrier library will connect to the carrier's certification servers or production servers.
  * Expected to be set during self-testing part of the certification process.
  * This configuration is ignored if :kconfig:`CONFIG_LWM2M_CARRIER_USE_CUSTOM_BOOTSTRAP_URI` is set.

* :kconfig:`CONFIG_LWM2M_CARRIER_USE_CUSTOM_BOOTSTRAP_URI`:

  * This configuration, together with :kconfig:`CONFIG_LWM2M_CARRIER_CUSTOM_BOOTSTRAP_URI` lets the LwM2M carrier library connect to a bootstrap server other than the normal carrier server, thereby enabling the generic mode.
  * Expected to be set during self-testing, or if the end product is not to be certified with the applicable carriers. See :ref:`lwm2m_certification`.
  * If this configuration is set, :kconfig:`CONFIG_LWM2M_CARRIER_CERTIFICATION_MODE` is ignored.

* :kconfig:`CONFIG_LWM2M_CARRIER_USE_CUSTOM_BOOTSTRAP_PSK`:

  * This configuration, together with :kconfig:`CONFIG_LWM2M_CARRIER_CUSTOM_BOOTSTRAP_PSK` can be set to use a non-default `Pre-Shared Key (PSK)`_.
  * If connecting to the normal carrier device management servers (normal operation), this configuration must not be set unless your carrier explicitly states to use a custom PSK, for example during self-testing.
  * If the :kconfig:`CONFIG_LWM2M_CARRIER_CUSTOM_BOOTSTRAP_URI` option is set and the server requires a secure connection, a PSK is required.

* :kconfig:`CONFIG_LWM2M_CARRIER_USE_CUSTOM_APN`:

  * This configuration, together with :kconfig:`CONFIG_LWM2M_CARRIER_CUSTOM_APN` produce different results depending on normal or generic mode of operation.
  * If :kconfig:`CONFIG_LWM2M_CARRIER_USE_CUSTOM_BOOTSTRAP_URI` is not set (normal), this configuration supplies a fallback APN. This might be required in your application, depending on the requirements from the carrier.
  * If :kconfig:`CONFIG_LWM2M_CARRIER_USE_CUSTOM_BOOTSTRAP_URI` is set (generic), :kconfig:`CONFIG_LWM2M_CARRIER_CUSTOM_APN` is used instead of the default APN, (and there is no fallback APN).

Note that these settings can put the LwM2M carrier library either in the normal mode where it connects to the applicable carriers, or in the generic mode where it can connect to any bootstrap server.

.. note::
   A change of the bootstrap server URI between builds does not trigger a new bootstrap.
   The bootstrap process is intended to happen only once unless it is initiated from the server.
   To redo the bootstrap process, you must erase the flash and then load your application again.

After calling the :c:func:`lwm2m_carrier_init` function, your application can call the non-returning function :c:func:`lwm2m_carrier_run` in its own thread.
Both these functions are called in :file:`nrf\\lib\\bin\\lwm2m_carrier\\os\\lwm2m_carrier.c`, which is included into the project when you enable the LwM2M carrier library.

The :c:func:`lwm2m_carrier_event_handler` function must be implemented by your application.
This is shown in the :ref:`lwm2m_carrier` sample.
A weak implementation is included in :file:`nrf\\lib\\bin\\lwm2m_carrier\\os\\lwm2m_carrier.c`.

See :file:`nrf\\lib\\bin\\lwm2m_carrier\\include\\lwm2m_carrier.h` for all the events and API.

.. _lwm2m_events:

LwM2M carrier library events
****************************

Following are the various LwM2M carrier library events:

* :c:macro:`LWM2M_CARRIER_EVENT_MODEM_INIT`:

  * This event indicates that the :ref:`nrf_modem` is initialized and can be used.
    See :ref:`req_appln_limitations`).

* :c:macro:`LWM2M_CARRIER_EVENT_CERTS_INIT`:

  * This event indicates that the CA certificates must be provisioned to the modem by the application.
    The LwM2M carrier library expects the application to assign the event data :c:member:`lwm2m_carrier_event_t.data` to a structure :c:type:`ca_cert_tags_t`.
    This structure must contain the security tags that indicate where the CA certificates are stored in the modem.
    See :ref:`lwm2m_carrier` sample for an example of how these certificates are written to the modem using :ref:`modem_key_mgmt` library.
    The LwM2M carrier library will apply these certificates during certain out-of-band FOTA operations.

* :c:macro:`LWM2M_CARRIER_EVENT_CONNECTING`, :c:macro:`LWM2M_CARRIER_EVENT_CONNECTED`, :c:macro:`LWM2M_CARRIER_EVENT_DISCONNECTING`, :c:macro:`LWM2M_CARRIER_EVENT_DISCONNECTED`:

  * These events indicate that the device is connecting to or disconnecting from the LTE network.
    They occur during the bootstrapping process, startup, and during FOTA.

* :c:macro:`LWM2M_CARRIER_EVENT_BOOTSTRAPPED`:

  * This event indicates that the bootstrap sequence is complete, and that the device is ready to be registered.
  * This event is typically seen during the first boot-up.

* :c:macro:`LWM2M_CARRIER_EVENT_LTE_READY`:

  * This event indicates that the device is registered to the LTE network (home or roaming).
  * The bootstrap sequence is complete, and the application can use the LTE link.

* :c:macro:`LWM2M_CARRIER_EVENT_REGISTERED`:

  * This event indicates that the device has registered successfully to the carrier's device management servers.

* :c:macro:`LWM2M_CARRIER_EVENT_DEFERRED`:

  * This event indicates that the connection to the device management server has failed.
  * The :c:macro:`LWM2M_CARRIER_EVENT_DEFERRED` event appears instead of the :c:macro:`LWM2M_CARRIER_EVENT_REGISTERED` event.
  * The :c:member:`timeout` parameter supplied with this event determines when the LwM2M carrier library will retry the connection.
  * Following are the various deferred reasons:

    * :c:macro:`LWM2M_CARRIER_DEFERRED_NO_REASON` - The application need not take any special action. If :c:member:`timeout` is 24 hours, the application can proceed with other activities until the retry takes place.

    * :c:macro:`LWM2M_CARRIER_DEFERRED_PDN_ACTIVATE` - This event indicates problem with the SIM card, or temporary network problems. If this persists, contact your carrier.

    * :c:macro:`LWM2M_CARRIER_DEFERRED_BOOTSTRAP_CONNECT` - The DTLS handshake with the bootstrap server has failed. If the application is using a custom PSK, verify that the format is correct.

    * :c:macro:`LWM2M_CARRIER_DEFERRED_BOOTSTRAP_SEQUENCE` - The bootstrap sequence is incomplete. The server failed either to acknowledge the request by the library, or to send objects to the library. Confirm that the carrier is aware of the IMEI.

    * :c:macro:`LWM2M_CARRIER_DEFERRED_SERVER_NO_ROUTE`, :c:macro:`LWM2M_CARRIER_DEFERRED_BOOTSTRAP_NO_ROUTE` - There is a routing problem in the carrier network. If this event persists, contact the carrier.

    * :c:macro:`LWM2M_CARRIER_DEFERRED_SERVER_CONNECT` - This event indicates that the DTLS handshake with the server has failed. This typically happens if the bootstrap sequence has failed on the carrier side.

    * :c:macro:`LWM2M_CARRIER_DEFERRED_SERVER_REGISTRATION` - The server registration has not completed, and the server does not recognize the connecting device. If this event persists, contact the carrier.

* :c:macro:`LWM2M_CARRIER_EVENT_FOTA_START`:

  * This event indicates that the modem update has started.
  * The application must immediately terminate any open TLS sessions.
  * See :ref:`req_appln_limitations`.

* :c:macro:`LWM2M_CARRIER_EVENT_REBOOT`:

  * This event indicates that the LwM2M carrier library will reboot the device.
  * If the application is not ready to reboot, it must return non-zero and then reboot at the earliest convenient time.

* :c:macro:`LWM2M_CARRIER_EVENT_ERROR`:

  * This event indicates an error.
  * The event data struct :c:type:`lwm2m_carrier_event_error_t` contains the information about the error (:c:member:`code` and :c:member:`value`).
  * Following are the valid error codes:

    * :c:macro:`LWM2M_CARRIER_ERROR_CONNECT_FAIL` - This error is generated from the :c:func:`lte_lc_init_and_connect` function. It indicates possible problems with the SIM card, or insufficient network coverage. See :c:member:`value` field of the event.

    * :c:macro:`LWM2M_CARRIER_ERROR_DISCONNECT_FAIL` - This error is generated from the :c:func:`lte_lc_offline` function. See :c:member:`value` field of the event.

    * :c:macro:`LWM2M_CARRIER_ERROR_BOOTSTRAP` - This error is generated from the :c:func:`modem_key_mgmt_write` function, if the :c:member:`value` field is negative. If the :c:member:`value` field is 0, it indicates that the bootstrap sequence has failed. If this error persists, contact your carrier.

    * :c:macro:`LWM2M_CARRIER_ERROR_FOTA_PKG` - This error indicates that the update package has been rejected. The integrity check has failed because of a wrong package sent from the server, or a wrong package received by client. The :c:member:`value` field will have an error of type :c:type:`nrf_dfu_err_t` from the file :file:`nrfxlib\\nrf_modem\\include\\nrf_socket.h`.

    * :c:macro:`LWM2M_CARRIER_ERROR_FOTA_PROTO` - This error indicates a protocol error. There might be unexpected HTTP header contents. The server might not support partial content requests.

    * :c:macro:`LWM2M_CARRIER_ERROR_FOTA_CONN` - This error indicates a connection problem. Either the server host name could not be resolved, or the remote server refused the connection.

    * :c:macro:`LWM2M_CARRIER_ERROR_FOTA_CONN_LOST` - This error indicates a loss of connection, or an unexpected closure of connection by the server.

    * :c:macro:`LWM2M_CARRIER_ERROR_FOTA_FAIL` - This error indicates a failure in applying a valid update. If this error persists, create a ticket in `DevZone`_ with the modem trace.

    * :c:macro:`LWM2M_CARRIER_ERROR_SERVICE_UNAVAILABLE` - This error indicates that the LWM2M Server in maintenance mode.

    * :c:macro:`LWM2M_CARRIER_ERROR_CONFIGURATION` - This error indicates that an illegal object configuration was detected.

Device objects
**************

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
The application uses the :c:func:`lwm2m_carrier_battery_level_set` function to indicate the current battery level of the device to the carrier.
