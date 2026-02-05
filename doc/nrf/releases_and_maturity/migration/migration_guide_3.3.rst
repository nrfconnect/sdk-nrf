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

Trusted Firmware-M
==================

.. toggle::

   * The default TF-M profile has changed.
     It is now :kconfig:option:`CONFIG_TFM_PROFILE_TYPE_NOT_SET` in all cases except for the Thingy:91 and Thingy:91 X board targets, on which it is still :kconfig:option:`CONFIG_TFM_PROFILE_TYPE_MINIMAL`.
     If your build is using the minimal TF-M profile, make sure to explicitly enable :kconfig:option:`CONFIG_TFM_PROFILE_TYPE_MINIMAL`.
     See :ref:`ug_tfm_building` for more information on the TF-M profiles.

   * For the nRF91 Series board targets that have MCUboot and a TF-M profile *other than minimal* enabled, the default TF-M partition size (as configured with :kconfig:option:`CONFIG_PM_PARTITION_SIZE_TFM`) has been reduced.
     In cases where it would previously be ``0x3FE00``, it is now ``0x17E00``.
     If your build was using the default value of ``0x3FE00`` and your TF-M flash usage is higher than the new value, make sure to explicitly set it to more than ``0x17E00`` to avoid flash overflow issues.

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
