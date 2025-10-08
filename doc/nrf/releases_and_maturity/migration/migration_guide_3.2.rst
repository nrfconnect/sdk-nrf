:orphan:

.. _migration_3.2:

Migration guide for |NCS| v3.2.0 (Working draft)
################################################

.. contents::
   :local:
   :depth: 3

This document describes the changes required or recommended when migrating your application from |NCS| v3.1.0 to |NCS| v3.2.0.

.. HOWTO
   Add changes in the following format:
   Component (for example, application, sample or libraries)
   *********************************************************
   .. toggle::
      * Change1 and description
      * Change2 and description

.. _migration_3.2_required:

Required changes
****************

The following changes are mandatory to make your application work in the same way as in previous releases.

nRF54H20
========

This section describes the changes specific to the nRF54H20 SoC and DK support in the |NCS|.

nRF54H20 SoC binaries
---------------------

.. toggle::

   * The nRF54H20 SoC binaries based on IronSide SE have been updated to version v23.0.1+16.
     Starting from the |NCS| v3.2.0, you should always upgrade your nRF54H20 SoC binaries to the latest version.

     For more information, see:

     * :ref:`abi_compatibility` for details about the SoC binaries.
     * :ref:`ug_nrf54h20_ironside_se_update` for instructions on updating the SoC binaries.

Samples and applications
========================

This section describes the changes related to samples and applications.

Matter
------

.. toggle::

   * For the Matter samples and applications:

     * All Matter over Wi-Fi samples and applications now store a portion of the application code related to the nRF70 Series Wi-Fi firmware in external flash memory by default.

       There are two consequences of this change:

       * The partition map has been changed.
         You cannot perform DFU between the previous |NCS| versions and v3.2.0 unless you disable storing of the Wi-Fi firmware patch in external memory.
       * The application code size is reduced, but the programming process may take longer when performing the full erase, because the entire external flash memory is erased before programming the Wi-Fi firmware patch.

       When using the ``west flash`` command, the default behavior is to erase the entire external memory before programming the Wi-Fi firmware patch.
       To reduce programming time, you can add the ``--ext-erase-mode=ranges`` option to erase only the necessary memory ranges:

       .. code-block:: console

          west flash --ext-erase-mode=ranges

       The longer programming time also occurs when using the :guilabel:`Erase and Flash to Board` option in the |nRFVSC|.
       To speed up the process in the |nRFVSC|, use the :guilabel:`Flash` button instead of :guilabel:`Erase and Flash to Board` in the `Actions View`_.

       To disable storing the Wi-Fi firmware patch in external memory and revert to the previous approach, complete the following steps:

       1. Remove the Wi-Fi firmware patch partition from the partition list.
       #. Set the :kconfig:option:`SB_CONFIG_WIFI_PATCHES_EXT_FLASH_STORE` Kconfig option to ``n``.
       #. Set the :kconfig:option:`SB_CONFIG_DFU_MULTI_IMAGE_PACKAGE_WIFI_FW_PATCH` Kconfig option to ``n``.
       #. Set the :kconfig:option:`SB_CONFIG_MCUBOOT_UPDATEABLE_IMAGES` Kconfig option to ``2``.

     * The Kconfig options ``CONFIG_CHIP_SPI_NOR`` and ``CONFIG_CHIP_QSPI_NOR`` have been removed.
       Instead, use the standard options :kconfig:option:`CONFIG_SPI_NOR` and :kconfig:option:`CONFIG_NORDIC_QSPI_NOR`.
       The configuration system will now automatically enable these options when the corresponding SPI or QSPI device is specified in the devicetree.
       This change ensures more consistent configuration by relying on the standard Kconfig options when external NOR flash devices are present.

    * All Matter over Wi-Fi samples and applications now enable the :kconfig:option:`CONFIG_CHIP_WIFI` and :kconfig:option:`CONFIG_WIFI_NRF70` Kconfig options, depending on the board used.
      Previously, :kconfig:option:`CONFIG_CHIP_WIFI` was enabled in the Matter stack configuration if the nRF7002 DK or nRF7002 EK was used, which caused issues when building the application with custom boards.

      To build your custom board with Wi-Fi support, set both the :kconfig:option:`CONFIG_CHIP_WIFI` and :kconfig:option:`CONFIG_WIFI_NRF70` Kconfig options to ``y``.

Libraries
=========

This section describes the changes related to libraries.

.. toggle::

   * :ref:`lte_lc_readme` library:

     * The type of the :c:member:`lte_lc_evt.modem_evt` field has been changed to :c:struct:`lte_lc_modem_evt`.
       The modem event type can be determined from the :c:member:`lte_lc_modem_evt.type` field.
       Applications using modem events need to be updated to read the event type from ``modem_evt.type`` instead of ``modem_evt``.

     * Modem events ``LTE_LC_MODEM_EVT_CE_LEVEL_0``, ``LTE_LC_MODEM_EVT_CE_LEVEL_1``, ``LTE_LC_MODEM_EVT_CE_LEVEL_2`` and ``LTE_LC_MODEM_EVT_CE_LEVEL_3`` have been replaced by event :c:enumerator:`LTE_LC_MODEM_EVT_CE_LEVEL`.
       The CE level can be read from :c:member:`lte_lc_modem_evt.ce_level`.

     * Changed the order of the :c:enumerator:`LTE_LC_MODEM_EVT_SEARCH_DONE` modem event, and registration and cell related events.
       When the modem has completed the network selection, the registration and cell related events (:c:enumerator:`LTE_LC_EVT_NW_REG_STATUS`, :c:enumerator:`LTE_LC_EVT_CELL_UPDATE`, :c:enumerator:`LTE_LC_EVT_LTE_MODE_UPDATE` and :c:enumerator:`LTE_LC_EVT_PSM_UPDATE`) are sent first, followed by the :c:enumerator:`LTE_LC_MODEM_EVT_SEARCH_DONE` modem event.
       If the application depends on the order of the events, it may need to be updated.

Trusted Firmware-M
==================

.. toggle::

   * Trusted Firmware-M changed how data is stored and read in the Protected Storage partition.
     As a consequence, the applications that build with TF-M (``*/ns`` board targets) and want to perform a firmware upgrade to this |NCS| release will not be able to read the existing Protected Storage data with the default configuration.
     To enable reading the Protected Storage data from a previous release, make sure that the application enables the :kconfig:option:`CONFIG_TFM_PS_SUPPORT_FORMAT_TRANSITION` Kconfig option.

.. _migration_3.2_recommended:

Recommended changes
*******************

The following changes are recommended for your application to work optimally after the migration.

Samples and applications
========================

This section describes the changes related to samples and applications.

MCUboot
-------

The default C library for MCUboot has changed to picolibc.
Picolibc is recommended over the minimal C library as it is a fully developed and supported C library designed for application usage.
If you have not explicitly specified the C library in your sysbuild project for MCUboot using either a :file:`sysbuild/mcuboot/prj.conf` file or :file:`sysbuild/mcuboot.conf` file, picolibc will be used by default.
To set picolibc in your project, use the :kconfig:option:`CONFIG_PICOLIBC` Kconfig option.
If you need to use the minimal C library (which is not recommended outside of testing scenarios), use the :kconfig:option:`CONFIG_MINIMAL_LIBC` Kconfig option.

Libraries
=========

This section describes the changes related to libraries.

|no_changes_yet_note|
