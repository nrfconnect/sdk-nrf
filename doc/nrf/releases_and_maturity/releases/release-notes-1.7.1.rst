.. _ncs_release_notes_171:

|NCS| v1.7.1 Release Notes
##########################

.. contents::
   :local:
   :depth: 2

|NCS| delivers reference software and supporting libraries for developing low-power wireless applications with Nordic Semiconductor products.
It includes the TF-M, MCUboot and the Zephyr RTOS open source projects, which are continuously integrated and re-distributed with the SDK.

The |NCS| is where you begin building low-power wireless applications with Nordic Semiconductor nRF52, nRF53, and nRF91 Series devices.
nRF53 Series devices are now supported for production.
Wireless protocol stacks and libraries may indicate support for development or support for production for different series of devices based on verification and certification status.
See the release notes and documentation for those protocols and libraries for further information.

Highlights
**********

This minor release adds the following fixes on top of :ref:`nRF Connect SDK v1.7.0 <ncs_release_notes_170>`:

* The :ref:`bt_mesh` is now capable of qualifying with the Bluetooth® Special Interest Group (SIG).

Release tag
***********

The release tag for the |NCS| manifest repository (|ncs_repo|) is **v1.7.1**.
Check the :file:`west.yml` file for the corresponding tags in the project repositories.

To use this release, check out the tag in the manifest repository and run ``west update``.
See :ref:`cloning_the_repositories` for more information.

Supported modem firmware
************************

See `Modem firmware compatibility matrix`_ for an overview of which modem firmware versions have been tested with this version of the |NCS|.

Use the latest version of the nRF Programmer app of `nRF Connect for Desktop`_ to update the modem firmware.
See :ref:`nrf9160_gs_updating_fw_modem` for instructions.

Known issues
************

See `known issues for nRF Connect SDK v1.7.1`_ for the list of issues valid for this release.

Changelog
*********

The following sections provide detailed lists of changes by component.

nRF5
====

The following changes are relevant for the nRF52 and nRF53 Series.

Bluetooth mesh
--------------

* Updated:

  * Fixed an issue where the :ref:`bt_mesh_scene_srv_readme` target scene matched the current scene.
    The transition now completes immediately.
  * Fixed an issue where the :ref:`bt_mesh_light_temp_srv_readme` calculated the time to edge to be longer than expected.

Zephyr
======

.. NOTE TO MAINTAINERS: All the Zephyr commits in the below git commands must be handled specially after each upmerge and each NCS release.

The Zephyr fork in |NCS| (``sdk-zephyr``) contains all commits from the upstream Zephyr repository up to and including ``14f09a3b00``, plus some |NCS| specific additions.
This is the same commit ID as the one used for |NCS| :ref:`v1.7.0 <ncs_release_notes_170>`.

For a complete list of |NCS| specific commits since v1.7.0, run:

.. code-block:: none

   git log --oneline manifest-rev ^v2.6.0-rc1-ncs1
