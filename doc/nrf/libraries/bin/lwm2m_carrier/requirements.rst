.. _req_appln_limitations:

Requirements and application limitations
########################################

.. contents::
   :local:
   :depth: 2

Below are some of the requirements and limitations of the application while running this module.

* The application must not call the :c:func:`nrf_modem_lib_init` function.

   * The LwM2M carrier library initializes and uses the :ref:`nrf_modem`.
     This library is needed to track the modem FOTA states.

* The LwM2M carrier library registers to receive several AT event reports using the :ref:`at_cmd_readme` and :ref:`at_notif_readme` libraries. The following notifications must not be deregistered by the application:

   * SMS events (AT+CNMI).
   * Packet Domain events (AT+CGEREP).
   * Extended signal quality events (AT+CESQ).
   * Network Time events (AT%XTIME).
   * Modem Domain events (AT%MDMEV).
   * Report Network Error Codes events (AT+CNEC) - EPS Session Management events are used by the LwM2M carrier library. The application may enable or disable EPS Mobility Management events.
   * Network Registration Status events (AT+CEREG) - Notification Level 4 is used by the LwM2M carrier library. The application can increase this level but must not decrease it.

* If the application wants to use eDRX, it must enable mode 2, since the LwM2M carrier library requires the unsolicited result code.

* The LwM2M carrier library controls the LTE link.

   * The LwM2M carrier library stores keys into the modem, which requires disconnecting from the LTE link and connecting to it.
   * The application must wait for the :c:macro:`LWM2M_CARRIER_EVENT_LTE_READY` event before using the LTE link.

* The LwM2M carrier library uses the modem DFU socket and a TLS socket for FOTA.

  * The modem DFU socket is available to the application until a carrier-initiated modem DFU (FOTA) occurs.
  * If the application is using the modem DFU or TLS sockets, it must immediately close both when the :c:macro:`LWM2M_CARRIER_EVENT_FOTA_START` event is received.
    This is necessary to let the library use the only modem DFU socket, and to have sufficient memory to perform a TLS handshake.
  * If the application needs a TLS socket at all times, it can use `Mbed TLS`_.

* The LwM2M carrier library uses both the DTLS sessions made available through the modem. Therefore, the application cannot run any DTLS sessions.

* The LwM2M carrier library provisions the necessary security credentials to the security tags 25, 26, 27, 28.
  These tags must not be used by the application.

* The CA certificates that are used for out-of-band FOTA must be provided by the application.
  Upon initialization, the application will receive the event :c:macro:`LWM2M_CARRIER_EVENT_CERTS_INIT` (see :ref:`lwm2m_events`).
  Although the certificates are updated as part of the |NCS| releases, you must check the requirements from your carrier to know which certificates are applicable.

* The LwM2M carrier library uses the following NVS record key range: 0xCA00 - 0xCAFF.
  This range must not be used by the application.


.. _lwm2m_lib_size:

Library size
************

The following library sizes are reported in the :ref:`liblwm2m_carrier_changelog`:

 * Library size (binary): This shows the standalone size of the library. This size includes all objects since the library is not linked. This size will change when linking the library to an application.
 * Library size (reference application): This size shows the *total* memory impact of enabling the LwM2M carrier library in the :ref:`lwm2m_carrier` sample.
   This size accounts for the library, abstraction layer and the associated heap and stack requirements. It also includes all the resources for all the dependencies, except :ref:`nrf_modem`.
   See :ref:`lwm2m_app_int` for more information.

.. note::

   Enabling the LwM2M carrier library into the :ref:`lwm2m_carrier` sample serves only as a reference.
   The increase in memory size due to the inclusion of the LwM2M carrier library depends on the application that it is being integrated into.
   For example, an application such as the :ref:`asset_tracker` already uses several libraries which the LwM2M carrier library depends on. This makes the added memory requirement considerably smaller.
