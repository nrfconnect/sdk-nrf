.. _req_appln_limitations:

Requirements and application limitations
########################################

Below are some of the requirements and limitations of the application while running this module.

* The application should not call the :c:func:`bsdlib_init` function.

   * The LwM2M carrier library initializes and uses the :ref:`bsdlib`.
     This library is needed to track the modem FOTA states.

* The application should not use the *NB-IoT* LTE mode.

   * The LwM2M carrier library is currently only certified for the *LTE-M* LTE mode.
   * The :option:`CONFIG_LTE_NETWORK_USE_FALLBACK` should be disabled in your application, as seen in the :ref:`lwm2m_carrier` sample project configuration (:file:`nrf/samples/nrf9160/lwm2m_carrier/prj.conf`).

* The LwM2M carrier library registers to receive several AT event reports using the :ref:`at_cmd_readme` and :ref:`at_notif_readme` libraries. The following notifications should not be deregistered by the application:

   * SMS events (AT+CNMI).
   * Packet Domain events (AT+CGEREP).
   * Report Network Error Codes events (AT+CNEC): EPS Session Management events are used by the LwM2M carrier library. The application may enable or disable EPS Mobility Management events.
   * Network Registration Status events (AT+CEREG): Notification Level 2 is used by the LwM2M carrier library. The application may increase this level, but should not decrease it.

* The LwM2M carrier library controls the LTE link.

   * This is needed for storing keys to the modem, which requires disconnecting from an LTE link and connecting to it.
   * The application should not connect to the LTE link between a :c:macro:`LWM2M_CARRIER_EVENT_DISCONNECTING` event and :c:macro:`LWM2M_CARRIER_EVENT_CONNECTED` event.

* The LwM2M carrier library uses a TLS session for FOTA.
  TLS handshakes performed by the application might fail if the LwM2M carrier library is performing one at the same time.

   * If the application is in a TLS session and a :c:macro:`LWM2M_CARRIER_EVENT_FOTA_START` event is sent to the application, the application must immediately end the TLS session.
   * TLS becomes available again upon the next :c:macro:`LWM2M_CARRIER_EVENT_READY` or :c:macro:`LWM2M_CARRIER_EVENT_DEFERRED` event.
   * The application should implement a retry mechanism so that the application can perform the TLS handshake later.
   * Another alternative is to supply TLS from a source other than the modem, for example `Mbed TLS`_.

* The LwM2M carrier library uses both the DTLS sessions made available through the modem. Therefore, the application cannot run any DTLS sessions.

* The LwM2M carrier library provisions the necessary security credentials to the security tags 11, 25, 26, 27, 28.
  These tags should not be used by the application.

* The LwM2M carrier library uses the following NVS record key range: 0xCA00 - 0xCAFF.
  This range should not be used by the application.


.. _lwm2m_lib_size:

Library size
************

The following library sizes are reported in the :ref:`liblwm2m_carrier_changelog`:

 * Library size (binary): This shows the standalone size of the library. This size includes all objects, since the library is not linked. This size will change when linking the library to an application.
 * Library size (reference application): This size shows the *total* memory impact of enabling the LwM2M carrier library in the :ref:`lwm2m_carrier` sample.
   This size accounts for the library, abstraction layer and the associated heap and stack requirements. It also includes all the resources for all the dependencies, except :ref:`bsdlib`.
   See :ref:`lwm2m_app_int` for more information.

.. note::

   Enabling the LwM2M carrier library into the :ref:`lwm2m_carrier` sample serves only as a reference.
   The increase in memory size due to the inclusion of the LwM2M carrier library depends on the application that it is being integrated into.
   For example, an application such as the :ref:`asset_tracker` already uses several libraries which the LwM2M carrier library depends on. This makes the added memory requirement considerably smaller.
