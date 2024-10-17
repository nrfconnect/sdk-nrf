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
For more information, see the :file:`lwm2m_os.h` file or the :ref:`liblwm2m_os` section for available APIs.

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
  * :ref:`lib_uicc_lwm2m`

  The inclusion of the :ref:`lib_uicc_lwm2m` library is optional and is added using the :kconfig:option:`CONFIG_UICC_LWM2M` Kconfig option.
  This module allows the LwM2M carrier library to use the bootstrap information stored on the SIM card.
  If present, the configuration in the SIM will take precedence over any other configuration.
  For example, if a bootstrap server URI is fetched from the SIM, the configuration set by the :kconfig:option:`CONFIG_LWM2M_CARRIER_CUSTOM_URI` Kconfig option is ignored.

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

* :kconfig:option:`CONFIG_LWM2M_CARRIER_SESSION_IDLE_TIMEOUT`:

  * This configuration specifies the session idle timeout (inactivity).
    Upon timeout, the LwM2M carrier library disconnects from one or more device management servers.
  * The timeout closes the DTLS session.
    A new DTLS session will be created on the next activity (for example, lifetime trigger).
  * Leaving this configuration empty (``0``) sets it to a default of 60 seconds.
  * Setting this configuration to ``-1`` disables the session idle timeout.
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

.. _general_options_enabled_carriers:

* :kconfig:option:`CONFIG_LWM2M_CARRIER_GENERIC`, :kconfig:option:`CONFIG_LWM2M_CARRIER_VERIZON`, :kconfig:option:`CONFIG_LWM2M_CARRIER_BELL_CA`, :kconfig:option:`CONFIG_LWM2M_CARRIER_LG_UPLUS`, :kconfig:option:`CONFIG_LWM2M_CARRIER_T_MOBILE`, :kconfig:option:`CONFIG_LWM2M_CARRIER_SOFTBANK`:

  * These configurations allow you to choose the networks in which the carrier library will apply.
  * For example, if you are deploying a product in several networks but only need to enable the carrier library within Verizon, you must set :kconfig:option:`CONFIG_LWM2M_CARRIER_VERIZON` to ``y`` and all the others to ``n``.
  * If only one carrier is selected, then the configurations listed in :ref:`server_options_lwm2m` are applied to the selected carrier.

    * This will typically have to be done while you are certifying your product, to be able to connect to the carriers certification servers, since they can require different settings than the default live servers.
    * See :ref:`lwm2m_carrier_provisioning` for more information on the test configuration.

  * If you select the :kconfig:option:`CONFIG_LWM2M_CARRIER_GENERIC` Kconfig option, all the server settings mentioned in :ref:`server_options_lwm2m` apply when outside of the other enabled carrier network.
  * If multiple carriers are enabled, but the carrier defined by  :kconfig:option:`CONFIG_LWM2M_CARRIER_GENERIC` is not one of them, then all server settings are ignored.

* :kconfig:option:`CONFIG_LWM2M_CARRIER_LG_UPLUS`, :kconfig:option:`CONFIG_LWM2M_CARRIER_LG_UPLUS_SERVICE_CODE`, :kconfig:option:`CONFIG_LWM2M_CARRIER_LG_UPLUS_DEVICE_SERIAL_NUMBER`:

  * The :kconfig:option:`CONFIG_LWM2M_CARRIER_LG_UPLUS_SERVICE_CODE` Kconfig option sets the LG U+ service code, which is needed to identify your device in the LG U+ device management.
  * The :kconfig:option:`CONFIG_LWM2M_CARRIER_LG_UPLUS_DEVICE_SERIAL_NUMBER` configuration lets you choose between using the nRF91 Series SiP 2DID Serial Number, or the device IMEI as a serial number when connecting to the LG U+ device management server.

  .. note::
     Application DFU is needed to enable LG U+ functionality.

* :kconfig:option:`CONFIG_LWM2M_CARRIER_FIRMWARE_DOWNLOAD_TIMEOUT`:

  * This configuration specifies the time (in minutes) allowed for a single firmware image download.
  * If the download is not completed by the time the specified number of minutes elapses, the download shall be aborted.
  * This configuration is only supported for Push delivery method of firmware images.
  * Leaving this configuration empty (``0``) disables the timer for unknown subscriber IDs, and set it to 30 minutes for the SoftBank subscriber ID.

* :kconfig:option:`CONFIG_LWM2M_CARRIER_QUEUE_MODE`:

  * This configuration specifies whether the LwM2M device is to inform the LwM2M Server that it may be disconnected for an extended period of time.

* :kconfig:option:`CONFIG_LWM2M_CARRIER_AUTO_REGISTER`:

  * This configuration specifies if the LwM2M carrier library will register with the LwM2M server automatically once connected.
  * Auto register is disabled for the SoftBank Subscriber ID, and enabled for other Subscriber IDs.
  * Auto register can be disabled if the library is operating in Generic mode (connecting to a custom URI instead of the predetermined carrier servers).

.. _server_options_lwm2m:

Server options
==============

Following are some of the server Kconfig options that you can configure.
See the :ref:`enabled carriers <general_options_enabled_carriers>` under :ref:`general_options_lwm2m` for when the option is relevant.

For :kconfig:option:`CONFIG_LWM2M_CARRIER_GENERIC`, no valid factory configuration has been set.
At a minimum, a URI must be set, unless the :kconfig:option:`CONFIG_LWM2M_SERVER_BINDING_CHOICE` Kconfig option value is non-IP.

.. note::
   Changing one or more server options will trigger a factory reset (resulting in a new bootstrap sequence).

* :kconfig:option:`CONFIG_LWM2M_CARRIER_CUSTOM_URI`:

  * This configuration lets the LwM2M carrier library connect to a custom server other than the normal carrier server and enables the generic mode if used in an operator network that is not supported.
  * You must set this option during self-testing.
    For more information, see :ref:`lwm2m_certification`.

* :kconfig:option:`CONFIG_LWM2M_CARRIER_IS_BOOTSTRAP_SERVER`:

  * This configuration is ignored if :kconfig:option:`CONFIG_LWM2M_CARRIER_CUSTOM_URI` is not set.
  * This configuration specifies if the server defined by :kconfig:option:`CONFIG_LWM2M_CARRIER_CUSTOM_URI` is an LwM2M bootstrap server.

* :kconfig:option:`CONFIG_LWM2M_CARRIER_SERVER_SEC_TAG`:

  * This configuration provides the library with a security tag.
    The security tag must contain a PSK, and can additionally contain a PSK identity.
  * This configuration should normally be left empty (``0``) unless stated by the operator, or when connecting to a custom URI.
    If left empty, the library will automatically apply the correct PSK for the different carrier device management.
  * The :ref:`sample <lwm2m_carrier>` allows you to set a PSK that is written to a modem security tag using the :ref:`CONFIG_CARRIER_APP_PSK <CONFIG_CARRIER_APP_PSK>` and :kconfig:option:`CONFIG_LWM2M_CARRIER_SERVER_SEC_TAG` Kconfig options.
    This is convenient for developing and debugging but must be avoided in the final product.
    Instead, see :ref:`modem_key_mgmt` or :ref:`at_client_sample` sample for `provisioning a PSK <Managing credentials_>`_.

* :kconfig:option:`CONFIG_LWM2M_CARRIER_SERVER_LIFETIME`:

  * This configuration specifies the lifetime of the LwM2M Server.
  * This configuration is ignored if a bootstrap server is configured (either by our factory configuration, or by :kconfig:option:`CONFIG_LWM2M_CARRIER_IS_BOOTSTRAP_SERVER`).
  * If this configuration is left empty (``0``) the factory configuration is used.
    This can be different for each supported carrier.
    For generic operation (:kconfig:option:`CONFIG_LWM2M_CARRIER_GENERIC`), the default is 1 hour.

* :kconfig:option:`CONFIG_LWM2M_SERVER_BINDING_CHOICE`:

  * This configuration can be used to overwrite the factory default by selecting :c:macro:`LWM2M_CARRIER_SERVER_BINDING_UDP` or :c:macro:`LWM2M_CARRIER_SERVER_BINDING_NONIP`).
  * This configuration is ignored if a bootstrap server is configured (either by our factory configuration, or by :kconfig:option:`CONFIG_LWM2M_CARRIER_IS_BOOTSTRAP_SERVER`).
  * If UDP binding is configured, a URI must also be set (:kconfig:option:`CONFIG_LWM2M_CARRIER_CUSTOM_URI`).
  * The APN (either network default, or the one set with :kconfig:option:`CONFIG_LWM2M_CARRIER_CUSTOM_APN`) must be UDP (IP) or non-IP respectively.
  * If this configuration is left empty (``0``) the factory configuration is used.
    This can be different for each supported carrier.
    For generic operation (:kconfig:option:`CONFIG_LWM2M_CARRIER_GENERIC`), the default is :c:macro:`LWM2M_CARRIER_SERVER_BINDING_UDP`.

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

The :c:func:`lwm2m_carrier_event_handler` function can be implemented by your application.
This is shown in the :ref:`lwm2m_carrier` sample.
A ``__weak`` implementation is included in :file:`nrf/lib/bin/lwm2m_carrier/os/lwm2m_carrier.c`.

The ``__weak`` implementation acts as default handling and should be sufficient for most needs, but certain events could need handling specific to your application.

For example:

* :c:macro:`LWM2M_CARRIER_EVENT_REBOOT`- This event indicates that the LwM2M carrier library will reboot the device.
  Your application might want to do some bookkeeping before rebooting.

* :c:macro:`LWM2M_CARRIER_EVENT_LTE_LINK_DOWN`- This event indicates that the device must disconnect from the LTE network.
  Your application might want to do some bookkeeping before disconnecting.

* :c:macro:`LWM2M_CARRIER_EVENT_ERROR_CODE_RESET`- This event indicates that the server has reset the "Error Code" resource (3/0/11) of the LwM2M carrier library.
  When received, the application should re-evaluate the errors and signal relevant errors again using the :c:func:`lwm2m_carrier_error_code_add` function.
  This is not handled in the ``__weak`` implementation since the applicable errors to report is application dependent.

See the :ref:`liblwm2m_carrier_events` section of the API documentation for a complete overview over the events, which are defined in the :file:`nrf/lib/bin/lwm2m_carrier/include/lwm2m_carrier.h` file.

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

    uart:~$ carrier_config print
    Automatic startup                No

    Custom carrier settings          Yes
      Carriers enabled               Verizon (1), T-Mobile (3), SoftBank (4), Bell Canada (5)
      Server settings
        Server URI
          Is bootstrap server        No  (Not used without server URI)
        PSK security tag             0
        Server lifetime              0
        Server binding               Not set
      Auto register                  Yes
      Bootstrap from smartcard       Yes
      Queue mode                     Yes
      Session idle timeout           60
      CoAP confirmable interval      86400
      APN
        PDN type                     IPv4v6
      Service code
      Device Serial Number type      1
      Firmware download timeout      0 (disabled)

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
When connecting to a generic LwM2M Server, the following objects are enabled:

+-----+---------------------------+
| ID  | Name                      |
+=====+===========================+
| 0   | Security                  |
+-----+---------------------------+
| 1   | Server                    |
+-----+---------------------------+
| 2   | Access Control            |
+-----+---------------------------+
| 3   | Device                    |
+-----+---------------------------+
| 4   | Connectivity Monitoring   |
+-----+---------------------------+
| 5   | Firmware Update           |
+-----+---------------------------+
| 6   | Location                  |
+-----+---------------------------+
| 7   | Connectivity Statistics   |
+-----+---------------------------+
| 10  | Cellular Connectivity     |
+-----+---------------------------+
| 11  | APN Connection Profile    |
+-----+---------------------------+
| 19  | Binary App Data Container |
+-----+---------------------------+
| 20  | Event Log                 |
+-----+---------------------------+

Resources
=========

Reading and writing to the resources of the prior listed objects are the LwM2M Server's interface to your device.
Many of these resources have no application API, and the reporting is automatically handled by the LwM2M carrier library.
An example is the Network Bearer or Radio Signal Strength resources in the Connectivity Monitoring object are simply read from the modem directly by the library.

Every resource, which is *not* automatically managed by the library, can be set using the resource APIs.
See the :ref:`liblwm2m_carrier_objects` section for more details on the resource APIs, which are defined in the :file:`nrf/lib/bin/lwm2m_carrier/include/lwm2m_carrier.h` file.

For example, your operator's LwM2M Server might observe the battery level of your device.
The battery level is not automatically known to the LwM2M carrier library, (or if there is no battery at all).
So it provides the default (0%) to the server.
The application can use the :c:func:`lwm2m_carrier_avail_power_sources_set` function to select :c:macro:`LWM2M_CARRIER_POWER_SOURCE_INTERNAL_BATTERY` as the power source, and then call the :c:func:`lwm2m_carrier_battery_level_set` function to indicate the current battery level of the device to the operator's server.

  .. note::
     Some Device Resources are not expected to change during runtime and are set at start-up using the :c:struct:`lwm2m_carrier_config_t` struct
     See :ref:`device_options_lwm2m` for the corresponding Kconfig options for the following events:

     * :c:member:`lwm2m_carrier_config_t.manufacturer`
     * :c:member:`lwm2m_carrier_config_t.model_number`
     * :c:member:`lwm2m_carrier_config_t.device_type`
     * :c:member:`lwm2m_carrier_config_t.hardware_version`
     * :c:member:`lwm2m_carrier_config_t.software_version`

DFU interface
*************

The LwM2M carrier library makes use of the :ref:`lib_dfu_target` library to manage the DFU process, providing a single interface to support different types of firmware upgrades.
Currently, the following types of firmware upgrades are supported:

* MCUboot-style upgrades (:c:macro:`LWM2M_OS_DFU_IMG_TYPE_APPLICATION`)
* Modem delta upgrades (:c:macro:`LWM2M_OS_DFU_IMG_TYPE_MODEM_DELTA`)
* Proprietary application upgrades over multiple files (:c:macro:`LWM2M_OS_DFU_IMG_TYPE_APPLICATION_FILE`)

The type of upgrade is determined when the library calls the :c:func:`lwm2m_os_dfu_img_type` function in the abstraction layer upon receiving a new firmware image.

If MCUboot-style upgrades are enabled, the LwM2M carrier library uses the function :c:func:`lwm2m_os_dfu_application_update_validate` to validate the application image update.
A ``__weak`` implementation of the function is included, which checks if the currently running image is not yet confirmed as valid (which is the case after an upgrade) and marks it appropriately.

The proprietary application upgrades over multiple files are currently only supported if the :kconfig:option:`CONFIG_LWM2M_CARRIER_SOFTBANK_DIVIDED_FOTA` Kconfig option is enabled.
This allows the library to perform the non-standard divided FOTA procedure in the SoftBank network.
The application update files required for this type of firmware upgrade can be generated during the building process by enabling the ``SB_CONFIG_LWM2M_CARRIER_DIVIDED_DFU`` sysbuild Kconfig option.
