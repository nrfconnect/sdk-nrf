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

  * The application can still use one DTLS session and one TLS session (or two DTLS sessions).
  * If the application is using the TLS socket, it must immediately close it when the :c:macro:`LWM2M_CARRIER_EVENT_FOTA_START` event is received.
  * The socket is released by the library again upon the next reboot, or in the event of a FOTA error.
  * If the application always needs a TLS socket, it can use `Mbed TLS`_.
  * For more information, see the :c:macro:`LWM2M_CARRIER_EVENT_FOTA_START` event in :ref:`lwm2m_events`.

* The LwM2M carrier library provisions the necessary security credentials to the security tags 25, 26, 27, 28.
  These tags must not be used by the application.
* If the Kconfig option :kconfig:option:`CONFIG_LWM2M_CARRIER_SERVER_SEC_TAG` is set, the range follows that ``sec_tag`` instead.

  * For example, setting :kconfig:option:`CONFIG_LWM2M_CARRIER_SERVER_SEC_TAG` to 42 uses the security tag range 43 to 46 instead of 25 to 28.

* The CA certificates that are used for out-of-band FOTA must be provided by the application.
  Out-of-band FOTA updates are done by the :ref:`lib_downloader`.
  Although the certificates are updated as part of the |NCS| releases, you must check the requirements from your carrier to know which certificates are applicable.

* The LwM2M carrier library uses the following NVS record key range: ``0xCA00`` to ``0xCAFF``.
  This range must not be used by the application.

.. _lwm2m_carrier_dependent:

Carrier-specific dependencies
*****************************

In order for your device to comply with the carrier's specifications, the following additional requirements apply to your application to use the carrier's LwM2M device management:

.. tabs::

   .. group-tab:: Verizon

        * In the Verizon network, the :ref:`liblwm2m_carrier_readme` library uses both DTLS sessions made available through the modem.
          Therefore, the application must expect to fail (or retry) if it attempts to establish a DTLS or TLS session.
          The application should never use a DTLS session indefinitely, because this will block the LwM2M carrier library.

   .. group-tab:: SoftBank

        * The application must support application FOTA.
        * The device must connect using a non-IP APN.

        The :ref:`lwm2m_carrier` sample features an extra config file to enable the :kconfig:option:`CONFIG_LWM2M_CARRIER_SOFTBANK` dependencies.

        * The device must operate in the NB-IoT system mode.

          * If the :ref:`lte_lc_readme` library is used, you can enable the Kconfig option :kconfig:option:`CONFIG_LTE_NETWORK_MODE_NBIOT` to set the system mode.

          * If the :ref:`lte_lc_readme` library is not used, see the `system mode section in the nRF9160 AT Commands Reference Guide`_ or the `system mode section in the nRF91x1 AT Commands Reference Guide`_, depending on the SiP you are using for more information on setting the NB-IoT system mode.

   .. group-tab:: LG U+

        * The application must support application FOTA.
          The :ref:`lwm2m_carrier` sample contains extra config files to enable the :kconfig:option:`CONFIG_LWM2M_CARRIER_LG_UPLUS` dependencies.
        * The application must set the LG U+ configurations listed in the :c:struct:`lwm2m_carrier_lg_uplus_config_t` structure.
          If you are unsure about which values to supply during certification, reach out to your carrier or your local Nordic Semiconductor sales representative.

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
