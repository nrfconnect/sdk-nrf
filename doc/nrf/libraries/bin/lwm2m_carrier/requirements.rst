.. _req_appln_limitations:

Requirements and application limitations
########################################

.. contents::
   :local:
   :depth: 2

Following are some of the requirements and limitations of the application while running this module:

* The LwM2M carrier library registers to receive several AT event reports using the :ref:`at_monitor_readme` library. The following notifications must not be deregistered by the application:

   * SMS events (``AT+CNMI``).
   * Packet Domain events (``AT+CGEREP``).
   * Extended signal quality events (``AT%CESQ``).
   * ODIS events (``AT+ODISNTF``).
   * Universal Integrated Circuit Card events (``AT%XSIM``).
   * Network Time events (``AT%XTIME``).
   * Modem Domain events (``AT%MDMEV``).
   * Report Network Error Codes events (``AT+CNEC``) - EPS Session Management events are used by the LwM2M carrier library.
     The application may enable or disable EPS Mobility Management events.
   * Network Registration Status events (``AT+CEREG``) - Notification Level 4 is used by the LwM2M carrier library.
     The application can increase this level but must not decrease it.

* If the application wants to use eDRX, it must enable mode 2, since the LwM2M carrier library requires the unsolicited result code.

* The Connectivity statistics AT command (``AT%XCONNSTAT``) should not be used by the application.

  * This command is used by the LwM2M carrier library to update the Connectivity Statistics object.

* The application must prioritize the LwM2M carrier library requests to connect and disconnect from the network.

   * The LwM2M carrier library stores keys into the modem, which requires disconnecting from the LTE link and connecting to it.
   * The events to indicate connection and disconnection are :c:macro:`LWM2M_CARRIER_EVENT_LTE_LINK_UP` and :c:macro:`LWM2M_CARRIER_EVENT_LTE_LINK_DOWN`.

* The LwM2M carrier library uses the TLS socket for FOTA.

  * If the application is using the TLS socket, it must immediately close it when the :c:macro:`LWM2M_CARRIER_EVENT_FOTA_START` event is received.
  * The socket is released by the library again upon the next reboot, or in the event of a FOTA error.
  * If the application always needs a TLS socket, it can use `Mbed TLS`_.

* The LwM2M carrier library uses both the DTLS sessions made available through the modem. Therefore, the application cannot run any DTLS sessions.

* The LwM2M carrier library provisions the necessary security credentials to the security tags 25, 26, 27, 28.
  These tags must not be used by the application.

* The CA certificates that are used for out-of-band FOTA must be provided by the application.
  Out-of-band FOTA updates are done by the :ref:`lib_download_client`.
  Although the certificates are updated as part of the |NCS| releases, you must check the requirements from your carrier to know which certificates are applicable.

* The LwM2M carrier library uses the following NVS record key range: ``0xCA00`` to ``0xCAFF``.
  This range must not be used by the application.


.. _lwm2m_lib_size:

Library size
************

The following library sizes are reported in the :ref:`liblwm2m_carrier_changelog`:

 * Library size (binary) - This shows the standalone size of the library.
   This size includes all objects since the library is not linked.
   This size will change when linking the library to an application.
 * Library size (reference application) - This size shows the *total* memory impact of enabling the LwM2M carrier library in the :ref:`lwm2m_carrier` sample.
   This size accounts for the library, abstraction layer and the associated heap and stack requirements.
   It also includes all the resources for all the dependencies, except :ref:`nrf_modem`.
   See :ref:`lwm2m_app_int` for more information.

.. note::

   Enabling the LwM2M carrier library into the :ref:`lwm2m_carrier` sample serves only as a reference.
   The increase in memory size due to the inclusion of the LwM2M carrier library depends on the application that it is being integrated into.
   For example, an application such as the :ref:`asset_tracker_v2` already uses several libraries which the LwM2M carrier library depends on.
   This makes the added memory requirement considerably smaller.
