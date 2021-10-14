.. _gs_updating:

Updating tools and repositories
###############################

.. contents::
   :local:
   :depth: 2

After you install the |NCS|, regularly check for updates to tools and repositories.
The west tool is updated regularly, like any other :ref:`required Python dependency package <gs_recommended_versions>`.

You might also want to switch to a newer release or check out the latest state of development.
However, if you work with a specific release of the |NCS|, you do not need to update your repositories, because the release will not change.
For an overview of changes in the latest releases, see :ref:`release_notes`.

.. _west_update:

Updating west
*************

To update west, run the following command in the command window:

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

This command updates west to the latest available version in the PyPi repository.

.. _gs_updating_repos:

Updating the repositories
*************************

To manage the ``nrf`` repository (the manifest repository), use Git.
To make sure that you have the latest changes, run ``git fetch origin`` to :ref:`fetch the latest code <dm-wf-update-ncs>` from the `sdk-nrf`_ repository.
Checking out a branch or tag in the ``nrf`` repository gives you a different version of the manifest file.
Running ``west update`` updates the project repositories to the state specified in this manifest file.

.. include:: gs_installing.rst
   :start-after: west-error-start
   :end-before: west-error-end

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

.. _gs_updating_ses:

Updating SEGGER Embedded Studio Nordic Edition
**********************************************

Each new release of the |NCS| might require manually updating to a newer version of the |SES| (SES) Nordic Edition.
Whenever you update to a newer release of the |NCS|, check the :ref:`gs_recommended_versions` page for the given release to see if you are using the minimum required version of the SES Nordic Edition.

.. note::
  Because of the custom |NCS| options available only in the SES Nordic Edition, notifications about newer versions of SES are disabled in the SES Nordic Edition.

.. tabs::

   .. group-tab:: Windows

      To update to the latest version of the SES Nordic Edition, use one of the following options:

      * Install the latest version of Toolchain Manager as described in :ref:`gs_assistant`.
      * Install the SES Nordic Edition manually as described in :ref:`installing_ses`.

      Then, :ref:`set up the build environment in SES <setting_up_SES>` again.

   .. group-tab:: Linux

      To update to the latest version of the SES Nordic Edition, install it manually as described in :ref:`installing_ses`.
      Then, :ref:`set up the build environment in SES <setting_up_SES>` again.

   .. group-tab:: macOS

      To update to the latest version of the SES Nordic Edition, install it manually as described in :ref:`installing_ses`.
      Then, :ref:`set up the build environment in SES <setting_up_SES>` again.
..

.. _gs_updating_ses_packages:

Updating SES packages
=====================

Updating SES Nordic Edition will not update already installed packages.
You might need to manually update the CPU support package when you cannot select a CPU as :guilabel:`Target Processor` when configuring a new project.

To update the nRF CPU Support Package after a SES update:

1. In SES, select :guilabel:`Tools` > :guilabel:`Package Manager...`.
#. Search for "nRF CPU Support Package".
#. Update the package to the latest version.

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
   #. Switch to the :guilabel:`Settings` tab and edit the name in the :guilabel:`Repository name` field to ``sdk-nrf``.

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
