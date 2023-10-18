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
* For the Serial LTE Modem (SLM) application:

  * The :kconfig:option:`CONFIG_SLM_AGPS` Kconfig option has been replaced by :kconfig:option:`CONFIG_NRF_CLOUD_AGNSS` and is now enabled by default.
  * The :kconfig:option:`CONFIG_SLM_PGPS` Kconfig option has been replaced by :kconfig:option:`CONFIG_NRF_CLOUD_PGPS`.
  * The :kconfig:option:`CONFIG_SLM_LOCATION` Kconfig option has been replaced by :kconfig:option:`CONFIG_NRF_CLOUD_LOCATION` and is now enabled by default.
  * The ``#XCELLPOS`` and ``#XWIFIPOS`` AT commands have been replaced by ``#XNRFCLOUDPOS``.
  * The ``#XAGPS`` and ``#XPGPS`` AT commands have been replaced by ``#XGPS``.
  * The operations to read (``6``) or erase (``8``) the MCUboot secondary slot have been removed from the ``#XFOTA`` AT command.
  * The ``#XSLMUART`` AT command has been removed.
    UART is now configured using only devicetree.
  * The :kconfig:option:`CONFIG_SLM_CARRIER_APP_DATA_CONTAINER_BUFFER_LEN` Kconfig option has been renamed to :kconfig:option:`CONFIG_SLM_CARRIER_APP_DATA_BUFFER_LEN`.

* Applications using MCUboot must update to setting their version using an :ref:`application VERSION file <zephyr:app-version-details>` instead of the previously used ``CONFIG_MCUBOOT_IMAGE_VERSION`` Kconfig option.
  Alternatively, you can set the version in Kconfig by using the :kconfig:option:`CONFIG_MCUBOOT_IMGTOOL_SIGN_VERSION` option, but using a :file:`VERSION` file is the recommended approach.

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
* For the Serial LTE Modem (SLM) application:
  * If you are using the :ref:`liblwm2m_carrier_readme` library, make sure to take into account the addition of the auto-connect feature that is enabled by default.
  * When performing a modem firmware update, you can now reset only the modem (instead of the whole device) using the new ``#XMODEMRESET`` AT command.

* Applications that use :file:`prj_<board>.conf` Kconfig configurations should be transitioned to using :file:`boards/<board>.conf` Kconfig fragments.

.. HOWTO

   Add changes in the following format:

.. * Change1 and description
.. * Change2 and description
