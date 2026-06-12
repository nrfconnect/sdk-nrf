.. _migration_3.5:

Migration notes for |NCS| v3.5.0 (Working draft)
################################################

.. contents::
   :local:
   :depth: 3

This document describes the changes required or recommended when migrating your application from |NCS| v3.4.0 to |NCS| v3.5.0.

.. HOWTO
   Add changes in the following format:
   Component (for example, application, sample or libraries)
   *********************************************************
   .. toggle::
      * Change1 and description
      * Change2 and description

.. _migration_3.5_required:

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

.. toggle::

   * :ref:`lib_location` library:

     * The library now always uses the chosen ``zephyr,wifi`` node to find the used Wi-Fi device.
       If your application uses the deprecated ``ncs,location-wifi`` node, you need to change it to use the ``zephyr,wifi`` node instead:

       .. code-block:: dts

          chosen {
                  zephyr,wifi = &mywifi;
          };

   * The Modem attestation token library has been renamed to :ref:`lib_attest_token` library.

     To migrate your application, complete the following steps:

     #. Replace the Kconfig options:

        * ``CONFIG_MODEM_ATTEST_TOKEN`` with :kconfig:option:`CONFIG_ATTEST_TOKEN`.
        * ``CONFIG_MODEM_ATTEST_TOKEN_PARSING`` with :kconfig:option:`CONFIG_ATTEST_TOKEN_PARSING`.

     #. Replace the :file:`modem/modem_attest_token.h` header file with :file:`attest_token.h`

     #. Replace the API calls:

        * ``modem_attest_token_get`` with :c:func:`attest_token_get`
        * ``modem_attest_token_free`` with :c:func:`attest_token_free`
        * ``modem_attest_token_parse`` with :c:func:`attest_token_parse`
        * ``modem_attest_token_get_uuids`` with :c:func:`attest_token_get_uuids`

Drivers
=======

This section describes the changes related to drivers.

|no_changes_yet_note|

.. _migration_3.5_recommended:

Recommended changes
*******************

The following changes are recommended for your application to work optimally after the migration.

Build and configuration system
==============================

|no_changes_yet_note|

Samples and applications
========================

This section describes the changes related to samples and applications.

|no_changes_yet_note|

Libraries
=========

This section describes the changes related to libraries.

|no_changes_yet_note|
