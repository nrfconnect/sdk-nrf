.. _gs_updating:

Updating repositories and tools
###############################

.. contents::
   :local:
   :depth: 2

After you install the |NCS|, regularly check for updates to repositories and tools.
The west tool is updated regularly, like any other :ref:`required Python dependency package <gs_recommended_versions>`.

You might also want to switch to a newer release or check out the latest state of development.
However, if you work with a specific release of the |NCS|, you do not need to update your repositories, because the release will not change.
For an overview of changes in the latest releases, see :ref:`release_notes`.

.. _gs_updating_repos:

Updating the repositories
*************************

|method_note|

Updating in |VSC|
=================

The |nRFVSC| lets you update the associated |NCS| repositories within the :guilabel:`Source Control View`.
For detailed instructions, see the `west module management`_ page in the extension's documentation.

Updating in Toolchain Manager
=============================

.. note::
   The SDK versions available in Toolchain Manager are for specific releases.
   Updating the SDK repositories in Toolchain Manager might therefore be required only in exceptional situations.

If you installed the |NCS| automatically using the :ref:`Toolchain Manager <gs_app_tcm>`, complete the following steps to update the repositories in Toolchain Manager:

1. Open the Toolchain Manager application in nRF Connect for Desktop.
#. Click the button with the arrow pointing down next to the installed |NCS| version to expand the drop-down menu with options.

   .. figure:: images/gs-assistant_tm_dropdown.png
      :alt: The Toolchain Manager dropdown menu for the installed nRF Connect SDK version, cropped

      The Toolchain Manager dropdown menu options

#. In the drop-down menu, click :guilabel:`Update SDK`.

Updating from command line
==========================

To manage the ``nrf`` repository (the manifest repository) from command line, use Git.
To make sure that you have the latest changes, run ``git fetch origin`` to :ref:`fetch the latest code <dm-wf-update-ncs>` from the `sdk-nrf`_ repository.
Checking out a branch or tag in the ``nrf`` repository gives you a different version of the manifest file.
Running ``west update`` updates the project repositories to the state specified in this manifest file.

.. include:: installing.rst
   :start-after: west-error-start
   :end-before: west-error-end

.. _gs_updating_repos_examples:

Examples of commands
====================

To switch to release |release| of the |NCS|, enter the following commands in the ``ncs/nrf`` directory:

.. parsed-literal::
   :class: highlight

   git fetch origin
   git checkout |release|
   west update

To update to a particular revision (SHA), make sure that you have that particular revision locally before you check it out (by running ``git fetch origin``)::

   git fetch origin
   git checkout 224bee9055d986fe2677149b8cbda0ff10650a6e
   west update

To switch to the latest state of development, enter the following commands::

   git fetch origin
   git checkout origin/main
   west update

.. note::
   Run ``west update`` every time you change or modify the current working branch (for example, when you pull, rebase, or check out a different branch).
   This will bring the project repositories to the matching revision defined by the manifest file.

.. _gs_updating_vsc:

Updating |nRFVSC|
*****************

|VSC| checks for extension updates and automatically installs them when they are available.
After an extension is updated, |VSC| prompts you to reload the application.

If you disabled automatic updates:

1. Open the :guilabel:`Extensions` tab and locate the |nRFVSC|.

#. The :guilabel:`Update` button appears when an update is available.
   Click the button to install the update.

.. _toolchain_update:

Updating toolchain in Toolchain Manager
***************************************

If you installed the |NCS| automatically using the :ref:`Toolchain Manager <gs_app_tcm>`, complete the following steps to update the toolchain in Toolchain Manager:

1. Open the Toolchain Manager application in nRF Connect for Desktop.
#. Click the button with the arrow pointing down next to the installed |NCS| version to expand the drop-down menu with options.

   .. figure:: images/gs-assistant_tm_dropdown.png
      :alt: The Toolchain Manager dropdown menu for the installed nRF Connect SDK version, cropped

      The Toolchain Manager dropdown menu options

#. In the drop-down menu, click :guilabel:`Update toolchain`.

.. _west_update:

Updating west from command line
*******************************

To update west to the latest available version in the PyPi repository, run the following command in the command window:

.. tabs::

   .. group-tab:: Windows

      .. parsed-literal::
         :class: highlight

         pip3 install -U west

   .. group-tab:: Linux

      .. parsed-literal::
         :class: highlight

         pip3 install --user -U west

   .. group-tab:: macOS

      .. parsed-literal::
         :class: highlight

         pip3 install -U west
..

.. _repo_move:

Pointing the repositories to the right remotes after they were moved
********************************************************************

Before the |NCS| version 1.3.0, the Git repositories were moved from the NordicPlayground GitHub organization to the nrfconnect organization.
They were also renamed, replacing the ``fw-nrfconnect-`` prefix with ``sdk-``.

The full list of repositories with their old and new URLs is in the following table:

.. list-table::
   :header-rows: 1

   * - Old URL
     - New URL
     - File system path

   * - https:\ //github.com/NordicPlayground/fw-nrfconnect-nrf
     - https://github.com/nrfconnect/sdk-nrf
     - :file:`ncs/nrf`

   * - https:\ //github.com/NordicPlayground/fw-nrfconnect-zephyr
     - https://github.com/nrfconnect/sdk-zephyr
     - :file:`ncs/zephyr`

   * - https:\ //github.com/NordicPlayground/fw-nrfconnect-mcuboot
     - https://github.com/nrfconnect/sdk-mcuboot
     - :file:`ncs/bootloader/mcuboot`

   * - https:\ //github.com/NordicPlayground/fw-nrfconnect-mcumgr
     - https://github.com/nrfconnect/sdk-mcumgr
     - :file:`ncs/modules/lib/mcumgr`

   * - https:\ //github.com/NordicPlayground/nrfxlib
     - https://github.com/nrfconnect/sdk-nrfxlib
     - :file:`ncs/nrfxlib`


If you cloned the repositories before the move, your local repositories and forks of the |NCS| repositories are automatically be redirected to the new ones.
However, you should point them directly to their new locations as described in this section.

To change the remotes, complete the following steps:

1. Rename any personal forks that you have of the |NCS| repositories to their new names:

   a. Visit your personal fork in a browser.
      For example, to rename the fw-nrfconnect-nrf repository, access your fork on GitHub (``https://github.com/<username>/fw-nrfconnect-nrf``, where *<username>* is your GitHub account name).
   #. Switch to the :guilabel:`Settings` tab and edit the name in the **Repository name** field to ``sdk-nrf``.

#. Rename the remotes:

   a. Go to your local copy of each of the repositories listed in the preceding table.
   #. Enter the following command:

      .. parsed-literal::
        :class: highlight

        git remote set-url *remote_name* *new_url*

      In this command, replace *remote_name* with the name of your remote (for example, ``origin`` for your fork or ``ncs`` for the upstream repository) and *new_url* with the URL of your fork or the new URL from the preceding table.
      If you are not sure about the remotes that are configured in your local repository, enter ``git remote -v`` to see your remotes.

For example, to point your existing fw-nrfconnect-nrf clone to its new URL, enter the following command:

.. code-block::

   cd ncs/nrf
   git remote set-url origin https://github.com/nrfconnect/sdk-nrf

Similarly, to point your existing fw-nrfconnect-zephyr clone to the new URL, enter the following command:

.. code-block::

   cd  ncs/zephyr
   git remote set-url ncs https://github.com/nrfconnect/sdk-zephyr

For more information about remotes and how to handle them, see :ref:`dm-wf-fork`.

.. |method_note| replace:: Use the method corresponding to the way you installed the |NCS|, as described in the following sections.
