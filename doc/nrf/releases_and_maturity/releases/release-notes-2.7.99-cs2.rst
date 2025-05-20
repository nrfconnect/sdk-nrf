.. _ncs_release_notes_2799_cs2:

|NCS| v2.7.99-cs2 Release Notes
###############################

.. contents::
   :local:
   :depth: 3

|NCS| v2.7.99-cs2 is a **customer sampling** release, tailored exclusively for participants in the nRF54H20 SoC customer sampling program.
Do not use this release of |NCS| if you are not participating in the program.

The release is not part of the regular release cycle and is specifically designed to support early adopters working with the nRF54H20 SoC.
It is intended to provide early access to the latest developments for nRF54H20 SoC, enabling participants to experiment with new features and provide feedback.

The scope of this release is intentionally narrow, concentrating solely on the nRF54H20 DK.
You can use the latest stable release of |NCS| to work with other boards.

All functionality related to nRF54H20 SoC is considered experimental.

Highlights
**********

Added the following features for the nRF54H20 as experimental:

* :ref:`peripheral_uart` sample:

  * Added a configuration that allows to build the sample application for the radio core.
    This allows for running Bluetooth® LE firmware and using the application core for other purposes.

* A new sample :ref:`ble_subrating`, which showcases the effect of the LE Connection Subrating feature on the duty cycle of a connection.
* Support for multi-core logging using Coresight System Trace Macrocell (STM).
  Synchronized logs from all cores are presented on a single output (UART).
  STM logging is supported in two modes:

    * Dictionary-based mode - Higher throughput, small memory footprint, :command:`nrfutil trace` decodes the output.
    * Standalone mode - Human-readable logs on UART, no host tool is required.

* Support for Embedded Trace Macrocell (ETM) used for real-time instruction and data flow tracing.
* Support for secure domain PSA crypto API.
* Power management fixes, including increased stability of System On All Idle.

Release tag
***********

The release tag for the |NCS| manifest repository (|ncs_repo|) is **v2.7.99-cs2**.
Check the :file:`west.yml` file for the corresponding tags in the project repositories.

To use this release, check out the tag in the manifest repository and run ``west update``.
See :ref:`cloning_the_repositories` and :ref:`gs_updating_repos_examples` for more information.

For information on the included repositories and revisions, see `Repositories and revisions for v2.7.99-cs2`_.

IDE and tool support
********************

`nRF Connect extension for Visual Studio Code <nRF Connect for Visual Studio Code_>`_ is the recommended IDE for |NCS| v2.7.99-cs2.
See the :ref:`installation` section for more information about supported operating systems and toolchain.

Migration notes
***************

See the `Migration notes for nRF Connect SDK v2.7.99-cs2 and the nRF54H20 DK`_ for the changes required or recommended when migrating your environment to the |NCS| v2.7.99-cs2.

Changelog
*********

The following sections provide detailed lists of changes by component.

Documentation
=============

* Added:

  * :ref:`migration_nrf54h20_to_2.7.99-cs2`.
  * A page about :ref:`ug_nrf54h20_logging`.
  * :ref:`abi_compatibility` reference page.
