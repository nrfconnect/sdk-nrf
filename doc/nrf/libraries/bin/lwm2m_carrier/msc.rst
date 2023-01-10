.. _lwm2m_msc:

Message sequence charts
#######################

The following message sequence chart shows the states of the ``lwm2m_carrier_thread_run`` thread up until the registration with the device management server:

.. figure:: /libraries/bin/lwm2m_carrier/images/lwm2m_carrier_msc_bootstrap.svg
    :alt: LwM2M library bootstrap and register procedure

    LwM2M library bootstrap and register procedure

The following message sequence chart shows FOTA updates:

.. figure:: /libraries/bin/lwm2m_carrier/images/lwm2m_carrier_msc_fota_success.svg
    :alt: LwM2M server initiated FOTA

    LwM2M server initiated FOTA

The following message sequence chart shows that FOTA fails at run time if an invalid CA certificate is provided:

.. figure:: /libraries/bin/lwm2m_carrier/images/lwm2m_carrier_msc_fota_fail_cert.svg
    :alt: FOTA CA certificate failure

    FOTA CA certificate failure
