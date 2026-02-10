.. _migration_3.3:

Migration notes for |NCS| v3.3.0 (Working draft)
################################################

.. contents::
   :local:
   :depth: 3

This document describes the changes required or recommended when migrating your application from |NCS| v3.2.x to |NCS| v3.3.0.

.. HOWTO
   Add changes in the following format:
   Component (for example, application, sample or libraries)
   *********************************************************
   .. toggle::
      * Change1 and description
      * Change2 and description

.. _migration_3.3_required:

Required changes
****************

The following changes are mandatory to make your application work in the same way as in previous releases.

Samples and applications
========================

This section describes the changes related to samples and applications.

Matter
------

.. toggle::

   * Changed partitions mapping for the nRF7002 DK in the Matter samples and applications.
     This change breaks the DFU between the previous |NCS| versions and the upcoming release.
     Ensure that you have valid partition mappings for the nRF7002 DK in your project to perform the DFU.

Libraries
=========

This section describes the changes related to libraries.

.. toggle::

   * :ref:`lte_lc_readme` library:

     * The functions ``lte_lc_modem_events_enable()`` and ``lte_lc_modem_events_disable()`` have been removed.
       Instead, use the :kconfig:option:`CONFIG_LTE_LC_MODEM_EVENTS_MODULE` Kconfig option to enable modem events.

   * :ref:`nrf_security_readme` library:

      * Removed the ``CONFIG_PSA_WANT_KEY_TYPE_WPA3_SAE_PT`` Kconfig option and replaced it with :kconfig:option:`CONFIG_PSA_WANT_KEY_TYPE_WPA3_SAE`.
      * Removed the ``CONFIG_PSA_WANT_ALG_WPA3_SAE`` Kconfig option and replaced it by options :kconfig:option:`CONFIG_PSA_WANT_ALG_WPA3_SAE_FIXED` and :kconfig:option:`CONFIG_PSA_WANT_ALG_WPA3_SAE_GDH`.
      * Removed the ``CONFIG_MBEDTLS_PSA_STATIC_KEY_SLOT_BUFFER_SIZE`` Kconfig option.
        The PSA Crypto core is able to infer the key slot buffer size based on the keys enabled in the build, so there is no need to define it manually.

.. _migration_3.3_recommended:

Recommended changes
*******************

The following changes are recommended for your application to work optimally after the migration.

Samples and applications
========================

This section describes the changes related to samples and applications.

|no_changes_yet_note|

Libraries
=========

This section describes the changes related to libraries.

|no_changes_yet_note|
