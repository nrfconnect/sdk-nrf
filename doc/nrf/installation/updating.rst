.. _gs_updating:
.. _repo_move:
.. _updating:

Updating repositories and tools
###############################

.. contents::
   :local:
   :depth: 2

After you install the |NCS|, regularly check for updates to repositories and tools.
The west tool is updated regularly, like any other :ref:`required Python dependency package <requirements>`.

You might also want to switch to a newer release or check out the latest state of development.
However, if you work with a specific release of the |NCS|, you do not need to update your repositories, because the release will not change.
For an overview of changes in the latest releases, see :ref:`release_notes`.

.. _gs_updating_repos:
.. _gs_updating_repos_examples:
.. _updating_repos_examples:
.. _updating_repos:

Updating the repositories
*************************

Use the method corresponding to the way you installed the |NCS|, as described in the following sections.

.. tabs::

   .. group-tab:: nRF Connect for VS Code

      The |nRFVSC| lets you update the associated |NCS| repositories within the :guilabel:`Source Control View`.
      For detailed instructions, see the `west module management`_ page in the extension's documentation.

      You can also change the SDK or toolchain in the |nRFVSC| to a new one.
      Complete the steps listed on the `How to change SDK and toolchain versions`_ page in the extension's documentation.

   .. group-tab:: Command line

      To manage the ``nrf`` repository (the manifest repository) from command line, use Git.

      Use the following set of commands:

      * ``git fetch origin`` - To :ref:`fetch the latest code <dm-wf-update-ncs>` from the `sdk-nrf`_ repository and make sure that you have the latest changes.
      * ``git checkout`` - If you want to check out a branch or tag in the ``nrf`` repository.
        This gives you a different version of the manifest file.
      * ``west update`` - To update the project repositories to the state specified in this manifest file.
        It is a good practice to run ``west update`` every time you change or modify the current working branch (for example, when you pull, rebase, or check out a different branch).

      .. include:: install_ncs.rst
         :start-after: west-error-start
         :end-before: west-error-end

      **Example: Switching to a release**

         .. toggle::

            To switch to release |release| of the |NCS|, enter the following commands in the ``ncs/nrf`` directory:

            .. parsed-literal::
               :class: highlight

               git fetch origin
               git checkout |release|
               west update

      **Example: Switching to a revision (SHA, branch, or tag)**

         .. toggle::

            To update to a particular revision, make sure that you have that particular revision on your local file system before you check it out by running ``git fetch origin``:

            .. code-block:: console

               git fetch origin
               git checkout *next_revision*
               west update

            In this case, *next_revision* can be either a SHA (for example, ``224bee9055d986fe2677149b8cbda0ff10650a6e``), a branch, or a tag name.

      **Example: Switching to the latest state of development (branch)**

         .. toggle::

            To switch to the ``main`` branch that includes the latest state of development, enter the following commands:

            .. code-block:: console

               git fetch origin
               git checkout origin/main
               west update

.. _vsc_update:

Updating |nRFVSC|
*****************

|VSC| checks for extension updates and automatically installs them when they are available.
After an extension is updated, |VSC| prompts you to reload the application.

If you disabled automatic updates:

1. Open the :guilabel:`Extensions` tab and locate the |nRFVSC|.

#. The :guilabel:`Update` button appears when an update is available.
   Click the button to install the update.

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

.. _toolchain_update:

Updating in Toolchain Manager
*****************************

.. note::
   Toolchain Manager is only recommended for the |NCS| v1.9.x and earlier.
   For newer releases, use the |nRFVSC| or the command line tools to manage SDK and toolchain.
   See :ref:`install_ncs` for details.
   For migration instructions to the |nRFVSC|, see `How to install the extension`_ in the extension documentation.

Updating toolchain in Toolchain Manager
=======================================

If you installed the |NCS| automatically using the :ref:`Toolchain Manager <gs_assistant>`, complete the following steps to update the toolchain in Toolchain Manager:

1. Open the Toolchain Manager application in nRF Connect for Desktop.
#. Click the button with the arrow pointing down next to the installed |NCS| version to expand the drop-down menu with options.

   .. figure:: images/gs-assistant_tm_dropdown.png
      :alt: The Toolchain Manager dropdown menu for the installed nRF Connect SDK version, cropped

      The Toolchain Manager dropdown menu options

#. In the drop-down menu, click :guilabel:`Update toolchain`.

Updating SDK in Toolchain Manager
=================================

.. note::
   The SDK versions available in Toolchain Manager are for specific releases.
   Updating the SDK repositories in Toolchain Manager might therefore be required only in exceptional situations.

If you installed the |NCS| automatically using the :ref:`Toolchain Manager <gs_assistant>`, complete the following steps to update the repositories in Toolchain Manager:

1. Open the Toolchain Manager application in nRF Connect for Desktop.
#. Click the button with the arrow pointing down next to the installed |NCS| version to expand the drop-down menu with options.

   .. figure:: images/gs-assistant_tm_dropdown.png
      :alt: The Toolchain Manager dropdown menu for the installed nRF Connect SDK version, cropped

      The Toolchain Manager dropdown menu options

#. In the drop-down menu, click :guilabel:`Update SDK`.
