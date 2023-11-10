.. _liblwm2m_carrier_changelog:

Changelog
#########

.. contents::
   :local:
   :depth: 2

All notable changes to this project are documented in this file.

liblwm2m_carrier 3.3.3
**********************

Release for modem firmware version 1.3.5 and 2.0.0.

Certification status
====================

For certification status, see `Mobile network operator certifications`_.

Changes
=======

* Minor fixes and improvements.

liblwm2m_carrier 3.3.2
**********************

Release for modem firmware version 1.3.5 and 2.0.0.

Certification status
====================

For certification status, see `Mobile network operator certifications`_.

Changes
=======

* Fixed an issue where failed modem firmware updates would not be reported correctly to the user application and the LwM2M server.


liblwm2m_carrier 3.3.1
**********************

Release for modem firmware version 1.3.5 and 2.0.0.

Certification status
====================

For certification status, see `Mobile network operator certifications`_.

Changes
=======

* Minor fixes and improvements.

liblwm2m_carrier 3.3.0
**********************

Release for modem firmware version 1.3.5 and 2.0.0.

Certification status
====================

For certification status, see `Mobile network operator certifications`_.

Size
====

See :ref:`lwm2m_lib_size` for an explanation of the library size in different scenarios.

+-------------------------+---------------+------------+
|                         | Flash (Bytes) | RAM (Bytes)|
+-------------------------+---------------+------------+
| Library size            | 78059         | 17288      |
| (binary)                |               |            |
+-------------------------+---------------+------------+
| Library size            | 98024         | 40416      |
| (reference application) |               |            |
+-------------------------+---------------+------------+

Changes
=======

* Added the Binary App Data Container object. The object is enabled for the SoftBank subscriber ID :kconfig:option:`CONFIG_LWM2M_CARRIER_SOFTBANK`, or if the LwM2M carrier library is operating in generic mode :kconfig:option:`CONFIG_LWM2M_CARRIER_GENERIC`.

* Added the Event Log object. The object is enabled for the SoftBank subscriber ID :kconfig:option:`CONFIG_LWM2M_CARRIER_SOFTBANK`, or if the LwM2M carrier library is operating in generic mode :kconfig:option:`CONFIG_LWM2M_CARRIER_GENERIC`.

* FOTA procedure now only requires a single modem library initialization.
  For more information see the modem library :ref:`nrf_modem_changelog`.

  * Old successful event flow: :c:member:`LWM2M_CARRIER_EVENT_FOTA_START` > :c:member:`LWM2M_CARRIER_EVENT_LTE_POWER_OFF` > :c:member:`LWM2M_CARRIER_EVENT_MODEM_SHUTDOWN` > :c:member:`LWM2M_CARRIER_EVENT_MODEM_INIT` > :c:member:`LWM2M_CARRIER_EVENT_MODEM_INIT` > :c:member:`LWM2M_CARRIER_EVENT_FOTA_SUCCESS` > :c:member:`LWM2M_CARRIER_EVENT_LTE_LINK_UP`.
  * New successful event flow: :c:member:`LWM2M_CARRIER_EVENT_FOTA_START` > :c:member:`LWM2M_CARRIER_EVENT_LTE_POWER_OFF` > :c:member:`LWM2M_CARRIER_EVENT_MODEM_SHUTDOWN` > :c:member:`LWM2M_CARRIER_EVENT_MODEM_INIT` > :c:member:`LWM2M_CARRIER_EVENT_FOTA_SUCCESS` > :c:member:`LWM2M_CARRIER_EVENT_LTE_LINK_UP`.

AT&T support has been deprecated
--------------------------------

The application must no longer connect to the AT&T Device Management server.
Consequently, the LwM2M carrier library is no longer required to certify with AT&T.

* Automatic connection and handling of custom AT&T logic by the LwM2M carrier library is deprecated.
* :kconfig:option:`CONFIG_LWM2M_CARRIER_ATT` is now to be disabled by default, and might be removed in a future release.
* :kconfig:option:`CONFIG_LWM2M_CARRIER_ATT` must be explicitly enabled to apply the library's AT&T configurations.

    * In previous releases, AT&T support could be enabled together with all other applicable carriers by initializing :c:macro:`lwm2m_carrier_config_t` with an empty ``carriers_enabled`` parameter.

liblwm2m_carrier 3.2.0
**********************

Release for modem firmware version 1.3.5.

Certification status
====================

For certification status, see `Mobile network operator certifications`_.

Size
====

See :ref:`lwm2m_lib_size` for an explanation of the library size in different scenarios.

+-------------------------+---------------+------------+
|                         | Flash (Bytes) | RAM (Bytes)|
+-------------------------+---------------+------------+
| Library size            | 75955         | 16575      |
| (binary)                |               |            |
+-------------------------+---------------+------------+
| Library size            | 96292         | 39712      |
| (reference application) |               |            |
+-------------------------+---------------+------------+

Changes
=======

* Added preliminary support for SoftBank subscriber ID.
  This carrier can be disabled or enabled with the Kconfig option :kconfig:option:`CONFIG_LWM2M_CARRIER_SOFTBANK`.
* Removed the error event types ``LWM2M_CARRIER_ERROR_FOTA_PKG``, ``LWM2M_CARRIER_ERROR_FOTA_PROTO``, ``LWM2M_CARRIER_ERROR_FOTA_CONN`` and ``LWM2M_CARRIER_ERROR_FOTA_CONN_LOST``.
  Instead, the :c:member:`LWM2M_CARRIER_ERROR_FOTA_FAIL` error event indicates an error code ``error.value`` in :c:struct:`lwm2m_carrier_event_t` (when :c:member:`LWM2M_CARRIER_ERROR_FOTA_FAIL` is received).
* Removed the dependency on the :ref:`lte_lc_readme` library.

  * This was primarily done to save space in the :ref:`serial_lte_modem` application.
  * All other relevant samples and applications use the :ref:`lte_lc_readme` library.
    It is highly recommended that you include it in your applications.

* Removed the event ``LWM2M_CARRIER_ERROR_INTERNAL``.

* FOTA process has been improved.

  * The FOTA procedure no longer requires the device to reboot.
    It uses callbacks documented in the :ref:`nrf_modem_lib_readme`. The Modem library must reinitialize to perform a modem update.
  * Added the events :c:member:`LWM2M_CARRIER_EVENT_MODEM_SHUTDOWN` and :c:member:`LWM2M_CARRIER_EVENT_MODEM_INIT` to explicitly request the application to perform the reinitialization during FOTA.
  * Added a new event, :c:member:`LWM2M_CARRIER_EVENT_FOTA_SUCCESS` to indicate the completion of FOTA update to the application.
  * Old successful event flow: :c:member:`LWM2M_CARRIER_EVENT_FOTA_START` > :c:member:`LWM2M_CARRIER_EVENT_REBOOT` > :c:member:`LWM2M_CARRIER_EVENT_LTE_POWER_OFF` > :c:member:`LWM2M_CARRIER_EVENT_REGISTERED`.
  * New successful event flow: :c:member:`LWM2M_CARRIER_EVENT_FOTA_START` > :c:member:`LWM2M_CARRIER_EVENT_LTE_POWER_OFF` > :c:member:`LWM2M_CARRIER_EVENT_MODEM_SHUTDOWN` > :c:member:`LWM2M_CARRIER_EVENT_MODEM_INIT` > :c:member:`LWM2M_CARRIER_EVENT_MODEM_INIT` > :c:member:`LWM2M_CARRIER_EVENT_FOTA_SUCCESS` > :c:member:`LWM2M_CARRIER_EVENT_LTE_LINK_UP`.

nRF modem dependency change
---------------------------

LwM2M carrier library no longer explicitly controls the :ref:`nrf_modem`.
Instead, the application can initialize the Modem library at its own convenience.
The LwM2M carrier library will then use the Modem library callbacks to start or pause its own operations.
See the :ref:`mlil_callbacks` section under :ref:`nrf_modem_lib_readme` for more information.

* Removed ``lwm2m_carrier_init()``.

  * The LwM2M carrier library now initializes every time the init callback from the Modem library is called.
  * Renamed ``lwm2m_carrier_run()`` to :c:func:`lwm2m_carrier_main`.

* Removed the event ``LWM2M_CARRIER_EVENT_INIT``.

  * This event was used to indicate that the modem was ready to be used by the application, but this is no longer necessary since the application now controls Modem library.
  * Keep in mind that CA root certificates must still be provisioned while the modem is offline (any time the link is down).
  * :c:func:`lte_lc_init` and :c:func:`lte_lc_register_handler` can be called at any time after :c:func:`nrf_modem_lib_init`.

* Removed ``lwm2m_os_nrf_modem_init()`` and ``lwm2m_os_nrf_modem_shutdown()``.

* The library no longer sends an initial ``LWM2M_CARRIER_EVENT_LTE_LINK_UP`` event after initialization.

  * This event was meant to indicate to the application when it could go online for the first time, but this is no longer needed.
    Instead, the library waits for the link to be brought up by the application.
  * An additional :c:member:`LWM2M_CARRIER_EVENT_LTE_DOWN` event can be triggered by the LwM2M carrier library the first time the application goes online.
    This is for continuing to write the correct bootstrap keys.

liblwm2m_carrier 3.1.0
**********************

Release for modem firmware version 1.3.3 and 1.3.4.

Certification status
====================

For certification status, see `Mobile network operator certifications`_.

Size
====

See :ref:`lwm2m_lib_size` for an explanation of the library size in different scenarios.

+-------------------------+---------------+------------+
|                         | Flash (Bytes) | RAM (Bytes)|
+-------------------------+---------------+------------+
| Library size            | 73482         | 15960      |
| (binary)                |               |            |
+-------------------------+---------------+------------+
| Library size            | 94644         | 40192      |
| (reference application) |               |            |
+-------------------------+---------------+------------+

Changes
=======

* Added shell functionality to the LwM2M carrier library (:file:`lwm2m_shell.c` and :file:`lwm2m_settings.c` or :file:`lwm2m_settings.h`).
  This is intended to provide convenient access to the API for development and debugging.

  * Enabled or disabled by using :kconfig:option:`CONFIG_LWM2M_CARRIER_SHELL` and :kconfig:option:`CONFIG_LWM2M_CARRIER_SETTINGS`.
  * For examples of using the shell, see the :ref:`lwm2m_carrier` sample documentation and the :ref:`lwm2m_carrier_shell` section in the library documentation.

* Added a new ``__weak`` function :c:func:`lwm2m_carrier_custom_init`.

  * This function is run in :file:`lwm2m_carrier.c` before :c:func:`lwm2m_carrier_init`.
  * This function allows Kconfig settings of the LwM2M carrier library to be overwritten without having to make changes in the :file:`lwm2m_carrier.c` file.
  * The :ref:`lwm2m_carrier` sample uses the :c:func:`lwm2m_carrier_custom_init` function to facilitate shell access to the initialization settings, by loading a stored ``lwm2m_carrier_config_t``.

* Fixed a bug where the functions :c:func:`lwm2m_carrier_avail_power_sources_set` and :c:func:`lwm2m_carrier_error_code_add` would not return an error if the device object is uninitialized.

* Removed the following functions from the glue layer:

  * ``lwm2m_os_sec_psk_exists()``
  * ``lwm2m_os_sec_psk_write()``
  * ``lwm2m_os_sec_psk_delete()``
  * ``lwm2m_os_sec_identity_exists()``
  * ``lwm2m_os_sec_identity_write()``
  * ``lwm2m_os_sec_identity_delete()``

* Added the Kconfig options :kconfig:option:`CONFIG_LWM2M_CARRIER_THREAD_STACK_SIZE` and :kconfig:option:`CONFIG_LWM2M_CARRIER_WORKQ_STACK_SIZE`.

  * These options allow you to alter the LwM2M carrier library thread and work queue stack without editing :file:`lwm2m_carrier.c`.

* Moved the configuration parameter :c:macro:`lwm2m_carrier_config_t` from :c:func:`lwm2m_carrier_init` to :c:func:`lwm2m_carrier_run`.

  * Added a new error event type :c:macro:`LWM2M_CARRIER_ERROR_RUN`.
    This event is returned if the configuration provided to :c:func:`lwm2m_carrier_run` is invalid.

* Removed ``certification_mode`` from the configuration parameters of :c:macro:`lwm2m_carrier_config_t`.
* Removed the ``CONFIG_LWM2M_CARRIER_CERTIFICATION_MODE`` Kconfig.

  * The LwM2M carrier library always connects to the correct production (live) server (if in an applicable network).
  * To connect to a certification (test) server, you must enter the appropriate URI using :kconfig:option:`CONFIG_LWM2M_CARRIER_CUSTOM_URI`.

* Removed ``psk`` from the configuration parameters of :c:macro:`lwm2m_carrier_config_t`.
* Removed the ``CONFIG_LWM2M_CARRIER_CUSTOM_PSK`` Kconfig.
* Added ``server_sec_tag`` to the configuration :c:macro:`lwm2m_carrier_config_t`.
* Added the :kconfig:option:`CONFIG_LWM2M_CARRIER_SERVER_SEC_TAG` Kconfig option.

  * The LwM2M carrier library no longer uses PSK as a configuration parameter.
    Instead, you can provide a ``sec_tag`` (containing a PSK).
  * The :ref:`lwm2m_carrier` sample now contains a Kconfig option :ref:`CONFIG_CARRIER_APP_PSK <CONFIG_CARRIER_APP_PSK>`, which will be written to the security tag provided by :kconfig:option:`CONFIG_LWM2M_CARRIER_SERVER_SEC_TAG`.
    This was added for convenience during development but must not be used for production.
  * See :ref:`modem_key_mgmt` for more information about using security tags, and :ref:`lwm2m_carrier_provisioning` for information on provisioning them for the LwM2M carrier library.

* Removed the Kconfig option ``CONFIG_LWM2M_CARRIER_USE_CUSTOM_URI``.

  * You need to use only the Kconfig option :kconfig:option:`CONFIG_LWM2M_CARRIER_CUSTOM_URI`.
    If the Kconfig option is empty, it is ignored.

* Added the Kconfig option :kconfig:option:`CONFIG_LWM2M_CARRIER_PDN_TYPE`.

  * The new ``pdn_type`` parameter in :c:macro:`lwm2m_carrier_config_t` is used to select the PDN type of the ``apn`` parameter.

* Added the Kconfig option :kconfig:option:`CONFIG_LWM2M_CARRIER_LG_UPLUS_DEVICE_SERIAL_NUMBER`.

  * This configuration lets you choose between using the nRF9160 SiP 2DID Serial Number, or the Device IMEI as a Serial Number when connecting to the LG U+ device management server.
  * Now that there are several LG U+ options, they have been grouped in :c:struct:`lwm2m_carrier_lg_uplus_config_t` inside :c:struct:`lwm2m_carrier_config_t`.

* Added the ``carriers_enabled`` parameter to :c:macro:`lwm2m_carrier_config_t`.

  * This parameter allows you to enable or disable the Carrier Library based on which Subscriber ID is used in the current network.

  * Added Kconfig options to the new "Enabled Carriers" menu:

    * :kconfig:option:`CONFIG_LWM2M_CARRIER_GENERIC`
    * :kconfig:option:`CONFIG_LWM2M_CARRIER_VERIZON`
    * :kconfig:option:`CONFIG_LWM2M_CARRIER_ATT`
    * :kconfig:option:`CONFIG_LWM2M_CARRIER_T_MOBILE`
    * :kconfig:option:`CONFIG_LWM2M_CARRIER_LG_UPLUS`

* Added the ``server_binding`` parameter to :c:macro:`lwm2m_carrier_config_t`.

  * This optional value can be left empty to use the default binding (UDP).
  * Added the new Kconfig :kconfig:option:`CONFIG_LWM2M_SERVER_BINDING_CHOICE`.
  * The binding can be either ``U`` (UDP) or ``N`` (Non-IP).

* Added the function :c:func:`lwm2m_carrier_request`.

  * This function allows the application to request that the carrier library takes a certain action using the following definitions:

     * :c:macro:`LWM2M_CARRIER_REQUEST_REBOOT`
     * :c:macro:`LWM2M_CARRIER_REQUEST_LINK_UP`
     * :c:macro:`LWM2M_CARRIER_REQUEST_LINK_DOWN`

  * This function allows the LwM2M carrier library to disconnect gracefully and it is mandatory to use when the Subscriber ID is LG U+.

liblwm2m_carrier 0.30.2
***********************

Release for modem firmware version 1.3.3.

Certification status
====================

For certification status, see `Mobile network operator certifications`_.

Changes
=======

* Added the functions :c:func:`lwm2m_os_lte_modes_get` and :c:func:`lwm2m_os_lte_mode_request`.

  * This makes it possible for the LwM2M library to make the device switch between NB-IoT and LTE-M networks.

Size
====

See :ref:`lwm2m_lib_size` for an explanation of the library size in different scenarios.

+-------------------------+---------------+------------+
|                         | Flash (Bytes) | RAM (Bytes)|
+-------------------------+---------------+------------+
| Library size            | 72186         | 15840      |
| (binary)                |               |            |
+-------------------------+---------------+------------+
| Library size            | 93784         | 38968      |
| (reference application) |               |            |
+-------------------------+---------------+------------+

liblwm2m_carrier 0.30.1
***********************

Release for modem firmware version 1.3.3.

Certification status
====================

For certification status, see `Mobile network operator certifications`_.

Changes
=======

* Minor fixes and improvements.

liblwm2m_carrier 0.30.0
***********************

Release for modem firmware version 1.3.1 and 1.3.2.

Certification status
====================

For certification status, see `Mobile network operator certifications`_.

Size
====

See :ref:`lwm2m_lib_size` for an explanation of the library size in different scenarios.

+-------------------------+---------------+------------+
|                         | Flash (Bytes) | RAM (Bytes)|
+-------------------------+---------------+------------+
| Library size            | 71582         | 15844      |
| (binary)                |               |            |
+-------------------------+---------------+------------+
| Library size            | 93876         | 38824      |
| (reference application) |               |            |
+-------------------------+---------------+------------+

Changes
=======

* Added support for LG U+ network operator.

* Added the App Data Container object (10250).
* Added support for application FOTA in the glue layer. This is required for LG U+ support.
* Added the Kconfig options :kconfig:option:`CONFIG_LWM2M_CARRIER_LG_UPLUS` and :kconfig:option:`CONFIG_LWM2M_CARRIER_LG_UPLUS_SERVICE_CODE`.

* Removed the Kconfig options ``CONFIG_LWM2M_CARRIER_USE_CUSTOM_PSK`` and ``CONFIG_LWM2M_CARRIER_USE_CUSTOM_APN``.

  * Instead, only the Kconfig options :kconfig:option:`CONFIG_LWM2M_CARRIER_CUSTOM_PSK` and :kconfig:option:`CONFIG_LWM2M_CARRIER_CUSTOM_APN` are needed. If the Kconfig options are empty, they are ignored.

* Renamed the event ``LWM2M_CARRIER_EVENT_CARRIER_INIT`` to :c:macro:`LWM2M_CARRIER_EVENT_INIT`.
* Removed the event ``LWM2M_CARRIER_EVENT_CERTS_INIT`` and initialization parameter ``lwm2m_carrier_event_certs_init_t``.

 * Instead, certificates can be written to modem when the :c:macro:`LWM2M_CARRIER_EVENT_INIT` event is received, before attaching to the network.
 * List of certificates is no longer supplied to the :c:func:`lwm2m_carrier_init` function. LwM2M carrier library will instead iterate through all CA certificates in the modem.

* Added the Kconfig option :kconfig:option:`CONFIG_LWM2M_CARRIER_SESSION_IDLE_TIMEOUT`.
* Removed some runtime resource ``_set()`` functions. The resources are static and therefore added to library initialization instead.

  * Removed ``lwm2m_carrier_device_type_set()``, ``lwm2m_carrier_hardware_version_set()`` and ``lwm2m_carrier_software_version_set()``.
  * Added :kconfig:option:`CONFIG_LWM2M_CARRIER_DEVICE_TYPE`, :kconfig:option:`CONFIG_LWM2M_CARRIER_DEVICE_HARDWARE_VERSION` and :kconfig:option:`CONFIG_LWM2M_CARRIER_DEVICE_SOFTWARE_VERSION`.

* Added new initialization configurations to set the static device object resources:

  * :kconfig:option:`CONFIG_LWM2M_CARRIER_DEVICE_MANUFACTURER`
  * :kconfig:option:`CONFIG_LWM2M_CARRIER_DEVICE_MODEL_NUMBER`

* The LwM2M carrier library now requests the application to handle the LTE link, instead of handling the link on its own.

  * Removed the glue functions ``lwm2m_os_lte_link_up()``, ``lwm2m_os_lte_link_down()``, and ``lwm2m_os_lte_power_down()``.
  * Removed the events ``LWM2M_CARRIER_EVENT_CONNECTING```, ``LWM2M_CARRIER_EVENT_CONNECTED``, ``LWM2M_CARRIER_EVENT_DISCONNECTING``, and ``LWM2M_CARRIER_EVENT_DISCONNECTED``.
  * Added the events :c:macro:`LWM2M_CARRIER_EVENT_LTE_LINK_UP`, :c:macro:`LWM2M_CARRIER_EVENT_LTE_LINK_DOWN`, and :c:macro:`LWM2M_CARRIER_EVENT_LTE_POWER_OFF`.
* Renamed the error ``LWM2M_CARRIER_ERROR_CONNECT_FAIL`` to :c:macro:`LWM2M_CARRIER_ERROR_LTE_LINK_UP_FAIL`.
* Renamed the error ``LWM2M_CARRIER_ERROR_DISCONNECT_FAIL`` to :c:macro:`LWM2M_CARRIER_ERROR_LTE_LINK_DOWN_FAIL`.
* Removed the event ``LWM2M_CARRIER_EVENT_LTE_READY``. The event was intended to help the user application coexist with the library, but it was not useful.

  * Actions to bring the link up and down are requested using the new :c:macro:`LWM2M_CARRIER_EVENT_LTE_LINK_UP` and :c:macro:`LWM2M_CARRIER_EVENT_LTE_LINK_DOWN` events.
    The application can therefore perform housekeeping at these events if needed.
  * Even when the ``LWM2M_CARRIER_EVENT_LTE_READY`` event was sent to the application, the carrier library could still disconnect the link to write keys to the modem after a while in some cases.
  * Any application must handle untimely disconnects anyway, because of factors such as signal coverage, making the ``LWM2M_CARRIER_EVENT_LTE_READY`` event redundant.

* NVS records are no longer statically defined by a devicetree partition. Instead, the :ref:`partition_manager` is used to define flash partition dynamically.

  * To use the legacy NVS partition, you can add a ``pm_static.yml`` file to your project with the following content:

    .. code-block:: none

       lwm2m_carrier:
         address: 0xfa000
         size: 0x3000
       free:
         address: 0xfd000
         size: 0x3000

    This is strongly encouraged if you are updating deployed devices, to make sure that the persistent configuration of the library is preserved across the versions.
    The address of the previous storage can be confirmed by checking the :file:`build/zephyr/zephyr.dts` file in your old project.

liblwm2m_carrier 0.22.0
***********************

Release for modem firmware version 1.3.1.

Certification status
====================

For certification status, see `Mobile network operator certifications`_.

Size
====

See :ref:`lwm2m_lib_size` for an explanation of the library size in different scenarios.

+-------------------------+---------------+------------+
|                         | Flash (Bytes) | RAM (Bytes)|
+-------------------------+---------------+------------+
| Library size            | 67872         | 15484      |
| (binary)                |               |            |
+-------------------------+---------------+------------+
| Library size            | 90532         | 37592      |
| (reference application) |               |            |
+-------------------------+---------------+------------+

Changes
=======

* Added a new event :c:macro:`LWM2M_CARRIER_DEFERRED_SIM_MSISDN`.

  * This event can trigger on devices with a new SIM card that has not been registered on the network yet.
    (Therefore, it has not received the phone number needed for device management).
* Removed dependency on the deprecated AT command and AT notification libraries from the glue layer.
* Added dependency on the AT monitor library in the glue layer.
* Changed the :c:func:`lwm2m_os_lte_link_up` function to perform an asynchronous connect.
* Removed the following unused functions from the glue layer: ``lwm2m_os_sec_ca_chain_exists()``, ``lwm2m_os_sec_ca_chain_cmp()``, ``lwm2m_os_sec_ca_chain_write()``.

liblwm2m_carrier 0.21.0
***********************

Release for modem firmware version 1.3.1.

Certification status
====================

For certification status, see `Mobile network operator certifications`_.

Size
====

See :ref:`lwm2m_lib_size` for an explanation of the library size in different scenarios.

+-------------------------+---------------+------------+
|                         | Flash (Bytes) | RAM (Bytes)|
+-------------------------+---------------+------------+
| Library size            | 75216         | 14275      |
| (binary)                |               |            |
+-------------------------+---------------+------------+
| Library size            | 103104        | 42672      |
| (reference application) |               |            |
+-------------------------+---------------+------------+

Changes
=======

* Library can now be provided a non-bootstrap custom URI. Previously, only bootstrap custom URI was accepted.

  * New Kconfig :kconfig:option:`CONFIG_LWM2M_CARRIER_IS_SERVER_BOOTSTRAP` indicates if the custom URI is a Bootstrap-Server.
  * New Kconfig :kconfig:option:`CONFIG_LWM2M_CARRIER_SERVER_LIFETIME` sets the lifetime for the (non-bootstrap) LwM2M server.
* Library will now read bootstrap information from Smartcard when applicable.

  * New Kconfig :kconfig:option:`CONFIG_LWM2M_CARRIER_BOOTSTRAP_SMARTCARD` can be used to disable this feature.
* Added a new event :c:macro:`LWM2M_CARRIER_EVENT_MODEM_DOMAIN` to indicate modem domain events.
* Removed logging from the OS glue layer.
* Added the Cellular Connectivity object.

  * Increased +CEREG notification level requirement from 2 to 4, so that the library can receive Active-Time and Periodic-TAU.
* Added the Location object, including the API :c:func:`lwm2m_carrier_location_set` and :c:func:`lwm2m_carrier_velocity_set`.

* Removed a limitation which stated that the application could not use the NB-IoT LTE mode.

liblwm2m_carrier 0.20.1
***********************

Release for modem firmware version 1.3.0.

Certification status
====================

For certification status, see `Mobile network operator certifications`_.

Size
====

See :ref:`lwm2m_lib_size` for an explanation of the library size in different scenarios.

+-------------------------+---------------+------------+
|                         | Flash (Bytes) | RAM (Bytes)|
+-------------------------+---------------+------------+
| Library size            | 64620         | 10687      |
| (binary)                |               |            |
+-------------------------+---------------+------------+
| Library size            | 109520        | 35184      |
| (reference application) |               |            |
+-------------------------+---------------+------------+

Changes
=======

* Fixed a race condition that could render the LwM2M carrier library unresponsive.

liblwm2m_carrier 0.20.0
***********************

Release for modem firmware version 1.3.0.

Certification status
====================

For certification status, see `Mobile network operator certifications`_.

Changes
=======

* CA certificates are no longer provided by the LwM2M carrier library.

  * Application is now expected to store CA certificates into the modem security tags.
  * Added a new event :c:macro:`LWM2M_CARRIER_EVENT_CERTS_INIT` that instructs the application to provide the CA certificate security tags to the LwM2M carrier library.
* Renamed the event :c:macro:`LWM2M_CARRIER_BSDLIB_INIT` to :c:macro:`LWM2M_CARRIER_EVENT_MODEM_INIT`.
* Added a new deferred event reason :c:macro:`LWM2M_CARRIER_DEFERRED_SERVICE_UNAVAILABLE`, which indicates that the LwM2M server is unavailable due to maintenance.
* Added a new error code :c:macro:`LWM2M_CARRIER_ERROR_CONFIGURATION` which indicates that an illegal object configuration was detected.
* Added new Kconfig options :kconfig:option:`CONFIG_LWM2M_CARRIER_USE_CUSTOM_APN` and :kconfig:option:`CONFIG_LWM2M_CARRIER_CUSTOM_APN` to set the ``apn`` member of :c:type:`lwm2m_carrier_config_t`.
* It is now possible to configure a custom bootstrap URI using :kconfig:option:`CONFIG_LWM2M_CARRIER_USE_CUSTOM_BOOTSTRAP_URI` regardless of operator SIM.

liblwm2m_carrier 0.10.2
***********************

Release for modem firmware versions 1.2.3 and 1.1.4, and |NCS| 1.4.2.

Certification status
====================

For certification status, see `Mobile network operator certifications`_.

Size
====

See :ref:`lwm2m_lib_size` for an explanation of the library size in different scenarios.

+-------------------------+---------------+------------+
|                         | Flash (Bytes) | RAM (Bytes)|
+-------------------------+---------------+------------+
| Library size            | 61728         | 10226      |
| (binary)                |               |            |
+-------------------------+---------------+------------+
| Library size            | 97116         | 29552      |
| (reference application) |               |            |
+-------------------------+---------------+------------+

Changes
=======

* :c:macro:`LWM2M_CARRIER_EVENT_LTE_READY` will be sent to the application even when the device is outside of AT&T and Verizon networks.
* The interval to check for sufficient battery charge during FOTA has been reduced from five minutes to one minute.

liblwm2m_carrier 0.10.1
***********************

Release for modem firmware versions 1.2.2 and 1.1.4, and |NCS| 1.4.1.

Certification status
====================

For certification status, see `Mobile network operator certifications`_.

Changes
=======

* Minor fixes and improvements.

liblwm2m_carrier 0.10.0
***********************

Snapshot release for modem firmware version 1.2.2 and the upcoming version 1.1.4, and |NCS| 1.4.0.

This release is intended to let users begin integration towards the AT&T and Verizon device management platforms.
Modem firmware version 1.1.4 must be used for Verizon, and the modem firmware version 1.2.2 must be used for AT&T.

The snapshot can be used for development and testing only.
It is not ready for certification.

Certification status
====================

The library is not certified with any carrier.

Changes
=======

* Reduced the required amount of stack and heap allocated by the library.
* Reduced the power consumption of the library.
* Renamed the event :c:macro:`LWM2M_CARRIER_EVENT_READY` to :c:macro:`LWM2M_CARRIER_EVENT_REGISTERED`.
* Introduced a new event :c:macro:`LWM2M_CARRIER_EVENT_LTE_READY`, to indicate that the LTE link can be used by the application.
* The Modem DFU socket can now be used by the application when it is not needed by the library.

liblwm2m_carrier 0.9.1
**********************

Release with AT&T support, intended for modem firmware version 1.2.1 and |NCS| version 1.3.1.

Certification status
====================

The library is certified with AT&T.

Size
====

See :ref:`lwm2m_lib_size` for an explanation of the library size in different scenarios.

+-------------------------+---------------+------------+
|                         | Flash (Bytes) | RAM (Bytes)|
+-------------------------+---------------+------------+
| Library size            | 61450         | 9541       |
| (binary)                |               |            |
+-------------------------+---------------+------------+
| Library size            | 92750         | 30992      |
| (reference application) |               |            |
+-------------------------+---------------+------------+

Changes
=======

* Minor fixes and improvements.

liblwm2m_carrier 0.9.0
**********************

Snapshot release for the upcoming modem firmware version 1.2.1 and the |NCS| 1.3.0.

This release is intended to let users begin integration towards the AT&T and Verizon device management platforms.
It can be used for development and testing only.
It is not ready for certification.

Certification status
====================

The library is not certified with any carrier.

Changes
=======

* Added new APIs to create and access portfolio object instances.
  A new portfolio object instance can be created using ``lwm2m_carrier_portfolio_instance_create()``.
  ``lwm2m_carrier_identity_read()`` and ``lwm2m_carrier_identity_write()`` are used to read and write to the corresponding Identity resource fields of a given portfolio object instance.
* Expanded API with "certification_mode" variable that chooses between certification (test) or production (live) servers upon the initialization of the LwM2M carrier library.
* Expanded API with "apn" variable to set a custom APN upon the initialization of the LwM2M carrier library.
* PSK Key is now set independently of custom URI.

  * Added the LWM2M_CARRIER_USE_CUSTOM_BOOTSTRAP_PSK and LWM2M_CARRIER_CUSTOM_BOOTSTRAP_PSK Kconfig options.

* PSK format has been modified to be more user-friendly.

  * Previous format: Byte array. For example, ``static const char bootstrap_psk[] = {0x01, 0x02, 0xab, 0xcd, 0xef};``.
  * Current format: A null-terminated string that must be composed of hexadecimal numbers. For example, "0102abcdef".

liblwm2m_carrier 0.8.2
**********************

Release for modem firmware version 1.1.2, with support for Verizon Wireless.

Certification status
====================

The library is certified with Verizon Wireless.

Changes
=======

* Fixed a memory leak.
* Added lwm2m_carrier_event_deferred_t to retrieve the event reason and timeout.
* Added FOTA errors to LWM2M_CARRIER_EVENT_ERROR event.

Size
====

See :ref:`lwm2m_lib_size` for an explanation of the library size in different scenarios.

+-------------------------+---------------+------------+
|                         | Flash (Bytes) | RAM (Bytes)|
+-------------------------+---------------+------------+
| Library size            | 45152         | 7547       |
| (binary)                |               |            |
+-------------------------+---------------+------------+
| Library size            | 65572         | 28128      |
| (reference application) |               |            |
+-------------------------+---------------+------------+

liblwm2m_carrier 0.8.1+build1
*****************************

Release for modem firmware version 1.1.0, with support for Verizon Wireless.

Certification status
====================

The library is certified with Verizon Wireless.

Changes
=======

* Fixed a memory leak.

Size
====

See :ref:`lwm2m_lib_size` for an explanation of the library size in different scenarios.

+-------------------------+---------------+------------+
|                         | Flash (Bytes) | RAM (Bytes)|
+-------------------------+---------------+------------+
| Library size            | 44856         | 7546       |
| (binary)                |               |            |
+-------------------------+---------------+------------+
| Library size            | 64680         | 28128      |
| (reference application) |               |            |
+-------------------------+---------------+------------+

liblwm2m_carrier 0.8.1
**********************

Release for modem firmware version 1.1.0, with support for Verizon Wireless.

Certification status
====================

The library is certified with Verizon Wireless.

Changes
=======

* Numerous stability fixes and improvements.
* Updated Modem library version dependency.
* Fixed an issue where high LTE network activity could prevent modem firmware updates over LwM2M.

* Added the following library events:

   * LWM2M_CARRIER_EVENT_CONNECTING, to indicate that the LTE link is about to be brought up.
   * LWM2M_CARRIER_EVENT_DISCONNECTING, to indicate that the LTE link is about to be brought down.
   * LWM2M_CARRIER_EVENT_DEFERRED, to indicate that the LwM2M operation is deferred for 24 hours.
   * LWM2M_CARRIER_EVENT_ERROR, to indicate that an error has occurred.

* Renamed the following library events:

   * LWM2M_CARRIER_EVENT_CONNECT to LWM2M_CARRIER_EVENT_CONNECTED.
   * LWM2M_CARRIER_EVENT_DISCONNECT to LWM2M_CARRIER_EVENT_DISCONNECTED.


liblwm2m_carrier 0.8.0
**********************

Release for modem firmware version 1.1.0 and |NCS| v1.1.0, with support for Verizon Wireless.

Certification status
====================

The library is not certified with Verizon Wireless.

Changes
=======

* Abstracted several new functions in the glue layer to improve compatibility on top of the master branch.
* Reorganized NVS keys usage to make it range-bound (0xCA00, 0xCAFF).
  This range is not backward compatible, so you should not rely on pre-existing information saved in flash by earlier versions of this library.
* Added APIs to set the following values from the application:

   * Available Power Sources
   * Power Source Voltage
   * Power Source Current
   * Battery Level
   * Battery Status
   * Memory Total
   * Error Code

  The application must set and maintain these values to reflect the state of the device.
  Updated values are pushed to the servers autonomously.

* Added API to set the "Device Type" resource. If not set, this is reported as "Smart Device".
* Added API to set the "Software Version" resource. If not set, this is reported as "LwM2M 0.8.0".
* Added API to set the "Hardware Version" resource. If not set, this is reported as "1.0".

Known issues and limitations
============================

* It is not possible to use a DTLS connection in parallel with the library.
* It is not possible to use a TLS connection in parallel with LwM2M-managed modem firmware updates.
  The application should close any TLS connections when it receives the LWM2M_CARRIER_EVENT_FOTA_START event from the library.


liblwm2m_carrier 0.6.0
**********************

Initial public release for modem firmware version 1.0.1.
This release is intended to let users begin the integration on the Verizon Wireless device management platform and start the certification process with Verizon Wireless.
We recommend upgrading to the next release when it becomes available.
The testing performed on this release does not meet Nordic standard for mass production release testing.


Known issues and limitations
============================

* It is not possible to use a DTLS connection in parallel with the library.
* It is not possible to use a TLS connection in parallel with LwM2M-managed modem firmware updates. The application should close any TLS connections when it receives the LWM2M_CARRIER_EVENT_FOTA_START event from the library.
* The API to query the application for resource values is not implemented yet.

	* The "Available Power Sources" resource is reported as ``DC power (0)`` and ``External Battery (2)``.
	* The following resources are reported to have value ``0`` (zero):

		* Power Source Voltage, Power Source Current, Battery Level, Battery Status, Memory Free, Memory Total, Error Code.
	* The "Device Type" resource is reported as ``Smart Device``.
	* The "Software Version" resource is reported as ``LwM2M 0.6.0``.
	* The "Hardware Version" is reported as ``1.0``.
* The following values are reported as dummy values instead of being fetched from the modem:

	* "IP address", reported as ``192.168.0.0``.
* The "Current Time" and "Timezone" resources do not respect write operations, instead, read operations on these resources will return the current time and timezone as kept by the nRF91 Series modem.
