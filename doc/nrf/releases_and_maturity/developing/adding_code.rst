.. _dm_adding_code:

Adding your own code
####################

.. contents::
   :local:
   :depth: 2

To maintain an application that uses the |NCS|, you should use the same tools that Nordic Semiconductor employs to develop it.
This section describes suggested user workflows to develop and maintain an application based on the |NCS|.

See :ref:`dm_code_base` for detailed information on how the |NCS| repositories are organized.

.. _dm_user_workflows:

User workflows
**************

The following workflows present different ways of adding your own code to the |NCS|.
They describe the actual practicalities of developing an application that is based on the |NCS| from a version control and maintenance point of view.

Which user workflow to choose depends on the type of application, the timeframe to develop it, and the need to update the |NCS| version used.

All workflows are described under the following basic assumptions:

* One or more applications are to be developed using the |NCS|.
* Additional board definitions might be required by the user.
* Additional libraries might be required by the user.
* The term *application* refers to the application code and any board definitions and libraries it requires.
* The application will require updates of the |NCS| revision.

.. _dm_workflow_1:

Workflow 1: Avoid Git and west
==============================

If you have your own version control tools, you might want to simply not use Git or west at all, and instead rely on your own toolset.
In such case, you must obtain a copy of the |NCS| on your file system and then manage the source code of both the SDK and your application yourself.

To obtain a copy of the |NCS|, you can use one of the following methods:

* :ref:`Install the nRF Connect SDK <install_ncs>` using the |nRFVSC| and download the desired |NCS| version.
  This corresponds to steps 2 and 3 on the linked page.
* Follow the instructions in the :ref:`dm-wf-get-ncs` section of the documentation.
  This requires you to install Git and west, but you can then ignore them from that point onwards.

If you need to update the copy of the |NCS| you are working with, you can obtain the source code again, or, if you have kept the original set of repositories, update it in the Toolchain Manager or by following steps in :ref:`dm-wf-update-ncs`.

Once you have obtained a copy of the |NCS| source code, which is self-contained in a single folder, you can then proceed to manage that code in any way you see fit.

Unless you take some :ref:`additional steps <zephyr:no-west>`, west itself must still be installed in order to build applications.

.. _dm_workflow_2:

Workflow 2: Freestanding application repository
===============================================

Another approach to maintaining your application is to completely decouple it from the |NCS| repositories and instead host it wherever you prefer - in Git, another version control system, or simply on your hard drive.
This is typically also known as a :ref:`freestanding <zephyr:zephyr-app-types>` or "out-of-tree" application, meaning that the application, board definitions, and any other libraries are actually outside any of the repositories provided by the |NCS| and can be placed anywhere at all.

As long as you do not need to make changes to any of the repositories of the |NCS|, you can use either the |nRFVSC| or command line to get the source code and update it later, as described in :ref:`install_ncs`.
This allows you to manage your application separately, whether it is inside or outside the top folder of the |NCS|.
If you choose to have your application outside of the folder hierarchy of the |NCS|, the build system will find the location of the SDK through the :makevar:`ZEPHYR_BASE` environment variable, which is set either through a script or manually in an IDE.
More information about application development and the |NCS| build and configuration system can be found in the :ref:`app_build_system` documentation section.

The drawback with this approach is that any changes you make to the set of |NCS| repositories are not directly trackable using Git, since you do not have any of the |NCS| repositories forked.
If you are tracking the main branch of the |NCS|, you can instead send the changes you require to the official repositories as Pull Requests, so that they are incorporated into the codebase.

.. _dm_workflow_3:

Workflow 3: Application in a fork of sdk-nrf
============================================

Forking the `sdk-nrf`_ repository and adding the application to it is another valid option to develop and maintain your application.
This approach also allows you to fork additional |NCS| repositories and point to those.
This can be useful if you have to make changes to those repositories beyond adding your own application to the manifest repository.

In order to use this approach, you first need to get the source code (either through the :ref:`Toolchain Manager <auto_installation_tcm_setup>` or :ref:`command line <dm-wf-get-ncs>`), and then :ref:`fork the sdk-nrf repository <dm-wf-fork>`.
Once you have your own fork, you can start adding your application to your fork's tree and push it to your own Git server.
Every time you want to update the revision of the |NCS| that you want to use as a basis for your application, you must follow the :ref:`instructions to update <dm-wf-update-ncs>` on your own fork of ``sdk-nrf``.

If you have changes in additional repositories beyond `sdk-nrf`_ itself, you can point to your own forks of those in the :file:`west.yml` included in your fork.

.. _dm_workflow_4:

Workflow 4: Workspace application repository (recommended)
==========================================================

An additional possibility is to take advantage of west to manage your own set of repositories.
This workflow is particularly beneficial if your application is split among multiple repositories or, just like in the previous workflow, if you want to make changes to one or more |NCS| repositories, since it allows you to define the full set of repositories yourself.

In order to implement this approach, you first need to create a manifest repository of your own, which just means a repository that contains a :file:`west.yml` manifest file in its root.
Next you must populate the manifest file with the list of repositories and their revisions.

You have two different options to create your own repository, as discussed in the following subsections:

* `Recommended: Using the example application repository`_
* `Creating your own manifest repository from scratch`_

Once you have your new manifest repository hosted online, you can use it with west just like you use the `sdk-nrf`_  repository when getting the source code (either through the :ref:`Toolchain Manager <auto_installation_tcm_setup>` or :ref:`command line <dm-wf-get-ncs>`) and later :ref:`updating it from command line <dm-wf-update-ncs>`.
You just need to replace ``sdk-nrf`` and ``nrf`` with the repository name and path you have chosen for your manifest repository (*your-name/your-application* and *your-app-workspace*, respectively), as shown in the following code:

.. parsed-literal::
   :class: highlight

   west init -m https:\ //github.com/*your-name/your-application* *your-app-worskpace*
   cd *your-app-workspace*
   west update

After that, to modify the |NCS| version associated with your app, change the ``revision`` value in the manifest file to the `sdk-nrf`_ Git tag, SHA, or the branch you want to use, save the file, and run ``west update``.
See :ref:`zephyr:west-basics` for more details.

.. _dm_workflow_4_example_repo:

Recommended: Using the example application repository
-----------------------------------------------------

.. example_app_start

Nordic Semiconductor maintains a templated example repository as a reference for users that choose this workflow, `ncs-example-application`_.
This repository showcases the following features of both the |NCS| and Zephyr:

* Basic :ref:`Zephyr application <zephyr:application>` skeleton
* :ref:`Zephyr workspace applications <zephyr:zephyr-workspace-app>`
* :ref:`West T2 topology <zephyr:west-t2>`
* :ref:`Custom boards <zephyr:board_porting_guide>`
* Custom :ref:`devicetree bindings <zephyr:dt-bindings>`
* Out-of-tree :ref:`drivers <zephyr:device_model_api>`
* Out-of-tree libraries
* Example CI configuration (using Github Actions)
* Custom :ref:`west extension <zephyr:west-extensions>`

.. example_app_end

It is highly recommended to use this templated repository as a starting point for your own application manifest repository.
Click the :guilabel:`Use this template` button on the GitHub web user interface.
Once you have your own copy of the repository, you can then modify it.
You can add your own projects to the manifest file, as well as your own boards, drivers, and libraries as required by your application.

The `ncs-example-application`_ repository is tagged every time a new release of the |NCS| is launched (starting with v2.3.0), using the same version number.
This allows you to select the tag that matches the version of the |NCS| you intend to use.

.. _dm_workflow_4_own_repo:

Creating your own manifest repository from scratch
--------------------------------------------------

If you decide not to use the example application repository as a starting point, you can start from scratch to create your own manifest repository that imports the |NCS|.

To start the process, ``import`` `sdk-nrf`_ in your own :file:`west.yml`, using the :ref:`manifest imports <zephyr:west-manifest-import>` feature of west.
This is demonstrated by the following code, that would be placed somewhere in your repository (typically in :file:`root/west.yml`):

.. code-block:: yaml

   # Example application-specific west.yml, using manifest imports.
   manifest:
     remotes:
       - name: ncs
         url-base: https://github.com/nrfconnect
     projects:
       - name: nrf
         repo-path: sdk-nrf
         remote: ncs
         revision: v2.5.0
         import: true
     self:
       path: application

Importing the ``sdk-nrf`` project in your :file:`west.yml` also results in the addition of all the |NCS| projects, including those imported from Zephyr, into your workspace.

Then, make the following changes if required:

* Point the entries of any |NCS| repositories that you have forked to your fork and fork revision, by adding them to the ``projects`` list using a new remote.
* Add any entries for repositories that you need that are not part of the |NCS|.

For example:

.. code-block:: yaml

   # Example your-application/west.yml, using manifest imports, with
   # an nRF Connect SDK fork and a separate module
   manifest:
     remotes:
       - name: ncs
         url-base: https://github.com/nrfconnect
       - name: your-remote
         url-base: https://github.com/your-name
     projects:
       - name: nrf
         remote: ncs
         revision: v2.5.0
         import: true
       # Example for how to override a repository in the nRF Connect SDK with your own:
       - name: mcuboot
         remote: your-remote
         revision: your-mcuboot-fork-SHA-or-branch
       # Example for how to add a repository not in nRF Connect SDK:
       - name: your-custom-library
         remote: your-remote
         revision: your-library-SHA-or-branch
     self:
       path: application

The variable values starting with *your-* in the above code block are just examples and you can replace them as needed.
The above example includes a fork of the ``mcuboot`` project, but you can fork any project in :file:`nrf/west.yml`.
