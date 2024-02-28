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

Libraries
=========

This section describes the changes related to libraries.

|no_changes_yet_note|

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

      * All Partition Manager configuration files (:file:`pm_static` files) have been removed from the :file:`configuration` directory.
        Instead, a :file:`pm_static_<BOARD>` file has been created for each target board and placed in the samples' directories.
        Setting the ``PM_STATIC_YML_FILE`` argument in the :file:`CMakeLists.txt` file has been removed, as it is no longer needed.

Libraries
=========

This section describes the changes related to libraries.

|no_changes_yet_note|
