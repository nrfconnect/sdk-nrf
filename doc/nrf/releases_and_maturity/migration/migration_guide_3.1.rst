:orphan:

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

nRF5340 Audio applications
--------------------------

.. toggle::

   * The :ref:`nrf53_audio_app` has been updated to use the :ref:`net_buf_interface` API to handle audio data.
     This change requires you to update your application code to use the new APIs for audio data handling.
     See :ref:`ncs_release_notes_changelog` for more information.

Libraries
=========

This section describes the changes related to libraries.

* :ref:`nrf_security_readme` library:

  * The ``CONFIG_PSA_USE_CRACEN_ASYMMETRIC_DRIVER`` Kconfig option has been replaced by :kconfig:option:`CONFIG_PSA_USE_CRACEN_ASYMMETRIC_ENCRYPTION_DRIVER`.


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
