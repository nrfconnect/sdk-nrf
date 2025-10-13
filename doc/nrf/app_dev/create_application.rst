.. _create_application:

Creating an application
#######################

.. contents::
   :local:
   :depth: 2

The process for creating an application in the |NCS| follows Zephyr's :ref:`Application Development principles <zephyr:application>` and the |NCS|'s :ref:`dev-model`.
The following sections provide basic information based on these two sources.

.. _create_application_structure:

Application structure
*********************

Here are the files that most of the Zephyr and |NCS| applications include in its :file:`<app>` directory:

.. ncs-include:: develop/application/index.rst
   :docset: zephyr
   :start-after: Here are the files in a simple Zephyr application:
   :end-before: These contents are:

You can read more about each of these files in Zephyr's :ref:`Application Development overview <zephyr:application>`.

.. _create_application_types:

Application types
*****************

Based on where the :file:`<app>` directory is located, you will usually work with one of the following types of applications in the |NCS|: repository, workspace, or freestanding.
These types are adopted from :ref:`Zephyr application types <zephyr:zephyr-app-types>`.

.. _create_application_types_repository:

Repository application
======================

This kind of application is located inside the |NCS| source code (:ref:`SDK repositories <dm_repo_types>`, including downstream forks of upstream projects).
In the following example, the :ref:`Bluetooth Peripheral UART <peripheral_uart>` sample is an |NCS| repository application and its location in the source code structure serves as the :file:`<app>` directory:

.. code-block:: none

   <home>/
   ├─── .west/
   ├─── bootloader/
   ├─── mbedtls/
   ├─── modules/
   ├─── nrf/
   │    ├─── applications/
   │    ├─── ...
   │    └─── samples/
   │       ├─── ...
   │       ├─── bluetooth/
   │       │  ├── ...
   │       │  └── peripheral_uart/   <--- <app> directory
   │       └─── ...
   ├─── nrfxlib/
   ├─── test/
   ├─── tools/
   └─── zephyr/

This type of application uses the default |NCS| settings and configuration, which might differ from the corresponding upstream configuration.
For example, a notable difference is that when building this type of applications, :ref:`sysbuild is enabled by default <sysbuild_enabled_ncs>`.

This application type is suitable for the following development cases:

* You want to test the solution provided by the |NCS| out-of-the-box.

For more information about applications placed inside the |NCS| source code, see the :ref:`workflow 3 on the development model page <dm_workflow_3>`.

.. _create_application_types_workspace:

Workspace application
=====================

This kind of application is located inside a west workspace, but outside of the repositories of the SDK.
The application placed in a workspace uses its own copy of the |NCS|.
It specifies the |NCS| version through the :file:`west.yml` `west manifest file`_, which is located in the application :file:`<app>` directory.

With this kind of application, the workspace has the following structure:

.. code-block:: none

   <home>/
   └─── <west-workspace>/
      ├─── .west/
      ├─── nrf/
      ├─── zephyr/
      ├─── ...
      └─── <app>/
         ├── src/
         ├── ...
         └── west.yml

This application type is suitable for the following development cases:

* You want to take advantage of west to manage your own set of repositories.
* You want to make changes to one or more of the repositories of the |NCS| when working on the application.
* You want to develop a project that involves more than one board target, for example using a mesh networking protocol like :ref:`ug_matter` or :ref:`ug_bt_mesh`.
* You want to run a big project that lets you develop most features without having to patch the |NCS| tree, for example with out-of-tree boards, drivers, or SoCs.
* You want to use out-of-tree applications from the `nRF Connect SDK Add-ons`_ index.
* You want to set up Continuous Integration (CI) for your application and explicitly define all dependencies (such as the |NCS| version) in the application's west manifest file.

For more information about applications placed in workspace in the |NCS|, see the :ref:`workflow 4 on the development model page <dm_workflow_4>`.

.. _create_application_types_freestanding:

Freestanding application
========================

This kind of application is handled separately from the |NCS|.
It is located out-of-tree, that is outside of a west workspace, and is not using the `west manifest file`_ to specify the SDK version.
The build system will find the location of the SDK through the :makevar:`ZEPHYR_BASE` environment variable, which is set either through a script or manually in an IDE.

With this kind of application, the workspace has the following structure:

.. code-block:: none

   <home>/
   ├─── <west-workspace>/
   │  ├─── .west/
   │  ├─── nrf/
   │  ├─── zephyr/
   │  └─── ...
   └─── <app>/
      ├─── src/
      └─── ...

This application type is suitable for the following development cases:

* You prefer to use one copy of the |NCS| when working on one or more applications because of limited bandwidth.
* You want to do quick prototyping and the results might be later deleted or migrated to an :ref:`application in a workspace <create_application_types_workspace>`.

For more information about freestanding applications in the |NCS|, see the :ref:`workflow 2 on the development model page <dm_workflow_2>`.

Creating an |NCS| application
*****************************

The process for creating an |NCS| application depends on the development environment.
Using |nRFVSC| is the recommended method.

.. note::
     No steps are provided for the creation of :ref:`repository applications <create_application_types_repository>`.
     Creating repository applications is not recommended, as placing any application in the |NCS| source file structure can corrupt the SDK installation.

.. _creating_vsc:

Creating application in |nRFVSC|
================================

.. note::
   If you prefer, you can `start VS Code walkthrough`_ and create applications and build configurations from there.

Use the following steps depending on the application placement:

.. tabs::

   .. group-tab:: Workspace application (recommended)

      To create a workspace application in |nRFVSC|:

      1. Open |VSC|.
      #. Open |nRFVSC|.
      #. In the `Welcome View`_, click the :guilabel:`Create a new application` action.
         A quick pick menu appears.
      #. Choose one of the following options:

         * :guilabel:`Create a blank application` - This will create an application with a code structure that you need to populate from scratch.
         * :guilabel:`Copy a sample` - This will create an application from an |NCS| sample or an |NCS| application.
           If you have more than one version of the |NCS| installed, you have to choose the version from which you copy the sample or the application from.

      #. Enter the location and the name for the application.
         The location will be the *<west-workspace>/* directory mentioned in the :ref:`workspace application structure <create_application_types_workspace>`.
         The application creation process starts after you enter the name.
         When the application is created, a VS Code prompt appears asking you what to do with the application.
      #. Click :guilabel:`Open`.
         This will open the new application and add it to the `Applications View`_ in the extension.
         At this point, you have created a freestanding application.
      #. Add the :file:`west.yml` to create a west workspace around the application:

         a. In the :guilabel:`Welcome View`, click the :guilabel:`Manage SDKs` action.
            A quick pick menu appears.
         #. Click :guilabel:`Manage West Workspace...`.
         #. In the :guilabel:`Manage West Workspace...` action menu, click :guilabel:`Create West Workspace`.
         #. Enter a location for the :file:`west.yml` file that matches the location provided when you were creating the application.
         #. Select the SDK version for the west workspace.
            The west workspace is initialized.
         #. Click :guilabel:`Manage SDKs` > :guilabel:`Manage West Workspace` > :guilabel:`West Update` to update the workspace modules.

      You can now start :ref:`configuring and building <configuration_and_build>` the application.

      See the `extension documentation <west module management_>`_ for more information about working with workspace applications in the extension.

   .. group-tab:: Freestanding application

      To create a freestanding application in |nRFVSC|:

      1. Open |VSC|.
      #. Open |nRFVSC|.
      #. In the `Welcome View`_, click the :guilabel:`Create a new application` action.
         A quick pick menu appears.
      #. Choose one of the following options:

         * :guilabel:`Create a blank application` - This will create an application with a code structure that you need to populate from scratch.
         * :guilabel:`Copy a sample` - This will create an application from an |NCS| sample or an |NCS| application.

      #. Enter the location and the name for the application.
         The application creation process starts after you enter the name.
         When the application is created, a VS Code prompt appears.
      #. Click :guilabel:`Open`.
         This opens the new application and adds it to the `Applications View`_ in the extension.

      You can now start :ref:`configuring and building <configuration_and_build>` the application.

      See the `extension documentation <Create a new application_>`_ for more information about creating freestanding applications in the extension.

      .. note::
          You can transform your freestanding application into a workspace application at any moment by completing the step 7 under the Workspace application tab.

For more information about the differences between the applications types from the extension's perspective, see the `Applications <Application support overview_>`_ page in the extension documentation.

.. _creating_cmd:

Creating application for use with command line
==============================================

Nordic Semiconductor recommends using the example application repository to create a workspace application, but you can also create freestanding applications.

Use the following steps depending on the application type:

.. tabs::

   .. group-tab:: Workspace application (recommended)

      This recommended process for command line takes advantage of Nordic Semiconductor's example application template repository, similar to the example application used for :ref:`creating an application in Zephyr <zephyr:application>`.

      .. include:: /dev_model_and_contributions/adding_code.rst
         :start-after: example_app_start
         :end-before: example_app_end

      To create a workspace application:

      1. Open the `ncs-example-application`_ repository in your browser.
      #. Click the :guilabel:`Use this template` button on the GitHub web user interface.
         This creates your own copy of the template repository.
         In the copy of the repository, the :file:`app` directory contains the template application that you can start modifying.
      #. |open_terminal_window_with_environment|
      #. Initialize the repository with the repository name and path you have chosen for your manifest repository (*your-name/your-application* and *your-app-workspace*, respectively).
         *your-app-workspace* corresponds to :file:`ncs/` in the :ref:`workspace application structure <create_application_types_workspace>`.
         Use the following command pattern:

         .. parsed-literal::
            :class: highlight

            west init -m https:\ //github.com/*your-name/your-application* *your-app-workspace*

      #. Go to the *your-app-workspace* directory using the following command pattern:

         .. parsed-literal::
            :class: highlight

            cd *your-app-workspace*

      #. Run the following west command to download the contents of the |NCS|:

         .. code-block::
            :class: highlight

            west update

         west will clone the |NCS| contents next to the example application directory.

      For more information, see the detailed description of the :ref:`workspace application workflow <dm_workflow_4>`.

   .. group-tab:: Freestanding application

      This procedure follows Zephyr's steps for :ref:`zephyr:zephyr-creating-app-by-hand` and the :ref:`workflow 2 on the development model page <dm_workflow_2>`.
      You can copy any of the :ref:`applications` or :ref:`samples` in the |NCS| as your source code files.

      To create a freestanding application:

      .. ncs-include:: develop/application/index.rst
         :docset: zephyr
         :start-after: as a starting point is likely to be easier.
         :end-before: .. _important-build-vars:

      #. Export a :ref:`Zephyr CMake package <zephyr:cmake_pkg>` by running the following command from the main directory of your |NCS| repository copied during :ref:`installation`:

         .. code-block::
            :class: highlight

             west zephyr-export

         This allows CMake to automatically load the boilerplate code required for building |NCS| applications.

You can now start :ref:`configuring and building <configuration_and_build>` the application using the command line.

.. _creating_add_on_index:

Creating application from add-ons
=================================

You can create a :ref:`workspace application <create_application_types_workspace>` also by browsing and copying reference applications from the `nRF Connect SDK Add-ons`_ index.
The index is a collection of publicly available |NCS| supplementary components that extend the SDK's functionality.
In addition to applications, it includes drivers, libraries, and protocol implementations.

To create an application from the add-on index, complete the following steps:

.. tabs::

   .. tab:: nRF Connect for VS Code (recommended)

      Complete the following steps in |nRFVSC|:

      1. In the `Welcome View`_, click :guilabel:`Create a new application`.
      #. Select :guilabel:`Browse nRF Connect SDK Add-on Index`.
      #. Browse through the available add-ons and select one that matches your needs.
      #. Follow the creation wizard to set up your workspace application.

   .. tab:: Command line

      When creating add-on applications from the command line, follow the instructions provided in the `nRF Connect SDK Add-ons`_ repository.
      These instructions guide you through the process of copying and configuring the add-on application in your workspace.

The add-on is copied to your workspace and automatically configured with a west workspace, allowing you to start development right away.

For more information, including how to contribute your own add-on to the index, read :file:`README.md` and :file:`CONTRIBUTING.md` in the `ncs-app-index repository <ncs-app-index_>`_.
