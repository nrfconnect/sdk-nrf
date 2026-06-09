.. _ug_nrf54h20_zms:

Enabling Zephyr Memory Storage
##############################

.. contents::
   :local:
   :depth: 2

.. include:: ../nrf54l/zms.rst
   :start-after: 54h_section_intro_start
   :end-before: 54h_section_intro_end

Enabling the ZMS module
***********************

.. include:: ../nrf54l/zms.rst
   :start-after: 54h_section_enabling_start
   :end-before: 54h_section_enabling_end

.. _ug_nrf54h20_zms_no_format_snapshot_recovery:

Mounting provisioned ZMS areas without formatting
*************************************************

If the device is in a lifecycle state where its file systems are expected to already be formatted, mount ZMS file systems with no-format.
This prevents reformatting the file system after the file system has been erased by corruption.

If mounting with no-format in ZMS fails, request :ref:`IronSide SE snapshot recovery <ug_nrf54h20_ironside_se_snapshot_recovery>`.

For information about mounting ZMS file systems with no-format, see :ref:`zephyr:zms_api`.

Optimizing ZMS in your application
**********************************

.. include:: ../nrf54l/zms.rst
   :start-after: 54h_section_optimizing_intro_start
   :end-before: 54h_section_optimizing_intro_end

Sector size and count
=====================

.. include:: ../nrf54l/zms.rst
   :start-after: 54h_section_sector_start
   :end-before: 54h_section_sector_end

Dimensioning cache
==================

.. include:: ../nrf54l/zms.rst
   :start-after: 54h_section_cache_start
   :end-before: 54h_section_cache_end
