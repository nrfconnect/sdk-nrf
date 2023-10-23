:orphan:

.. _migration_2.6:

Migration guide for |NCS| v2.6.0 (Working draft)
################################################

This document describes the changes required or recommended when migrating your application from |NCS| v2.5.0 to |NCS| v2.6.0.

Required changes
****************

The following changes are mandatory to make your application work in the same way as in previous releases.

* For the Serial LTE Modem (SLM) application:

  * The Zephyr settings backend has been changed from :ref:`FCB <fcb_api>` to :ref:`NVS <nvs_api>`.
    Because of this, one setting is restored to its default value after the switch if you are using the :ref:`liblwm2m_carrier_readme` library.
    The setting controls whether the SLM connects automatically to the network on startup.
    You can read and write it using the ``AT#XCARRIER="auto_connect"`` command.

Recommended changes
*******************

The following changes are recommended for your application to work optimally after the migration.

.. HOWTO

   Add changes in the following format:

.. * Change1 and description
.. * Change2 and description
