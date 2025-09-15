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

Libraries
=========

This section describes the changes related to libraries.

|no_changes_yet_note|

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

|no_changes_yet_note|

Libraries
=========

This section describes the changes related to libraries.

|no_changes_yet_note|
