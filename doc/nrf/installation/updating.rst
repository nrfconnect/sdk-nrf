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

.. _using_toolchain_environment:

Using the correct command line environment
******************************************

Whenever you update repositories and tools, make sure that you use the command line environment that is configured to work with west and the rest of the nRF Connect Toolchain and nRF Connect SDK environment.
This is also valid for commands that must be executed in the |NCS| toolchain environment or when you get the following error:

.. code-block:: console

   west is not recognized as an internal or external command

Depending on your preferred development method, you can start the correct CLI toolchain environment the following way:

.. tabs::

   .. group-tab:: nRF Connect for VS Code

      Start the nRF Connect terminal profile from the `Welcome View`_ (:guilabel:`Open terminal` action) or the `Panel View`_.
      See `How to use nRF Connect terminal profile`_ in the extension documentation for more information.

      .. note::
          Repositories and tools can be updated in the |nRFVSC| using GUI.
          See :ref:`updating_repos` below for details.

   .. group-tab:: Command line

      Use the command for your operating system with the ``--ncs-version`` corresponding to the version of the |NCS| you are working with.
      The following commands start the command line environment for the latest release (|release|):

      .. tabs::

         .. tab:: Windows

            .. parsed-literal::
               :class: highlight

               nrfutil sdk-manager toolchain launch --ncs-version |release|  --terminal

         .. tab:: Linux

            .. parsed-literal::
               :class: highlight

               nrfutil sdk-manager toolchain launch --ncs-version |release| --shell

         .. tab:: macOS

            .. parsed-literal::
               :class: highlight

               nrfutil sdk-manager toolchain launch --ncs-version |release| --shell

      ..

      You can also use other options instead of ``--ncs-version``.
      See the `sdk-manager command documentation <sdk-manager Starting and inspecting the toolchain environment_>`_ for more information.

.. _gs_updating_repos:
.. _gs_updating_repos_examples:
.. _updating_repos_examples:
.. _updating_repos:

Updating the repositories
*************************

Use the method corresponding to the way you installed the |NCS|, as described in the following sections.

.. tabs::

   .. group-tab:: nRF Connect for VS Code

      |nRFVSC| lets you update the associated |NCS| repositories within the :guilabel:`Source Control View`.
      For detailed instructions, see the `west module management`_ page in the extension's documentation.

      You can also change the SDK or toolchain in |nRFVSC| to a new one.
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

.. _migrating_project:

Migrating your project to a new SDK version
===========================================

After you updated the |NCS| repositories to the new version and you need to migrate your |NCS| project to the new version, check the available :ref:`migration_guides` for information about which components received major breaking changes and what you have to do to keep using them.

.. note::
    |migration_contact_devzone|

.. _vsc_update:

Updating |nRFVSC|
*****************

|VSC| checks for extension updates and automatically installs them when they are available.
After an extension is updated, |VSC| prompts you to reload the application.

If you disabled automatic updates:

1. Open the :guilabel:`Extensions` tab and locate the |nRFVSC|.

#. The :guilabel:`Update` button appears when an update is available.
   Click the button to install the update.

Sometimes the extension can offer pre-release versions.
In such cases, you can `switch to the pre-release version of the extension`_ to test the new features before they are released.

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
