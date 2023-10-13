:orphan:

.. _migration_2.5:

Migration guide for |NCS| v2.5.0 (Working draft)
################################################

This document describes the changes required or recommended when migrating your application from |NCS| v2.4.0 to |NCS| v2.5.0.

Required changes
****************

The following changes are mandatory to make your application work in the same way as in previous releases.

* All references to GNSS assistance have been changed from ``A-GPS`` to `A-GNSS`_.
  This affects the :ref:`nrfxlib:nrf_modem_gnss_api`, as well as :ref:`lib_nrf_cloud`, :ref:`lib_location`, and :ref:`lib_lwm2m_location_assistance` libraries.
  See release notes for changes related to each component.

Recommended changes
*******************

The following changes are recommended for your application to work optimally after the migration.

* Latest changes in Zephyr and nRF Connect SDK allow power optimization for the LwM2M client.
  Using DTLS Connection Identifier reduces the DTLS handshake overhead when performing the LwM2M Update operation.
  This is enabled using the :kconfig:option:`CONFIG_LWM2M_CLIENT_UTILS_DTLS_CID` Kconfig option and requires modem firmware v1.3.5 or newer.
  Zephyr's LwM2M engine now support tickless operation mode when the Kconfig option :kconfig:option:`CONFIG_LWM2M_TICKLESS` is enabled.
  This prevents the device from waking up on every 500 ms and achieves longer sleep periods.
  These power optimizations are enabled on the :ref:`lwm2m_client` sample when using the :file:`overlay-dtls-cid.conf` overlay file.
* Applications that use Zephyr's LwM2M stack and the :ref:`lib_lwm2m_client_utils` library must refactor to use the new event :c:member:`LWM2M_FOTA_UPDATE_MODEM_RECONNECT_REQ` when updating the modem firmware to avoid rebooting the device.
  For an example, see the :ref:`lwm2m_client` sample.
* Applications that use Zephyr's LwM2M stack are recommended to use the :kconfig:option:`CONFIG_LWM2M_UPDATE_PERIOD` Kconfig option to set the LwM2M update sending interval.

.. HOWTO

   Add changes in the following format:

.. * Change1 and description
.. * Change2 and description
