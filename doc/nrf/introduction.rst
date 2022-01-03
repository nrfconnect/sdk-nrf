.. _ncs_introduction:

About the |NCS|
###############

.. contents::
   :local:
   :depth: 2

The |NCS| enables you to develop applications for nRF52, nRF53, and nRF91 Series devices.
It is a set of open source projects maintained by Nordic Semiconductor, consisting of several Git repositories publicly hosted on `GitHub`_:

* `sdk-nrf`_ repository - contains applications, samples, libraries, and drivers that are specifically targeted at Nordic Semiconductor devices.
* `sdk-nrfxlib`_ repository - contains closed-source libraries and modules in binary format.
  See the :doc:`nrfxlib documentation <nrfxlib:README>`.
* `sdk-mcuboot`_ repository - contains a fork of the `MCUboot`_ project, which provides a secure bootloader application.
  You can find the fork in :file:`bootloader/mcuboot` after obtaining the |NCS| source code.
  See the :doc:`documentation <mcuboot:index-ncs>` in Nordic Semiconductor’s MCUboot fork.
* `sdk-zephyr`_ repository - contains a fork of the `Zephyr`_ project, which provides samples, libraries, and drivers for a wide variety of devices, including Nordic Semiconductor devices.
  See the :doc:`documentation <zephyr:index>` in Nordic Semiconductor’s Zephyr fork.

Every nRF Connect SDK release consists of a combination of these repositories at different revisions.
The revision of each of those repositories is determined by the current revision of the main (or manifest) repository, ``sdk-nrf``.
It contains the SDK manifest file that enables you to manage the repositories as one code base with the west tool.

About the |NCS| license
***********************

Licenses are located close to the source files.
You can find a :file:`LICENSE` file, containing the details of the license, at the top of every |NCS| repository.
Each file included in the repositories also has an `SPDX identifier`_ that mentions this license.

If a folder or set of files is open source and included in |NCS| under its own license (for example, any of the Apache or MIT licenses), it will have either its own :file:`LICENSE` file included in the folder or the license information embedded inside the source files themselves.

The `SPDX tool`_ is used to generate license reports on each release of the |NCS|.
You can also use SPDX to generate license reports for your projects that are specific to the code included in your application.

Documentation pages
*******************

.. include:: doc_structure.rst
   :start-after: doc_structure_start
   :end-before: doc_structure_end

The |NCS| documentation contains all information that is specific to the |NCS| and describes our libraries, samples, and applications.
API documentation is extracted from the source code and included with the library documentation.

For instructions about building the documentation locally, see :ref:`doc_build`.
For more information about the documentation conventions and templates, see :ref:`documentation`.

Tools and configuration
***********************

The figure below visualizes the tools and configuration methods in the |NCS|.
They are based on the :ref:`Zephyr project <zephyr:getting_started>`.
All of them have a role in the creation of an application, from configuring the libraries or applications to building them.

.. figure:: images/ncs-toolchain.svg
   :alt: nRF Connect SDK tools and configuration

   |NCS| tools and configuration methods

* :ref:`zephyr:kconfig` generates definitions that configure libraries and subsystems.
* :ref:`Devicetree <zephyr:dt-guide>` describes the hardware.
* CMake generates build files based on the provided :file:`CMakeLists.txt` files, which use information from Kconfig and devicetree.
  See the `CMake documentation`_.
* Ninja (comparable to make) uses the build files to build the program, see the `Ninja documentation`_.
* The `GCC compiler`_ creates the executables.

Git
===

`Git`_ is a free and open source distributed version control system that allows managing the changes in the code or other collections of information (set of files) over time.

Git organizes data (files or directories) in project repositories.
The data is managed like a series of snapshots.
Every time you commit, or save the state of your project, Git takes a snapshot of what the files look like at that exact moment and stores a reference to that snapshot.
For unchanged files, Git provides just a link to the previous identical file it has already stored.

Git offers a lot of flexibility in how users manage changes, and repositories are easily duplicated.
In |NCS|, forking is the agreed-upon Git workflow.
To contribute, the official public repository in GitHub is forked.

When you say you are *forking* a repository, you are creating a copy of the repository under your GitHub ID.
This means that you are creating an identical copy that might diverge from the original over time.
This copy is your personal public repository that nobody else is allowed to push to, but changes can be pulled from it.

The original repository is called the *upstream* repository, and the newly created copy the *downstream* repository.
Any changes made to the original repository are reflected back to your forked repositories by using fetch and rebase commands.

A ``git clone`` command is used to get a copy of your downstream repository onto your local machine.
This serves as a private development environment.

Local commits are pushed to your own downstream repository, and not the official one.
To integrate the changes into the main upstream repository, a pull request is created explicitly.
Before it is merged, the pull request also serves as a convenient discussion thread if there are issues with the contributed code.
If your pull request is approved, the changes are merged with the existing original content.
Until then, your changes are reflected only in the copy you forked.

A fork can be hosted on any server, including a public Git hosting site like `GitHub`_.
It is, however, important to differentiate between the generic concept of a fork and GitHub's concept of a `GitHub fork`_.
When you create a GitHub fork, GitHub copies the original repository and tags the downstream repository (the fork) with a flag that allows users to send pull requests from the fork to its upstream repository.
GitHub also supports creating forks without linking them to the upstream respository.
See the `GitHub documentation <GitHub duplicate_>`_ for information about how to do this.

Everything in Git is checksummed before it is stored and is then referred to by that checksum.
The mechanism that Git uses for this checksumming is called a SHA-1 hash.
This hash is a 40-character string, composed of hexadecimal characters (0–9 and a–f), and calculated based on the contents of a file or directory structure in Git.

West
====

The Zephyr project includes a tool called west.
The |NCS| uses :ref:`west <zephyr:west>` to manage the combination of multiple Git repositories and versions.

Some of west’s features are similar to those provided by Git Submodules and Google’s Repo tool.
But west also includes custom features required by the Zephyr project that were not sufficiently supported by the existing tools.

For more details about the reasons behind the introduction of west, see the :ref:`zephyr:west-history` section of the Zephyr documentation.

West's workspace contains exactly one :ref:`manifest repository <zephyr:west-basics>`, which is a main Git repository containing a `west manifest file`_.
Additional Git repositories in the workspace managed by west are called projects.
The manifest repository controls which commits to use from the different projects through the manifest file.
In the |NCS|, the main repository `sdk-nrf`_ contains a west manifest file :file:`west.yml`, that determines the revision of all other repositories.
This means that sdk-nrf acts as the manifest repository, while the other repositories are projects.

When developing in the |NCS|, your application will use libraries and features from folders that are cloned from different repositories or projects.
The west tool keeps control of which commits to use from the different projects.
It also makes it fairly simple to add and remove modules.

Some west commands are related to Git commands with the same name, but operate on the entire west workspace.
Some west commands take projects as arguments.
The two most important workspace-related commands in west are ``west init`` and ``west update``.

The ``west init`` command creates a west workspace, and you typically need to run it only once to initialize west with the revision of the |NCS| that you want to check out.
It clones the manifest repository into the workspace.
However, the content of the manifest repository is managed using Git commands, since west does not modify or update it.

To clone the project repositories, use the ``west update`` command.
This command makes sure your workspace contains Git repositories matching the projects defined in the manifest file.
Whenever you check out a different revision in your manifest repository, you should run ``west update`` to make sure your workspace contains the project repositories the new revision expects (according to the manifest file).

For more information about ``west init``, ``west update``, and other built-in commands, see :ref:`zephyr:west-built-in-cmds`.

For more information about the west tool, see the :ref:`zephyr:west` user guide.

See :ref:`getting_started` for information about how to install the |NCS| and about the first steps.
See :ref:`dev-model` for more information about the |NCS| code base and how to manage it.
