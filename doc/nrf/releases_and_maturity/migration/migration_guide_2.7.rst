.. _migration_2.7:

Migration guide for |NCS| v2.7.0 (Working draft)
################################################

.. contents::
   :local:
   :depth: 3

This document describes the changes required or recommended when migrating your application from |NCS| v2.6.0 to |NCS| v2.7.0.

.. HOWTO

   Add changes in the following format:

   Component (for example, application, sample or libraries)
   *********************************************************

   .. toggle::

      * Change1 and description
      * Change2 and description

.. _migration_2.7_required:

Required changes
****************

The following changes are mandatory to make your application work in the same way as in previous releases.

Samples and applications
========================

This section describes the changes related to samples and applications.

* For applications using the :ref:`lib_mqtt_helper` library:

  * The ``CONFIG_MQTT_HELPER_CERTIFICATES_FILE`` is now replaced by :kconfig:option:`CONFIG_MQTT_HELPER_CERTIFICATES_FOLDER`.
    The new option is a folder path where the certificates are stored.
    The folder path must be relative to the root of the project.

    If you are using the :ref:`lib_mqtt_helper` library, you must update the Kconfig option to use the new option.

  * When using the :kconfig:option:`CONFIG_MQTT_HELPER_PROVISION_CERTIFICATES` Kconfig option, the certificate files must be in standard PEM format.
    This means that the PEM files must not be converted to string format anymore.

Serial LTE Modem (SLM)
----------------------

.. toggle::

  * The AT command parsing has been updated to utilize the :ref:`at_cmd_custom_readme` library.
    If you have introduced custom AT commands to the SLM, you need to update the command parsing to use the new library.
    See the :ref:`slm_extending` page for more information.

Peripheral samples
------------------

* :ref:`radio_test` sample:

  * The CLI command ``fem tx_power_control <tx_power_control>`` replaces ``fem tx_gain <tx_gain>`` .
    This change applies to the sample built with the :kconfig:option:`CONFIG_RADIO_TEST_POWER_CONTROL_AUTOMATIC` set to ``n``.

Libraries
=========

This section describes the changes related to libraries.

FEM abstraction layer
---------------------

.. toggle::

  * For applications using :ref:`fem_al_lib`:
    The function :c:func:`fem_tx_power_control_set` replaces the function :c:func:`fem_tx_gain_set`.
    The function :c:func:`fem_default_tx_output_power_get` replaces the function :c:func:`fem_default_tx_gain_get`.

.. _migration_2.7_recommended:

Recommended changes
*******************

The following changes are recommended for your application to work optimally after the migration.

Samples and applications
========================

* For applications using build types (without Partition Manager or child images):

  * The :makevar:`CONF_FILE` used for :ref:`app_build_additions_build_types` is now deprecated and is being replaced with the :makevar:`FILE_SUFFIX` variable, inherited from Zephyr.
    You can read more about it in :ref:`app_build_file_suffixes`, :ref:`cmake_options`, and the :ref:`related Zephyr documentation <zephyr:application-file-suffixes>`.

    If your application uses build types, it is recommended to update the :file:`sample.yaml` to use the new variable instead of :makevar:`CONF_FILE`.

    .. note::
        :ref:`Partition Manager's static configuration <ug_pm_static>` and :ref:`child image Kconfig configuration <ug_multi_image_permanent_changes>` are not yet compatible with :makevar:`FILE_SUFFIX`.
        Read more about this in the note in :ref:`app_build_file_suffixes`.

Matter
------

.. toggle::

   * For the Matter samples and applications:

      * All Partition Manager configuration files (:file:`pm_static` files) have been removed from the :file:`configuration` directory.
        Instead, a :file:`pm_static_<BOARD>` file has been created for each target board and placed in the samples' directories.
        Setting the ``PM_STATIC_YML_FILE`` argument in the :file:`CMakeLists.txt` file has been removed, as it is no longer needed.

Libraries
=========

This section describes the changes related to libraries.

|no_changes_yet_note|
