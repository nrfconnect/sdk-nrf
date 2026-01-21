.. _migration_2.9:

Migration guide for |NCS| v2.9.0
################################

.. contents::
   :local:
   :depth: 3

This document describes the changes required or recommended when migrating your application from |NCS| v2.8.0 to |NCS| v2.9.0.

.. HOWTO
   Add changes in the following format:
   Component (for example, application, sample or libraries)
   *********************************************************
   .. toggle::
      * Change1 and description
      * Change2 and description

.. _migration_2.9_required:

Required changes
****************

The following changes are mandatory to make your application work in the same way as in previous releases.

.. _migration_2.9_recommended:

Recommended changes
*******************

The following changes are recommended for your application to work optimally after the migration.

Samples and applications
========================

This section describes the changes related to samples and applications.

Matter
------

.. toggle::

   * For the Matter samples and applications:

      * The :ref:`ug_matter_device_watchdog` mode has been changed.
        Previously, the :ref:`ug_matter_device_watchdog_pause_mode` was enabled by default for all Matter samples.
        Now, this mode is disabled and all Matter watchdog sources must be fed within the specified time window.

        To re-enable the pause mode, set the :option:`CONFIG_NCS_SAMPLE_MATTER_WATCHDOG_PAUSE_IN_SLEEP` Kconfig option to ``y``.
