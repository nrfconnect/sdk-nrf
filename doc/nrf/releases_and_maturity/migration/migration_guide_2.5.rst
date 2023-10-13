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

.. HOWTO

   Add changes in the following format:

.. * Change1 and description
.. * Change2 and description
