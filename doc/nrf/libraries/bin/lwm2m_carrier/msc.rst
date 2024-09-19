.. _lwm2m_msc:

Message sequence charts
#######################

The following message sequence chart shows the state of the LwM2M carrier library and the events given to the application up until the successful registration with an LwM2M device management server:

.. _lwm2m_carrier_msc_bootstrap:

.. figure:: /libraries/bin/lwm2m_carrier/images/lwm2m_carrier_msc_bootstrap.svg
    :alt: LwM2M library bootstrap and register procedure

    LwM2M library bootstrap and register procedure

The following message sequence chart shows modem firmware updates initiated from the server:

.. _lwm2m_carrier_msc_fota_success:

.. figure:: /libraries/bin/lwm2m_carrier/images/lwm2m_carrier_msc_fota_success.svg
    :alt: LwM2M Server initiated modem FOTA

    LwM2M Server initiated modem FOTA

The following message sequence chart shows that FOTA fails at run time if an invalid CA certificate is provided:

.. _lwm2m_carrier_msc_fota_fail_cert:

.. figure:: /libraries/bin/lwm2m_carrier/images/lwm2m_carrier_msc_fota_fail_cert.svg
    :alt: FOTA CA certificate failure

    FOTA CA certificate failure

The following message sequence chart shows that FOTA fails at runtime if an invalid downloaded fragment fails to be verified:

.. _lwm2m_carrier_msc_fota_fail_crc:

.. figure:: /libraries/bin/lwm2m_carrier/images/lwm2m_carrier_msc_fota_fail_crc.svg
    :alt: FOTA CRC failure

    FOTA CRC failure

The following message sequence chart shows the :c:func:`lwm2m_carrier_request()` API being used to deregister from the server:

.. _lwm2m_carrier_request:

.. figure:: /libraries/bin/lwm2m_carrier/images/lwm2m_carrier_request.svg
    :alt: lwm2m_carrier_request() example

    lwm2m_carrier_request() example

The following message sequence chart shows examples of the :c:macro:`LWM2M_CARRIER_EVENT_DEFERRED` event being issued when connecting to a bootstrap server:

.. _lwm2m_carrier_msc_deferred_bs:

.. figure:: /libraries/bin/lwm2m_carrier/images/lwm2m_carrier_msc_deferred_bs.svg
    :alt: Deferred events for a bootstrap server

    Issues connecting to the bootstrap server leading to deferred events.

The following message sequence chart shows examples of the :c:macro:`LWM2M_CARRIER_EVENT_DEFERRED` event being issued when connecting to a LwM2M server:

.. _lwm2m_carrier_msc_deferred:

.. figure:: /libraries/bin/lwm2m_carrier/images/lwm2m_carrier_msc_deferred.svg
    :alt: Deferred events for a LwM2M server

    Issues connecting to the LwM2M server leading to deferred events.
