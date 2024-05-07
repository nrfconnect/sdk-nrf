.. _create_application:

Create an application
#####################

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

Based on where the :file:`<app>` directory is located, you will usually work with one of two types of applications in the |NCS|: workspace or freestanding.
These types are adopted from :ref:`Zephyr application types <zephyr:zephyr-app-types>`.

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
* You want to run a big project that lets you develop most features without having to patch the |NCS| tree, for example with out-of-tree boards, drivers, SoCs, and so on.

For more information about applications placed in workspace in the |NCS|, see the :ref:`workflow 4 on the development model page <dm_workflow_4>`.

.. _create_application_types_freestanding:

Freestanding application
========================

This kind of application is handled separately from the |NCS|.
It is located out-of-tree, that is outside of a west workspace, and is not using the `west manifest file`_ to specify the SDK version.
Instead, the |NCS| version is taken from the :makevar:`ZEPHYR_BASE` environment variable.
This means that all freestanding applications will use the same |NCS| version and the same copy of the SDK.

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
Using the |nRFVSC| is the recommended method.

.. _creating_vsc:

Creating application in the |nRFVSC|
====================================

Use the following steps depending on the application placement:

.. tabs::

   .. group-tab:: Workspace application (recommended)

      To create a workspace application in the |nRFVSC|:

      1. Open |VSC|.
      #. Open the |nRFVSC|.
      #. In the :guilabel:`Welcome View`, click the :guilabel:`Create a new application` action.
         A quick pick menu appears.
      #. Choose one of the following options:

         * :guilabel:`Create a blank application` - This will create an application with a code structure that you need to populate from scratch.
         * :guilabel:`Browse a sample` - This will create an application from an |NCS| sample or an |NCS| application.

      #. Enter the location and the name for the application.
         The location will be the *<west-workspace>/* directory mentioned in the :ref:`workspace application structure <create_application_types_workspace>`.
         The application creation process starts after you enter the name.
         When the application is created, a VS Code prompt appears.
      #. Click :guilabel:`Open`.
         This opens the new application and adds it to the :guilabel:`Applications View` in the extension.
         At this point, you have created a freestanding application.
      #. Add the :file:`west.yml` to create a west workspace around the application:

         a. In the :guilabel:`Welcome View`, click the :guilabel:`Manage SDKs` action.
            A quick pick menu appears.
         #. Click :guilabel:`Create west workspace`.
         #. Enter a location for the :file:`west.yml` file that matches the location provided in step 4.
         #. Select the SDK version for the west workspace.
            The west workspace is initialized and the :guilabel:`Manage SDKs` action changes to :guilabel:`Manage west workspace`.
         #. Click :guilabel:`Manage west workspace` and select :guilabel:`West Update` button to update the workspace modules.

      You can now start :ref:`configuring and building <configuration_and_build>` the application.

      See the `extension documentation <west module management_>`_ for more information about west workspace and workspace applications in the extension.

   .. group-tab:: Freestanding application

      To create a freestanding application in the |nRFVSC|:

      1. Open |VSC|.
      #. Open the |nRFVSC|.
      #. In the :guilabel:`Welcome View`, click the :guilabel:`Create a new application` action.
         A quick pick menu appears.
      #. Choose one of the following options:

         * :guilabel:`Create a blank application` - This will create an application with a code structure that you need to populate from scratch.
         * :guilabel:`Browse a sample` - This will create an application from an |NCS| sample or an |NCS| application.

      #. Enter the location and the name for the application.
         The application creation process starts after you enter the name.
         When the application is created, a VS Code prompt appears.
      #. Click :guilabel:`Open`.
         This opens the new application and adds it to the :guilabel:`Applications View` in the extension.

      You can now start :ref:`configuring and building <configuration_and_build>` the application.

      See the `extension documentation <Create a new application_>`_ for more information about creating freestanding applications in the extension.

      .. note::
          You can transform your freestanding application into a workspace application at any moment by completing the step 7 under the Workspace application tab.

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
      #. Open a command line terminal.
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
