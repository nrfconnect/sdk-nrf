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

LTE link control library
------------------------

.. toggle::

   * For applications using :ref:`lte_lc_readme`:

     * Remove all instances of the :c:func:`lte_lc_init` function.
     * Replace the use of the :c:func:`lte_lc_deinit` function with the :c:func:`lte_lc_power_off` function.
     * Replace the use of the :c:func:`lte_lc_init_and_connect` function with the :c:func:`lte_lc_connect` function.
     * Replace the use of the :c:func:`lte_lc_init_and_connect_async` function with the :c:func:`lte_lc_connect_async` function.
     * Remove the use of the ``CONFIG_LTE_NETWORK_USE_FALLBACK`` Kconfig option.
       Use the :kconfig:option:`CONFIG_LTE_NETWORK_MODE_LTE_M_NBIOT` or :kconfig:option:`CONFIG_LTE_NETWORK_MODE_LTE_M_NBIOT_GPS` Kconfig option instead.
       In addition, you can control the priority between LTE-M and NB-IoT using the :kconfig:option:`CONFIG_LTE_MODE_PREFERENCE` Kconfig option.

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
