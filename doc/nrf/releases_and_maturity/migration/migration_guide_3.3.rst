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

.. _matter_lock_snippets:

Matter
------

.. toggle::

   * Changed partitions mapping for the nRF7002 DK in the Matter samples and applications.
     This change breaks the DFU between the previous |NCS| versions and the upcoming release.
     Ensure that you have valid partition mappings for the nRF7002 DK in your project to perform the DFU.

   * Removed the application-specific snippets from the Matter samples.
     Snippets are available now in the global :file:`snippets/matter` directory in the |NCS| root.
     To see the snippets documentation, refer to the :ref:`matter_snippets` page.

     Refer to the following list of changes for more information:

     * :ref:`matter_light_switch_sample`

       * Removed the ``lit_icd`` snippet and enabled LIT ICD configuration by default.

     * :ref:`matter_lock_sample`

       * Removed the ``schedules`` snippet.
         Use the :file:`schedules.conf` configuration overlay file instead.

         For example:

         .. code-block:: none

           west build -b nrf54l15dk_nrf54l15_cpuapp -p -- -DEXTRA_CONF_FILE=schedules.conf

     * :ref:`matter_bridge_app`

       * Removed the ``onoff_plug`` snippet.
         Use the :file:`onoff_plug.conf` configuration overlay file instead.

         For example:

         .. code-block:: none

           west build -b nrf54l15dk_nrf54l15_cpuapp -p -- -DEXTRA_CONF_FILE=onoff_plug.conf

     * Renamed the ``diagnostic-logs`` snippet to ``matter-diagnostic-logs``.

Thread
------

.. toggle::

   * Removed all application-specific snippets from the Thread samples.
     New snippets are available now in the global :file:`snippets` directory in the |NCS| root.
     Sample-specific configurations were converted to extra configuration files in each sample's :file:`extra_conf/` directory.
     To see the snippets documentation, refer to the :ref:`openthread_snippets` page.

     Refer to the following table for the list of changes:

     .. list-table:: Thread samples — snippet and extra configuration file migration
        :widths: 22 22 36
        :header-rows: 1

        * - Sample
          - Removed snippet
          - Replacement
        * - :ref:`ot_cli_sample`
          - ``ci``, ``ci_l2``, ``debug``, ``logging``, ``usb``, ``diag_gpio``
          - Use global snippets ``ot-ci``, ``ot-debug``, ``ot-usb``, ``ot-diag-gpio``
        * -
          - ``logging_l2``
          - Use extra configuration file: :file:`extra_conf/l2_logging.conf`
        * -
          - ``low_power``
          - Use extra configuration file: :file:`extra_conf/low_power.conf` and extra Devicetree overlay file: :file:`extra_conf/low_power.overlay`
        * -
          - ``multiprotocol``
          - Use extra configuration file: :file:`extra_conf/multiprotocol.conf`
        * -
          - ``tcat``, ``tcp``
          - Use extra configuration files: :file:`extra_conf/tcat.conf`, :file:`extra_conf/tcp.conf`
        * - :ref:`coap_client_sample`
          - ``ci``, ``debug``, ``logging``
          - Use global snippets ``ot-ci``, ``ot-debug``.
        * -
          - ``mtd``
          - Use extra configuration file: :file:`extra_conf/mtd.conf`
        * -
          - ``multiprotocol_ble``
          - Use extra configuration file: :file:`extra_conf/multiprotocol_ble.conf`
        * - :ref:`coap_server_sample`
          - ``ci``, ``debug``, ``l2``, ``logging``, ``logging_l2``
          - Use global snippets ``ot-ci``, ``ot-debug``, ``ot-zephyr-l2``.
        * - :ref:`ot_coprocessor_sample`
          - ``ci``, ``debug``, ``logging``, ``usb``
          - Use global snippets ``ot-ci``, ``ot-debug``, ``ot-usb``.
        * -
          - ``hci``
          - Use extra configuration file: :file:`extra_conf/rcp_hci.conf` and extra Devicetree overlay file: :file:`extra_conf/rcp_hci.overlay`
        * -
          - ``vendor_hook``
          - Set the :kconfig:option:`CONFIG_OPENTHREAD_COPROCESSOR_VENDOR_HOOK_SOURCE` Kconfig option to the path to the vendor hook source file.
            The default one is :file:`src/user_vendor_hook.cpp`.

     Example for building with a global snippet and an overlay (CLI sample):

     .. code-block:: none

        west build -b nrf54l15dk_nrf54l15_cpuapp -p -- -DSNIPPET=ot-debug -DEXTRA_CONF_FILE=extra_conf/tcp.conf

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
