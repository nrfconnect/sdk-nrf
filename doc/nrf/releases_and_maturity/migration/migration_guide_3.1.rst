.. _migration_3.1:

Migration guide for |NCS| v3.1.0 (Working draft)
################################################

.. contents::
   :local:
   :depth: 3

This document describes the changes required or recommended when migrating your application from |NCS| v3.0.0 to |NCS| v3.1.0.

.. HOWTO
   Add changes in the following format:
   Component (for example, application, sample or libraries)
   *********************************************************
   .. toggle::
      * Change1 and description
      * Change2 and description

.. _migration_3.1_required:

Required changes
****************

The following changes are mandatory to make your application work in the same way as in previous releases.

Samples and applications
========================

This section describes the changes related to samples and applications.

Matter
------

.. toggle::

   * For the Matter samples and applications:

      * The :kconfig:option:`CONFIG_NCS_SAMPLE_MATTER_ZAP_FILE_PATH` Kconfig option has been introduced.
        Previously, the path to the ZAP file was deduced based on hardcoded locations.
        Now, the location is configured using the :kconfig:option:`CONFIG_NCS_SAMPLE_MATTER_ZAP_FILE_PATH` Kconfig option.
        This change requires you to update your application ``prj.conf`` file by setting the :kconfig:option:`CONFIG_NCS_SAMPLE_MATTER_ZAP_FILE_PATH` option to point the location of you ZAP file.

   * For the :ref:`Matter light bulb <matter_light_bulb_sample>` sample:

      * The deferred attribute persistence implementation has changed in the latest Matter version and you must align it as follows:

        * Remove the following lines from the :file:`app_task.cpp` file located in the application's :file:`src` directory:

          .. code-block:: C++

             #include <app/DeferredAttributePersistenceProvider.h>

             DeferredAttributePersistenceProvider gDeferredAttributePersister(Server::GetInstance().GetDefaultAttributePersister(),
                                                                              Span<DeferredAttribute>(&gCurrentLevelPersister, 1),
                                                                              System::Clock::Milliseconds32(5000));

        * Add the following lines to the :file:`app_task.cpp` file located in the application's :file:`src` directory:

          .. code-block:: C++

             #include <app/util/persistence/DefaultAttributePersistenceProvider.h>
             #include <app/util/persistence/DeferredAttributePersistenceProvider.h>

             DefaultAttributePersistenceProvider gSimpleAttributePersistence;
             DeferredAttributePersistenceProvider gDeferredAttributePersister(gSimpleAttributePersistence,
                                                                              Span<DeferredAttribute>(&gCurrentLevelPersister, 1),
                                                                              System::Clock::Milliseconds32(5000));

        * Modify the ``mPostServerInitClbk`` function passed to the ``Nrf::Matter::PrepareServer`` function in the :file:`app_task.cpp` file should be modified to call additionally the ``gSimpleAttributePersistence.Init(Nrf::Matter::GetPersistentStorageDelegate())``.


Thread
------

.. toggle::

   * The OpenThread samples have been updated to directly use the OpenThread stack with the IEEE 802.15.4 radio driver.
     In this case, the Zephyr networking layer is disabled and its features are not available.

     The new architecture option has been enabled by default in the following samples:

     * :ref:`ot_coprocessor_sample`
     * :ref:`coap_server_sample`
     * :ref:`ot_cli_sample`

     If you want to use the architecture option that uses the Zephyr networking layer, you need to manually enable the following Kconfig options in your application :file:`prj.conf` file:

     * :kconfig:option:`CONFIG_NETWORKING`
     * :kconfig:option:`CONFIG_NET_L2_OPENTHREAD`

     Or use Snippet ``l2``.

     For example, to enable the Zephyr networking layer in the :ref:`ot_cli_sample` for the nRF54L15 DK, build the sample with the following command:

     .. code-block:: none

        west build -p -b nrf54l15dk/nrf54l15/cpuapp -- -Dcli_SNIPPET=l2

     Additionally, to enable logging from the Zephyr networking layer, you need to enable the following Kconfig options in your application :file:`prj.conf` file:

     * :kconfig:option:`CONFIG_OPENTHREAD_L2_DEBUG`
     * :kconfig:option:`CONFIG_OPENTHREAD_L2_LOG_LEVEL_DBG`
     * :kconfig:option:`CONFIG_OPENTHREAD_L2_DEBUG_DUMP_15_4`
     * :kconfig:option:`CONFIG_OPENTHREAD_L2_DEBUG_DUMP_IPV6`

     Or use snippet ``logging_l2``.

     For example, to enable logging from the Zephyr networking layer in the :ref:`ot_cli_sample` for the nRF54L15 DK, build the sample with the following command:

     .. code-block:: none

        west build -p -b nrf54l15dk/nrf54l15/cpuapp -- -Dcli_SNIPPET="l2;logging_l2"

nRF5340 Audio applications
--------------------------

.. toggle::

   * The :ref:`nrf53_audio_app` has been updated to use the :ref:`net_buf_interface` API to handle audio data.
     This change requires you to update your application code to use the new APIs for audio data handling.
     See :ref:`ncs_release_notes_changelog` for more information.
   * The audio devices are now identified by a location bitfield instead of a channel.
     This requires the location bitfield to be set according to your preference.
     If you are using the :file:`buildprog.py` script, it will handle the correct write to UICR.
     You only need to update the locations in the :file:`devices.json` file.
   * As a device can have multiple audio locations, the location name is no longer included in the advertised device name during DFU.

nRF Desktop
-----------

.. toggle::

   * The :ref:`nrf_desktop_hid_state` has been updated to use the :ref:`nrf_desktop_hid_keymap` to map an application-specific key ID to a HID report ID and HID usage ID pair.
     The ``CONFIG_DESKTOP_HID_STATE_HID_KEYMAP_DEF_PATH`` Kconfig option has been removed and needs to be replaced with the :ref:`CONFIG_DESKTOP_HID_KEYMAP_DEF_PATH <config_desktop_app_options>` Kconfig option.
     The format of the configuration file remains the same.

Libraries
=========

This section describes the changes related to libraries.

* :ref:`nrf_security_readme` library:

  * The ``CONFIG_PSA_USE_CRACEN_ASYMMETRIC_DRIVER`` Kconfig option has been replaced by :kconfig:option:`CONFIG_PSA_USE_CRACEN_ASYMMETRIC_ENCRYPTION_DRIVER`.

* :ref:`supl_client` library:

  * The SUPL client OS integration library dependency on the newlib C library has been removed.
    To use SUPL with picolibc, v0.8.0 or later of the nRF91 Series SUPL client library is required.

.. _migration_3.1_recommended:

Build system
============

.. toggle::

   * In sysbuild, the following CMake extensions have been removed:

     * ``sysbuild_dt_nodelabel``
     * ``sysbuild_dt_alias``
     * ``sysbuild_dt_node_exists``
     * ``sysbuild_dt_node_has_status``
     * ``sysbuild_dt_prop``
     * ``sysbuild_dt_comp_path``
     * ``sysbuild_dt_num_regs``
     * ``sysbuild_dt_reg_addr``
     * ``sysbuild_dt_reg_size``
     * ``sysbuild_dt_has_chosen``
     * ``sysbuild_dt_chosen``

     You must now use pre-existing devicetree extensions, such as ``dt_nodelabel``, without the ``sysbuild_`` prefix.
     To specify the sysbuild image, use the ``TARGET`` argument in place of ``IMAGE``.

     The following example shows one of the removed functions:

     .. code-block:: none

        sysbuild_dt_chosen(
          flash_node
          IMAGE ${DEFAULT_IMAGE}
          PROPERTY "zephyr,flash"
        )

     It should now be modified as follows:

     .. code-block:: none

        dt_chosen(
          flash_node
          TARGET ${DEFAULT_IMAGE}
          PROPERTY "zephyr,flash"
        )

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
