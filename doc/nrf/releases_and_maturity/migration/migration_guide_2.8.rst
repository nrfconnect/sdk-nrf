.. _migration_2.8:

Migration guide for |NCS| v2.8.0 (Working draft)
################################################

.. contents::
   :local:
   :depth: 3

This document describes the changes required or recommended when migrating your application from |NCS| v2.7.0 to |NCS| v2.8.0.

.. HOWTO

   Add changes in the following format:

   Component (for example, application, sample or libraries)
   *********************************************************

   .. toggle::

      * Change1 and description
      * Change2 and description

.. _migration_2.8_required:

Required changes
****************

The following changes are mandatory to make your application work in the same way as in previous releases.

Samples and applications
========================

This section describes the changes related to samples and applications.

|no_changes_yet_note|

Libraries
=========

This section describes the changes related to libraries.

LTE link control
----------------

.. toggle::

   For applications using :ref:`lte_lc_readme`:

     * If your application uses any of the following functions:

      * :c:func:`lte_lc_ptw_set`
      * :c:func:`lte_lc_edrx_param_set`
      * :c:func:`lte_lc_edrx_req`
      * :c:func:`lte_lc_edrx_get`
      * :c:func:`lte_lc_neighbor_cell_measurement`
      * :c:func:`lte_lc_neighbor_cell_measurement_cancel`
      * :c:func:`lte_lc_modem_events_enable`
      * :c:func:`lte_lc_modem_events_disable`

      Or handles any of the following callback events:

      * :c:macro:`LTE_LC_EVT_EDRX_UPDATE`
      * :c:macro:`LTE_LC_EVT_TAU_PRE_WARNING`
      * :c:macro:`LTE_LC_EVT_NEIGHBOR_CELL_MEAS`
      * :c:macro:`LTE_LC_EVT_MODEM_SLEEP_EXIT_PRE_WARNING`
      * :c:macro:`LTE_LC_EVT_MODEM_SLEEP_EXIT`
      * :c:macro:`LTE_LC_EVT_MODEM_SLEEP_ENTER`
      * :c:macro:`LTE_LC_EVT_MODEM_EVENT`

      Or sets any of the following Kconfig options to ``y``:

      * :kconfig:option:`LTE_LC_MODEM_SLEEP_NOTIFICATIONS`
      * :kconfig:option:`LTE_LC_TAU_PRE_WARNING_NOTIFICATIONS`

      Set the Kconfig option :kconfig:option:`CONFIG_LTE_EXTENDED_FEATURES` Kconfig option to ``y`` in the project configuration file :file:`prj.conf`.

.. _migration_2.8_recommended:

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
