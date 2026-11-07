.. _ncs_addons_setup:

nRF Connect SDK add-ons — workspace setup
#########################################

.. contents::
   :local:
   :depth: 2

This page describes how to obtain an add-on workspace.
It includes prerequisites, nRF Connect for VS Code instructions, and command-line west flows.
Once completed, refer to each add-on documentation for information about required hardware, optional packages (Python requirements, third-party licenses), and the first sample to build.

For higher-level overview of add-ons, see :ref:`ncs_addons_overview`.

Prerequisites
*************

Before you can integrate an add-on into your |NCS| workspace:

#. Install the nRF Connect SDK prerequisites and toolchain for the NCS version compatible with the add-on release you choose.
   You can obtain this information from the add-on's release notes or compatibility table.
   Follow :ref:`Installing the nRF Connect SDK <install_ncs>` through toolchain setup.
#. From the add-on setup page, note the Git URL, west manifest folder name (for example, ``ncs-zigbee``, ``ncs-matter-edge-ai``), and tag or branch to check out.
   The pin is also in the add-on's :file:`west.yml`.

.. _ncs_addons_setup_steps:

Initializing the workspace
**************************

In a typical core checkout, ``west config manifest.path`` is ``nrf`` and west follows the NCS manifest.
For an add-on, you either initialize west from the add-on repository or set ``manifest.path`` to the add-on folder.
West then uses the add-on's :file:`west.yml` as ``self``: pinned NCS revision, optional imports (another add-on or board support package (BSP)), and the add-on sources at a fixed path.
You can use a new workspace directory or an existing :file:`ncs` tree.

nRF Connect for VS Code
=======================

Use nRF Connect for VS Code for a new workspace or when the extension manages the NCS install.

Complete the steps in the :ref:`Creating application from add-ons <creating_add_on_index>` section of the |NCS| documentation.

Once you have created the workspace, complete any extra steps on the add-on setup page: hardware requirements, ``pip install -r …``, licenses, tools.


Command line
============

Enable the add-on manifest, then run ``west update``.

#. Choose new workspace or existing |NCS| workspace:

   .. list-table::
      :header-rows: 1
      :widths: 22 39 39

      * - Criteria
        - New workspace
        - Existing |NCS| workspace
      * - Use when
        - You do not have an :file:`ncs` checkout yet, or you want a dedicated folder for this add-on.
        - You already use a shared :file:`ncs` tree with ``manifest.path`` set to ``nrf``.
      * - How the manifest is set
        - ``west init`` from the add-on repository (remote or local clone).
        - ``git clone`` the add-on into the workspace, then ``west config manifest.path``.
      * - Not the same as
        - Cloning into an existing :file:`ncs` folder without ``west init`` there.
        - ``west init`` into a new folder when you intended to reuse :file:`ncs`.

#. Point west at the add-on:

   .. tabs::

      .. group-tab:: New workspace

         From the parent of ``<workspace-folder>``, initialize west from the add-on repository:

         .. code-block:: console

            west init -m https://github.com/nrfconnect/<add-on-repo> --mr <add-on-tag-or-branch> <workspace-folder>
            cd <workspace-folder>

         Replace ``<add-on-repo>``, ``<add-on-tag-or-branch>``, and ``<workspace-folder>`` from the add-on setup page or :file:`west.yml`.

         Some add-ons document ``git clone`` into ``<workspace-folder>`` followed by ``west init -l <add-on-folder>`` instead of ``west init -m``.
         That replaces the two commands above, not an extra step after them.

      .. group-tab:: Existing |NCS| workspace

         Clone the add-on into the workspace and set ``manifest.path``:

         .. code-block:: console

            cd <workspace-folder>
            git clone https://github.com/nrfconnect/<add-on-repo>
            west config manifest.path <add-on-folder>

         Use the clone URL and ``<add-on-folder>`` name from the add-on documentation (often the repository name).

#. Update west projects from the workspace folder (``cd <workspace-folder>`` if needed):

   .. code-block:: console

      west update

   If the add-on documents it, you may use a shallow fetch (same pin, less history):

   .. code-block:: console

      west update --narrow --fetch-opt=--depth=1

#. If the add-on's :file:`west.yml` imports another add-on, keep ``manifest.path`` on the top-level add-on.
   Do not point west at the imported repository unless its documentation says so.
   The ``west update`` command already pulls imported projects from the top-level manifest.

#. Optional — switch a shared workspace back to core NCS when you need the ``nrf`` manifest again (main-tree work, ``sdk-nrf`` contribution, another add-on or NCS line, tooling that assumes ``manifest.path`` is ``nrf``).
   Skip this step if the folder is add-on-only or you use one workspace per add-on.

   .. code-block:: console

      west config manifest.path nrf
      west update
      west config manifest.path

   The last command should print ``nrf``.

Verify and continue on the add-on site
**************************************

Workspace setup is complete when ``west update`` succeeds and ``west config manifest.path`` shows the add-on folder (or ``nrf`` after an optional switch-back).

On the add-on documentation:

#. Complete hardware and any add-on-specific installation steps.
#. Open a reference sample for your board; build and flash with the same |NCS| flow (``west build``, ``west flash``, sysbuild when required).
   Applications, Kconfig, devicetree, and :file:`sample.yaml` work like core NCS samples in the add-on tree.

If a build fails, match add-on version, pinned NCS revision, and toolchain to the release notes compatibility table before changing ``manifest.path`` or tool versions.

Related material
****************

* :ref:`ncs_addons_overview` — Definition, when to use add-ons, documentation layout.
* :ref:`ncs_addons_coverage` — What capabilities are delivered as add-ons.
* `nRF Connect SDK Add-on Index <nRF Connect SDK Add-ons_>`_ — Select an add-on and version.
