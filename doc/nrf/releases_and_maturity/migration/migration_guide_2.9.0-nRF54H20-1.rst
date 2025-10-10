:orphan:

.. _migration_2.9.0-nRF54H20-1:

Migration guide for |NCS| v2.9.0-nRF54H20-1
###########################################

.. contents::
   :local:
   :depth: 3

This document describes the changes required or recommended when migrating your nRF54H20 application from the |NCS| v2.8.0 to the |NCS| v2.9.0-nRF54H20-1.

.. HOWTO

   Add changes in the following format:

   Component (for example, application, sample or libraries)
   *********************************************************

   .. toggle::

      * Change1 and description
      * Change2 and description

.. _migration_2.9.0-nRF54H20-1_required:

Required changes
****************

The following changes are mandatory to make your application work in the same way as in previous releases.

nRF54H20
========

This section describes the changes specific to the nRF54H20 SoC and DK support in the |NCS|.

DK compatibility
----------------

.. toggle::

   * The |NCS| v2.9.0-nRF54H20-1 is compatible only with the Engineering C - v0.9.0 and later revisions of the nRF54H20 DK, PCA10175.
     Check the version number on your DK's sticker to verify its compatibility with the |NCS|.

Dependencies
------------

The following required dependencies for the nRF54H20 SoC and DK have been updated.

SDK and toolchain
+++++++++++++++++

.. toggle::

  * To update the SDK and the toolchain, do the following:

   1. Open Toolchain Manager in nRF Connect for Desktop.
   #. Click :guilabel:`SETTINGS` in the navigation bar to specify where you want to install the |NCS|.
   #. In :guilabel:`SDK ENVIRONMENTS`, click the :guilabel:`Install` button next to the |NCS| version |release|.

nRF Util
++++++++

.. toggle::

  * ``nrfutil`` has been updated to v7.13.0.

    Install nRF Util v7.13.0 as follows:

      1. Download the nRF Util executable file from the `nRF Util development tool`_ product page.
      #. Add nRF Util to the system path on Linux and macOS, or environment variables on Windows, to run it from anywhere on the system.
         On Linux and macOS, use one of the following options:

         * Add nRF Util's directory to the system path.
         * Move the file to a directory in the system path.

      #. On macOS and Linux, give ``nrfutil`` execute permissions by typing ``chmod +x nrfutil`` in a terminal or using a file browser.
         This is typically a checkbox found under file properties.
      #. On macOS, to run the nRF Util executable, you need to allow it in the system settings.
      #. Verify the version of the nRF Util installation on your machine by running the following command::

            nrfutil --version

      #. If your version is below 7.13.0, run the following command to update nRF Util::

            nrfutil self-upgrade

         For more information, see the `nRF Util`_ documentation.

nRF Util device
+++++++++++++++

.. toggle::

  * nRF Util ``device`` command has been updated to v2.7.15.

    Install the nRF Util ``device`` command v2.7.15 as follows::

       nrfutil install device=2.7.15 --force

    For more information, consult the `nRF Util`_ documentation.

nRF Util trace
++++++++++++++

.. toggle::

  * nRF Util ``trace`` command has been updated to v3.1.0.

    Install the nRF Util ``trace`` command v3.1.0 as follows::

       nrfutil install trace=3.1.0 --force

    For more information, consult the `nRF Util`_ documentation.

nRF Util suit
+++++++++++++

.. toggle::

  * nRF Util ``suit`` command has been updated to v0.9.0.

    Install the nRF Util ``suit`` command v0.9.0 as follows::

       nrfutil install suit=0.9.0 --force

    For more information, consult the `nRF Util`_ documentation.

nRF54H20 SoC binaries
+++++++++++++++++++++

.. toggle::

  * The *nRF54H20 SoC binaries* bundle has been updated to version 0.9.2.

    .. note::
       The nRF54H20 SoC binaries only support specific versions of the |NCS| and do not support rollbacks to a previous version.
       Upgrading the nRF54H20 SoC binaries on your development kit might break the DK's compatibility with applications developed for previous versions of the |NCS|.
       For more information, see :ref:`abi_compatibility`.

    To update the SoC binaries bundle of your development kit while in Root of Trust, do the following:

    1. Download the `nRF54H20 SoC binaries v0.9.2`_.

       .. note::
          On macOS, ensure that the ZIP file is not unpacked automatically upon download.

    #. Purge the device as follows::

          nrfutil device recover --core Application --serial-number <serial_number>
          nrfutil device recover --core Network --serial-number <serial_number>
          nrfutil device reset --reset-kind RESET_PIN --serial-number <serial_number>

    #. Run ``west update``.
    #. Move the correct :file:`.zip` bundle to a folder of your choice, then run nRF Util to program the binaries using the following command::

          nrfutil device x-suit-dfu --firmware nrf54h20_soc_binaries_v0.9.2.zip --serial-number <serial_number>

    #. Purge the device again as follows::

          nrfutil device recover --core Application --serial-number <serial_number>
          nrfutil device recover --core Network --serial-number <serial_number>
          nrfutil device reset --reset-kind RESET_PIN --serial-number <serial_number>

Application development
-----------------------

The following are the changes required to migrate your applications to the |NCS| v2.9.0-nRF54H20-1.

ZMS backend
+++++++++++

The support for the backend for Zephyr Memory Settings (ZMS) has been updated.
This update does not affect the ZMS Zephyr API.

Deprecated ZMS Kconfigs
+++++++++++++++++++++++

The following ZMS Kconfig options are deprecated:

   * ``CONFIG_SETTINGS_ZMS_NAME_CACHE``
   * ``CONFIG_SETTINGS_ZMS_NAME_CACHE_SIZE``
   * ``CONFIG_ZMS_LOOKUP_CACHE_FOR_SETTINGS``

New ZMS backend defaults
++++++++++++++++++++++++

The ZMS settings backend now defaults to using the entire available storage partition.
To customize the partition size used, complete the following steps:

1. Set ``CONFIG_SETTINGS_ZMS_CUSTOM_SECTOR_COUNT`` to ``y``.
#. Set the number of sectors used by the ZMS settings backend using the ``CONFIG_SETTINGS_ZMS_SECTOR_COUNT`` Kconfig option.

For example::

   CONFIG_SETTINGS_ZMS_CUSTOM_SECTOR_COUNT=y
   CONFIG_SETTINGS_ZMS_SECTOR_COUNT=8
