.. _ncs_release_notes_2799_cs1:

|NCS| v2.7.99-cs1 Release Notes
###############################

.. contents::
   :local:
   :depth: 3

|NCS| v2.7.99-cs1 is a **customer sampling** release, tailored exclusively for participants in the nRF54H20 SoC customer sampling program.
Do not use this release of |NCS| if you are not participating in the program.

The release is not part of the regular release cycle and is specifically designed to support early adopters working with the nRF54H20 SoC.
It is intended to provide early access to the latest developments for nRF54H20 SoC, enabling participants to experiment with new features and provide feedback.

The scope of this release is intentionally narrow, concentrating solely on the nRF54H20 DK.
You can use the latest stable release of |NCS| to work with other boards.

All functionality related to nRF54H20 SoC is considered experimental.

Highlights
**********

Added the following features as experimental:

* Power management with support for System On All Idle power state.
* Clock control API.
* New peripheral driver SAADC.

Release tag
***********

The release tag for the |NCS| manifest repository (|ncs_repo|) is **v2.7.99-cs1**.
Check the :file:`west.yml` file for the corresponding tags in the project repositories.

To use this release, check out the tag in the manifest repository and run ``west update``.
See :ref:`cloning_the_repositories` and :ref:`gs_updating_repos_examples` for more information.

For information on the included repositories and revisions, see Repositories and revisions for v2.7.99-cs1.

IDE and tool support
********************

`nRF Connect extension for Visual Studio Code <nRF Connect for Visual Studio Code_>`_ is the recommended IDE for |NCS| v2.7.99-cs1.
See the :ref:`installation` section for more information about supported operating systems and toolchain.

.. note::

   Because of the significant, potentially breaking changes introduced in the |NCS| v2.7.0, the support for v2.7.99-cs1 in the |nRFVSC| is experimental in the following areas:

     * sysbuild - During the extension stabilization period, manually edit and verify Kconfig option values instead of using the nRF Kconfig GUI.
     * Hardware Model v2 - During the extension stabilization period, edit devicetree files manually instead of using the Devicetree Visual Editor.
       Moreover, custom boards added in the extension will follow the Zephyr's Hardware Model v1.

   The extension users that need v2.7.99-cs1 should `switch to the pre-release version of the extension`_.

Migration notes
***************

See the `Migration notes for nRF Connect SDK v2.7.99-cs1 and the nRF54H20 DK`_ for the changes required or recommended when migrating your environment to the |NCS| v2.7.99-cs1.

Changelog
*********

The following sections provide detailed lists of changes by component.

Zephyr
======

.. NOTE TO MAINTAINERS: All the Zephyr commits in the below git commands must be handled specially after each upmerge and each nRF Connect SDK release.

The Zephyr fork in |NCS| (``sdk-zephyr``) contains all commits from the upstream Zephyr repository up to and including ``28b6861211d2d9f54411ff60357a42d84a253600``, with some |NCS| specific additions.

* Added:

  * Experimental support for Power management with support for System On All Idle power state.
  * Experimental support for a new peripheral driver SAADC.
    See the :zephyr:code-sample:`adc_sequence` and :zephyr:code-sample:`adc_dt` sample documentation for more details.
  * Experimental support for the clock control API.
    See the :file:`include/zephyr/drivers/clock_control/nrf_clock_control.h` file for more details.

For the list of upstream Zephyr commits (not including cherry-picked commits) incorporated into nRF Connect SDK since the most recent release, run the following command from the :file:`ncs/zephyr` repository (after running ``west update``):

.. code-block:: none

   git log --oneline 28b6861211 ^25e90e7bb0

For the list of |NCS| specific commits, including commits cherry-picked from upstream, run:

.. code-block:: none

   git log --oneline manifest-rev ^28b6861211

The current |NCS| main branch is based on revision ``28b6861211`` of Zephyr.

.. note::
   For possible breaking changes and changes between the latest Zephyr release and the current Zephyr version, refer to the :ref:`Zephyr release notes <zephyr_release_notes>`.

Documentation
=============

* Added:

  * A page about :ref:`ug_nrf54h20_architecture_clockman`.
  * A page about :ref:`ug_nrf54h20_architecture_pm`.
  * Documentation for :ref:`multicore_idle_test`.
  * :ref:`migration_nrf54h20_to_2.7.99-cs1`.

* Updated :ref:`ug_nrf54h` and :ref:`ug_nrf54h20_gs` guides with information about the |NCS| v2.7.99-cs1.
