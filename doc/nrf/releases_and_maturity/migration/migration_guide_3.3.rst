.. _migration_3.3:

Migration guide for |NCS| v3.3.0 (Working draft)
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

   * Changed the factory data build system to use sysbuild only instead of building it within the application image.
     
     The :kconfig:option:`CONFIG_CHIP_FACTORY_DATA_BUILD` Kconfig option is now read only and you cannot change its value.

     Matter factory data generation is now controlled by the :kconfig:option:`SB_CONFIG_MATTER_FACTORY_DATA_GENERATE` Kconfig option.
     This option is enabled by default in all |NCS| Matter samples and applications.

     To disable automatic generation of the factory data set, set the :kconfig:option:`SB_CONFIG_MATTER_FACTORY_DATA_GENERATE` Kconfig option to ``n``.

     The :kconfig:option:`CONFIG_CHIP_FACTORY_DATA` Kconfig option is set automatically to ``y`` if the :kconfig:option:`SB_CONFIG_MATTER_FACTORY_DATA_GENERATE` Kconfig option is enabled.
     In this case, the :kconfig:option:`CONFIG_CHIP_FACTORY_DATA` Kconfig option is read only and you cannot disable it.

     If you want to manually generate the factory data set, flash it to the device and use it, set the :kconfig:option:`SB_CONFIG_MATTER_FACTORY_DATA_GENERATE` Kconfig option to ``n`` and enable the factory data support by setting the :kconfig:option:`CONFIG_CHIP_FACTORY_DATA` Kconfig option to ``y``.

     For example:

     .. code-block:: console

        west build -b nrf52840dk_nrf52840 -- -DSB_CONFIG_MATTER_FACTORY_DATA_GENERATE=n -DCONFIG_CHIP_FACTORY_DATA=y

Libraries
=========

This section describes the changes related to libraries.

|no_changes_yet_note|

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
