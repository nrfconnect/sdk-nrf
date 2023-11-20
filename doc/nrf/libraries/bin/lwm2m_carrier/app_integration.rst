.. _lwm2m_app_int:

Application integration
#######################

.. contents::
   :local:
   :depth: 2

This section describes the needed interaction between the LwM2M carrier library and the user application.

.. figure:: /libraries/bin/lwm2m_carrier/images/lwm2m_carrier_overview.svg
    :alt: Interaction between the LwM2M carrier library and the user application

    Interaction between the LwM2M carrier library and the user application


The LwM2M carrier library has an OS abstraction layer.
This abstraction layer makes the LwM2M carrier library independent of the |NCS| modules and underlying implementation of primitives such as timers, non-volatile storage, and heap allocation.
For more information, see :file:`lwm2m_os.h`.

.. figure:: /libraries/bin/lwm2m_carrier/images/lwm2m_carrier_os_abstraction.svg
    :alt: LwM2M library OS abstraction overview

    LwM2M library OS abstraction overview

It provides an abstraction of the following modules:

* |NCS| modules:

  .. lwm2m_osal_mod_list_start

  * :ref:`at_monitor_readme`
  * :ref:`lib_download_client`
  * :ref:`sms_readme`
  * :ref:`pdn_readme`
  * :ref:`lib_dfu_target`

  .. lwm2m_osal_mod_list_end

* Zephyr modules:

  * :ref:`zephyr:kernel_api` (``include/kernel.h``)
  * :ref:`zephyr:nvs_api`

The OS abstraction layer is fully implemented for the |NCS|, and it needs to be ported if used with other RTOS or on other systems.

When the LwM2M carrier library is enabled in your application, it includes the file :file:`nrf/lib/bin/lwm2m_carrier/os/lwm2m_carrier.c`.
This automatically runs the library's main function (:c:func:`lwm2m_carrier_main`).

.. _lwm2m_configuration:

Configuration
*************

To run the library in an application, you must implement the application with the API of the library.
Enable the library by setting the :kconfig:option:`CONFIG_LWM2M_CARRIER` Kconfig option to ``y``.

The :ref:`lwm2m_carrier` sample project configuration (:file:`nrf/samples/cellular/lwm2m_carrier/prj.conf`) contains all the configurations that are needed by the LwM2M carrier library.

To overwrite the carrier default settings, you can provide the initialization parameter :c:type:`lwm2m_carrier_config_t` with the Kconfig options specified in the following sections.
You can also use the provided :ref:`lwm2m_carrier_shell` to quickly get started and experiment with the API.

.. _general_options_lwm2m:

General options
===============

Following are some of the general Kconfig options that you can configure:

* :kconfig:option:`CONFIG_LWM2M_CARRIER_BOOTSTRAP_SMARTCARD`:

  * This configuration allows the LwM2M carrier library to use the bootstrap information stored on the SIM card.
    The configuration in the SIM will take precedence over any other configuration.
    For example, if a bootstrap server URI is fetched from the SIM, the configuration set by the :kconfig:option:`CONFIG_LWM2M_CARRIER_CUSTOM_URI` Kconfig option is ignored.

* :kconfig:option:`CONFIG_LWM2M_CARRIER_SESSION_IDLE_TIMEOUT`:

  * This configuration specifies the session idle timeout (inactivity).
    Upon timeout, the LwM2M carrier library disconnects from one or more device management servers.
  * The timeout closes the DTLS session.
    A new DTLS session will be created on the next activity (for example, lifetime trigger).
  * Leaving this configuration empty (``0``) sets it to a default of 60 seconds.
  * This configuration does not apply when the DTLS session is using Connection ID.

* :kconfig:option:`CONFIG_LWM2M_CARRIER_COAP_CON_INTERVAL`:

  * This configuration specifies how often to send a Confirmable message instead of a Non-Confirmable message, according to RFC 7641 section 4.5.
  * Leaving this configuration empty (``0``) sets it to a default of 24 hours.
  * Setting this to -1 will always use Confirmable notifications.

* :kconfig:option:`CONFIG_LWM2M_CARRIER_CUSTOM_APN`:

  * This configuration produces different results depending on normal or generic mode of operation.
  * If the connected device management server does not support the APN Connection Profile object, this configuration is ignored.
  * If :kconfig:option:`CONFIG_LWM2M_CARRIER_CUSTOM_URI` is not set (normal), this configuration provides a fallback APN.
    This might be required in your application, depending on the requirements from the carrier.
  * If :kconfig:option:`CONFIG_LWM2M_CARRIER_CUSTOM_URI` is set (generic), :kconfig:option:`CONFIG_LWM2M_CARRIER_CUSTOM_APN` is used instead of the default APN.
    The default APN becomes the fallback APN.

* :kconfig:option:`CONFIG_LWM2M_CARRIER_PDN_TYPE`:

  * This configuration selects the PDN type of the custom APN (:kconfig:option:`CONFIG_LWM2M_CARRIER_CUSTOM_APN`).
  * The default value is ``IPV4V6``.
  * If :kconfig:option:`CONFIG_LWM2M_CARRIER_CUSTOM_APN` is not set, this configuration is ignored.

* :kconfig:option:`CONFIG_LWM2M_CARRIER_GENERIC`, :kconfig:option:`CONFIG_LWM2M_CARRIER_VERIZON`, :kconfig:option:`CONFIG_LWM2M_CARRIER_ATT`, :kconfig:option:`CONFIG_LWM2M_CARRIER_LG_UPLUS`, :kconfig:option:`CONFIG_LWM2M_CARRIER_T_MOBILE`, :kconfig:option:`CONFIG_LWM2M_CARRIER_SOFTBANK`:

  * These configurations allow you to choose the networks in which the carrier library will apply.
  * For example, if you are deploying a product in several networks but only need to enable the carrier library within Verizon, you must set :kconfig:option:`CONFIG_LWM2M_CARRIER_VERIZON` to ``y`` and all the others to ``n``.
  * If only one carrier is selected, then the :kconfig:option:`CONFIG_LWM2M_CARRIER_CUSTOM_URI` and :kconfig:option:`CONFIG_LWM2M_CARRIER_SERVER_SEC_TAG` will be used for this carrier.

    * This will typically have to be done while you are certifying your product, to be able to connect to the carriers certification servers, since they will require a URI different from the default live servers.
    * See :ref:`lwm2m_carrier_provisioning` for more information on the test configuration.

  * If multiple operator networks are selected then the :kconfig:option:`CONFIG_LWM2M_CARRIER_GENERIC` and :kconfig:option:`CONFIG_LWM2M_CARRIER_SERVER_SEC_TAG` will be used for Generic mode, which is used when not in any of the other selected networks.

* :kconfig:option:`CONFIG_LWM2M_CARRIER_LG_UPLUS`, :kconfig:option:`CONFIG_LWM2M_CARRIER_LG_UPLUS_SERVICE_CODE`, :kconfig:option:`CONFIG_LWM2M_CARRIER_LG_UPLUS_DEVICE_SERIAL_NUMBER`:

  * The :kconfig:option:`CONFIG_LWM2M_CARRIER_LG_UPLUS_SERVICE_CODE` Kconfig option sets the LG U+ service code, which is needed to identify your device in the LG U+ device management.
  * The :kconfig:option:`CONFIG_LWM2M_CARRIER_LG_UPLUS_DEVICE_SERIAL_NUMBER` configuration lets you choose between using the nRF9160 SoC 2DID Serial Number, or the device IMEI as a serial number when connecting to the LG U+ device management server.

  .. note::
     Application DFU is needed to enable LG U+ functionality.

.. _server_options_lwm2m:

Server options
==============

Following are some of the server Kconfig options that you can configure:

The server settings can put the LwM2M carrier library either in the normal mode where it connects to the applicable carriers, or in the generic mode where it can connect to any server.

* :kconfig:option:`CONFIG_LWM2M_CARRIER_IS_BOOTSTRAP_SERVER`:

  * This configuration specifies if the :kconfig:option:`CONFIG_LWM2M_CARRIER_CUSTOM_URI` is an LwM2M Bootstrap Server.

* :kconfig:option:`CONFIG_LWM2M_CARRIER_CUSTOM_URI`:

  * This configuration lets the LwM2M carrier library connect to a custom server other than the normal carrier server and enables the generic mode if used in an operator network that is not supported.
  * You must set this option during self-testing, or if the end product is not to be certified with the applicable carriers.
    For more information, see :ref:`lwm2m_certification`.

* :kconfig:option:`CONFIG_LWM2M_CARRIER_SERVER_SEC_TAG`:

  * This configuration provides the library with a security tag containing a PSK.
  * This configuration should normally be left empty (``0``) unless stated by the operator, or when connecting to a custom URI.
    In this case, the library will automatically apply the correct PSK for the different carrier device management.
  * The :ref:`sample <lwm2m_carrier>` allows you to set a PSK that is written to a modem security tag using the :ref:`CONFIG_CARRIER_APP_PSK <CONFIG_CARRIER_APP_PSK>` and :kconfig:option:`CONFIG_LWM2M_CARRIER_SERVER_SEC_TAG` Kconfig options.
    This is convenient for developing and debugging but must be avoided in the final product.
    Instead, see :ref:`modem_key_mgmt` or :ref:`at_client_sample` sample for `provisioning a PSK <Managing credentials_>`_.

* :kconfig:option:`CONFIG_LWM2M_CARRIER_SERVER_LIFETIME`:

  * This configuration specifies the lifetime of the custom LwM2M server.
  * This configuration is ignored if :kconfig:option:`CONFIG_LWM2M_CARRIER_IS_BOOTSTRAP_SERVER` is set.

* :kconfig:option:`CONFIG_LWM2M_SERVER_BINDING_CHOICE`:

  * The binding can be either ``U`` (UDP) or ``N`` (Non-IP).
  * Leaving this configuration empty selects the default binding (UDP).

.. _device_options_lwm2m:

Device options
==============

These values are reported in the Device Object and are not expected to change during run time.
These configurations can be left empty unless otherwise stated by your operator.
The library will automatically set the values according to the detected operator.

Following are the device Kconfig options:

* :kconfig:option:`CONFIG_LWM2M_CARRIER_DEVICE_MANUFACTURER`
* :kconfig:option:`CONFIG_LWM2M_CARRIER_DEVICE_MODEL_NUMBER`
* :kconfig:option:`CONFIG_LWM2M_CARRIER_DEVICE_TYPE`
* :kconfig:option:`CONFIG_LWM2M_CARRIER_DEVICE_HARDWARE_VERSION`
* :kconfig:option:`CONFIG_LWM2M_CARRIER_DEVICE_SOFTWARE_VERSION`

.. _lwm2m_events:

LwM2M carrier library events
****************************

The :c:func:`lwm2m_carrier_event_handler` function may be implemented by your application.
This is shown in the :ref:`lwm2m_carrier` sample.
A ``__weak`` implementation is included in :file:`nrf/lib/bin/lwm2m_carrier/os/lwm2m_carrier.c`.

Following are the various LwM2M carrier library events that are also listed in :file:`nrf/lib/bin/lwm2m_carrier/include/lwm2m_carrier.h`.

* :c:macro:`LWM2M_CARRIER_EVENT_LTE_LINK_DOWN`:

  * This event indicates that the device must disconnect from the LTE network.
  * It occurs during the bootstrapping process and FOTA.
    It can also be triggered when the application calls :c:func:`lwm2m_carrier_request`.

* :c:macro:`LWM2M_CARRIER_EVENT_LTE_LINK_UP`:

  * This event indicates that the device must connect to the LTE network.
  * It occurs during the bootstrapping process and FOTA.
    It can also be triggered when the application calls :c:func:`lwm2m_carrier_request`.

* :c:macro:`LWM2M_CARRIER_EVENT_BOOTSTRAPPED`:

  * This event indicates that the bootstrap sequence is complete, and that the device is ready to be registered.
  * This event is typically seen during the first boot-up.

* :c:macro:`LWM2M_CARRIER_EVENT_REGISTERED`:

  * This event indicates that the device has registered successfully to the carrier's device management servers.

* :c:macro:`LWM2M_CARRIER_EVENT_DEFERRED`:

  * This event indicates that the connection to the device management server has failed.
  * The :c:macro:`LWM2M_CARRIER_EVENT_DEFERRED` event appears instead of the :c:macro:`LWM2M_CARRIER_EVENT_REGISTERED` event.
  * The :c:member:`timeout` parameter supplied with this event determines when the LwM2M carrier library will retry the connection.
  * Following are the various deferred reasons:

    * :c:macro:`LWM2M_CARRIER_DEFERRED_NO_REASON` - The application need not take any special action.
      If :c:member:`timeout` is 24 hours, the application can proceed with other activities until the retry takes place.
    * :c:macro:`LWM2M_CARRIER_DEFERRED_PDN_ACTIVATE` - This event indicates problem with the SIM card, or temporary network problems.
      If this persists, contact your carrier.
    * :c:macro:`LWM2M_CARRIER_DEFERRED_BOOTSTRAP_CONNECT` - The DTLS handshake with the bootstrap server has failed.
      If the application is using a custom PSK, verify that the format is correct.
    * :c:macro:`LWM2M_CARRIER_DEFERRED_BOOTSTRAP_SEQUENCE` - The bootstrap sequence is incomplete.
      The server failed either to acknowledge the request by the library, or to send objects to the library. Confirm that the carrier is aware of the IMEI.
    * :c:macro:`LWM2M_CARRIER_DEFERRED_SERVER_NO_ROUTE`, :c:macro:`LWM2M_CARRIER_DEFERRED_BOOTSTRAP_NO_ROUTE` - There is a routing problem in the carrier network.
      If this event persists, contact the carrier.
    * :c:macro:`LWM2M_CARRIER_DEFERRED_SERVER_CONNECT` - This event indicates that the DTLS handshake with the server has failed.
      This typically happens if the bootstrap sequence has failed on the carrier side.
    * :c:macro:`LWM2M_CARRIER_DEFERRED_SERVER_REGISTRATION` - The server registration has not completed, and the server does not recognize the connecting device.
      If this event persists, contact the carrier.
    * :c:macro:`LWM2M_CARRIER_DEFERRED_SERVICE_UNAVAILABLE` - The server is unavailable due to maintenance.
    * :c:macro:`LWM2M_CARRIER_DEFERRED_SIM_MSISDN` - The device is waiting for the SIM MSISDN to be available to read.
* :c:macro:`LWM2M_CARRIER_EVENT_FOTA_START`:

  * This event indicates that the modem update has started.
  * The application must immediately terminate any open TLS and DTLS sessions.
  * See :ref:`req_appln_limitations`.
* :c:macro:`LWM2M_CARRIER_EVENT_FOTA_SUCCESS`

  * This event indicates that the FOTA procedure is successful.
* :c:macro:`LWM2M_CARRIER_EVENT_REBOOT`:

  * This event indicates that the LwM2M carrier library will reboot the device.
  * If the application is not ready to reboot, it must return non-zero and then reboot at the earliest convenient time.
* :c:macro:`LWM2M_CARRIER_EVENT_MODEM_INIT`:

  * This event indicates that the application must initialize the modem for the LwM2M carrier library to proceed.
  * This event is indicated during FOTA procedures to reinitialize the :ref:`nrf_modem_lib_readme`.
* :c:macro:`LWM2M_CARRIER_EVENT_MODEM_SHUTDOWN`:

  * This event indicates that the application must shut down the modem for the LwM2M carrier library to proceed.
  * This event is indicated during FOTA procedures to reinitialize the :ref:`nrf_modem_lib_readme`.
* :c:macro:`LWM2M_CARRIER_EVENT_ERROR`:

  * This event indicates an error.
  * The event data struct :c:type:`lwm2m_carrier_event_error_t` contains the information about the error (:c:member:`type` and :c:member:`value`).
  * Following are the valid error types:

    * :c:macro:`LWM2M_CARRIER_ERROR_LTE_LINK_UP_FAIL` - This error is generated if the request to connect to the LTE network has failed.
      It indicates possible problems with the SIM card, or insufficient network coverage. See :c:member:`value` field of the event.
    * :c:macro:`LWM2M_CARRIER_ERROR_LTE_LINK_DOWN_FAIL` - This error is generated if the request to disconnect from the LTE network has failed.
    * :c:macro:`LWM2M_CARRIER_ERROR_BOOTSTRAP` - This error is generated during the bootstrap procedure.

      +--------------------------------------------------------+--------------------------------------------------------------------------------------+--------------------------------------------------+
      | Errors                                                 | More information                                                                     | Recovery                                         |
      |                                                        |                                                                                      |                                                  |
      +========================================================+======================================================================================+==================================================+
      | Retry limit for connecting to the bootstrap            | Common reason for this failure can be incorrect URI or PSK,                          | Library retries after next device reboot.        |
      | server has been reached (``-ETIMEDOUT``).              | or the server is unavailable (for example, temporary network issues).                |                                                  |
      |                                                        | If this error persists, contact your carrier.                                        |                                                  |
      +--------------------------------------------------------+--------------------------------------------------------------------------------------+--------------------------------------------------+
      | Failure to provision the PSK                           | The LTE link was up while the modem attempted to write keys to the modem.            | Library retries after 24 hours.                  |
      | needed for the bootstrap procedure (``-EACCES``).      | Verify that the application prioritizes the ``LWM2M_CARRIER_EVENT_LTE_LINK_UP``      |                                                  |
      |                                                        | and ``LWM2M_CARRIER_EVENT_LTE_LINK_DOWN`` events.                                    |                                                  |
      +--------------------------------------------------------+--------------------------------------------------------------------------------------+--------------------------------------------------+
      | Failure to read MSISDN or ICCID values (``-EFAULT``).  | ICCID is fetched from SIM, while MSISDN is received from the network for             | Library retries upon next network connection.    |
      |                                                        | some carriers. If it has not been issued yet, the bootstrap process cannot proceed.  |                                                  |
      +--------------------------------------------------------+--------------------------------------------------------------------------------------+--------------------------------------------------+

    * :c:macro:`LWM2M_CARRIER_ERROR_FOTA_FAIL` - This error indicates a failure to update the device.
      If this error persists, create a ticket in `DevZone`_ with the modem trace.
      The following error values may apply:

      * ``-EPERM`` - No valid security tag found.
        The security tag contains the certificate needed to secure the connection to the repository server.
        Check with your operator which certificates are needed for the firmware update, and make sure that you have provisioned these to the device.
      * ``-ENOMEM`` - Too many open connections to connect to the firmware repository.
        Pay attention to :c:macro:`LWM2M_CARRIER_EVENT_FOTA_START`, which prompts you to close any TLS socket.
      * ``-EBADF`` - Incorrect firmware update version.
        The Firmware could not be applied to the device.
        Check that you are providing the correct FOTA image and that it is compatible with the current firmware.

    * :c:macro:`LWM2M_CARRIER_ERROR_CONFIGURATION` - This error indicates that an illegal object configuration has been detected.
    * :c:macro:`LWM2M_CARRIER_ERROR_INIT` - This error indicates that the LwM2M carrier library has failed to initialize.
    * :c:macro:`LWM2M_CARRIER_ERROR_RUN` - This error indicates that the library configuration is invalid.
      Ensure that the :c:struct:`lwm2m_carrier_config_t` structure is configured correctly.

.. _lwm2m_carrier_shell:

LwM2M carrier shell configuration
*********************************

The LwM2M carrier shell allows you to interact with the carrier library through the shell command line.
This allows you to overwrite initialization parameters and call the different runtime APIs of the library.
This can be useful for getting started and debugging.
See :ref:`zephyr:shell_api` for more information.

To enable and configure the LwM2M carrier shell, set the :kconfig:option:`CONFIG_LWM2M_CARRIER_SHELL` Kconfig option to ``y``.
The :kconfig:option:`CONFIG_LWM2M_CARRIER_SHELL` Kconfig option has the following dependencies:

* :kconfig:option:`CONFIG_FLASH_MAP`
* :kconfig:option:`CONFIG_SHELL`
* :kconfig:option:`CONFIG_SETTINGS`

In the :ref:`lwm2m_carrier` sample, you can enable the LwM2M carrier shell by :ref:`building with the overlay file <lwm2m_carrier_shell_overlay>` :file:`overlay-shell.conf`.

.. figure:: /libraries/bin/lwm2m_carrier/images/lwm2m_carrier_os_abstraction_shell.svg
    :alt: LwM2M carrier shell

    LwM2M carrier shell

carrier_config
==============

The initialization parameter :c:type:`lwm2m_carrier_config_t` can be overwritten with custom settings through the LwM2M carrier shell command group ``carrier_config``.
Use the ``print`` command to display the configurations that are written with ``carrier_config``:

.. code-block:: console

    > carrier_config print
    Automatic startup                No

    Custom carrier settings          Yes
      Carriers enabled               All
      Bootstrap from smartcard       Yes
      Session idle timeout           0
      CoAP confirmable interval      0
      APN
      PDN type                       IPv4v6
      Service code
      Device Serial Number type      0

    Custom carrier server settings   No
      Is bootstrap server            No  (Not used without server URI)
      Server URI
      PSK security tag               0
      Server lifetime                0
      Server binding                 U

    Custom carrier device settings   No
      Manufacturer
      Model number
      Device type
      Hardware version
      Software version

To allow time to change configurations before the library applies them, the application waits in the initialization phase (:c:func:`lwm2m_carrier_custom_init`) until ``auto_startup`` is set.

.. code-block::

   uart:~$ carrier_config auto_startup y
   Set auto startup: Yes

The settings are applied by the function :c:func:`lwm2m_carrier_custom_init`.

This function is implemented in :file:`nrf/lib/bin/lwm2m_carrier/os/lwm2m_settings.c` that is included in the project when you enable the LwM2M carrier shell.
The library thread calls the :c:func:`lwm2m_carrier_custom_init` function before calling the :c:func:`lwm2m_carrier_main` function.

carrier_api
===========

The LwM2M carrier shell command group ``carrier_api`` allows you to access the public LwM2M API as shown in :file:`nrf/lib/bin/lwm2m_carrier/include/lwm2m_carrier.h`.

For example, to indicate the battery level of the device to the carrier, the function :c:func:`lwm2m_carrier_battery_level_set` is used.
This can also be done through the ``carrier_api`` command:

.. code-block::

   > carrier_api device battery_level 20
   Battery level updated successfully


help
====

To display help for all available shell commands, pass the following command to shell:

.. parsed-literal::
   :class: highlight

   > [*group*] help

If the optional argument is not provided, the command displays help for all command groups.

If the optional argument is provided, it displays help for subcommands of the specified command group.
For example, ``carrier_config help`` displays help for all ``carrier_config`` commands.

Objects
*******

The objects enabled depend on the carrier network.
When connecting to a generic LwM2M server, the following objects are enabled:

* Security
* Server
* Access Control
* Device
* Connectivity Monitoring
* Firmware Update
* Location
* Connectivity Statistics
* Cellular Connectivity
* APN Connection Profile
* Binary App Data Container
* Event Log

Resources
=========

The following values that reflect the state of the device must be kept up to date by the application:

* Available Power Sources - Defaults to ``0`` if not set (DC Power).
* Power Source Voltage - Defaults to ``0`` if not set.
* Power Source Current - Defaults to ``0`` if not set.
* Battery Level - Defaults to ``0`` if not set.
* Battery Status - Defaults to ``5`` if not set (Not Installed).
* Memory Total - Defaults to ``0`` if not set.
* Error Code - Defaults to ``0`` if not set (No Error).
* Device Type - Defaults to ``Smart Device`` if not set.
* Software Version - Defaults to ``LwM2M <libversion>``.
  For example, ``LwM2M carrier 3.2.0`` for release 3.2.0.
* Hardware Version - Default value is read from the modem.
  An example value is ``nRF9160 SICA B0A``.
* Location - Defaults to ``0`` if not set.

The following values are read from the modem by default but can be overwritten:

* Manufacturer
* Model Number
* UTC Offset
* Time zone
* Current Time

For example, the carrier device management platform can observe the battery level of your device.
The application uses the :c:func:`lwm2m_carrier_battery_level_set` function to indicate the current battery level of the device to the carrier.
