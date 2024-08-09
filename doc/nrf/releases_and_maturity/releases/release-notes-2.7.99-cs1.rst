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

TBD

Release tag
***********

The release tag for the |NCS| manifest repository (|ncs_repo|) is **v2.7.99-cs1**.
Check the :file:`west.yml` file for the corresponding tags in the project repositories.

To use this release, check out the tag in the manifest repository and run ``west update``.
See :ref:`cloning_the_repositories` and :ref:`gs_updating_repos_examples` for more information.

For information on the included repositories and revisions, see `Repositories and revisions for v2.7.99-cs1`_.

IDE and tool support
********************

`nRF Connect extension for Visual Studio Code <nRF Connect for Visual Studio Code_>`_ is the recommended IDE for |NCS| v2.7.0.
See the :ref:`installation` section for more information about supported operating systems and toolchain.

.. note::

   Because of the significant, potentially breaking changes introduced in the |NCS| v2.7.0, the support for v2.7.99-cs1 in the |nRFVSC| is experimental in the following areas:

     * sysbuild - During the extension stabilization period, manually edit and verify Kconfig option values instead of using the nRF Kconfig GUI.
     * Hardware Model v2 - During the extension stabilization period, edit devicetree files manually instead of using the Devicetree Visual Editor.
       Moreover, custom boards added in the extension will follow the Zephyr's Hardware Model v1.

   The extension users that need v2.7.99-cs1 should `switch to the pre-release version of the extension`_.

Known issues
************

For the list of issues valid for this release, navigate to `known issues page on the main branch`_ and select ``v2.7.0`` from the dropdown list.

Migration notes
***************

See the :ref:`migration_guides` page for a set of miration guides specifically tailored for the nRF54H20 customer sampling paritipants.

.. _ncs_release_notes_2799_cs1_changelog:

Changelog
*********

The following sections provide detailed lists of changes by component.

TBD
