.. _ncs_introduction:

Introduction
############

.. contents::
   :local:
   :depth: 2

The |NCS| is a scalable and unified software development kit for building low-power wireless applications based on the Nordic Semiconductor nRF52, nRF53, and nRF91 Series wireless devices.
It offers an extensible framework for building size-optimized software for memory-constrained devices as well as powerful and complex software for more advanced devices and applications.

It integrates the Zephyr™ real-time operating system (RTOS) and a wide range of complete applications, samples, and protocol stacks such as Bluetooth® Low Energy, Bluetooth mesh, Matter, Thread/Zigbee and LTE-M/NB-IoT/GPS, TCP/IP.
It also includes middleware such as CoAP, MQTT, LwM2M, various libraries, hardware drivers, Trusted Firmware-M for security, and a secure bootloader (MCUBoot).


Repositories
************

The |NCS| is a combination of software developed by Nordic Semiconductor and open source projects, hosted as `Git`_ repositories in the `nrfconnect GitHub organization`_.

The ``sdk-nrf`` repository contains the SDK manifest file that manages the repositories as one :ref:`code base <dm_code_base>` with the :ref:`ncs_west_intro` tool.

Some notable repositories include:

* `sdk-nrf`_ repository - contains applications, samples, libraries, and drivers that are specifically targeted at Nordic Semiconductor devices.
* `sdk-nrfxlib`_ repository - contains closed-source libraries and modules in binary format.
  See the :doc:`nrfxlib documentation <nrfxlib:README>`.
* `sdk-zephyr`_ repository - contains a fork of the `Zephyr`_ project, which provides samples, libraries, and drivers for a wide variety of devices, including Nordic Semiconductor devices.
  See the :doc:`documentation <zephyr:index>` in Nordic Semiconductor’s Zephyr fork.
* `sdk-mcuboot`_ repository - contains a fork of the `MCUboot`_ project, which provides a secure bootloader application.
  You can find the fork in :file:`bootloader/mcuboot` after obtaining the |NCS| source code.
  See the :doc:`documentation <mcuboot:index-ncs>` in Nordic Semiconductor’s MCUboot fork.

Every |NCS| release consists of a combination of all included repositories at different revisions.
See the :ref:`repos_and_revs` section for a comprehensive list of repositories and their current revisions.
The revision of each of those repositories is determined by the current revision of the main (manifest) repository ``sdk-nrf``.

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

Git offers a lot of flexibility in how users manage changes, and repositories are easily duplicated.
In |NCS|, forking is the agreed-upon Git workflow.
To contribute, the official public repository in GitHub is forked.

A fork can be hosted on any server, including a public Git hosting site like `GitHub`_.
It is, however, important to differentiate between the generic concept of a fork and GitHub's concept of a `GitHub fork`_.
When you create a GitHub fork, GitHub copies the original repository and tags the downstream repository (the fork) with a flag that allows users to send pull requests from the fork to its upstream repository.
GitHub also supports creating forks without linking them to the upstream repository.
See the `GitHub documentation <GitHub duplicate_>`_ for information about how to do this.

.. _ncs_west_intro:

West
====

The Zephyr project includes a tool called west.
The |NCS| uses :ref:`west <zephyr:west>` to manage the combination of multiple Git repositories and versions.

Some of west’s features are similar to those provided by Git Submodules and Google’s Repo tool.
But west also includes custom features required by the Zephyr project that were not sufficiently supported by the existing tools.

West's workspace contains exactly one :ref:`manifest repository <zephyr:west-basics>`, which is a main Git repository containing a `west manifest file`_.
Additional Git repositories in the workspace managed by west are called projects.
The manifest repository controls which commits to use from the different projects through the manifest file.
In the |NCS|, the main repository `sdk-nrf`_ contains a west manifest file :file:`west.yml`, that determines the revision of all other repositories.
This means that `sdk-nrf`_ acts as the manifest repository, while the other repositories are projects.

When developing in the |NCS|, your application will use libraries and features from folders that are cloned from different repositories or projects.
The west tool keeps control of which commits to use from the different projects.
It also makes it fairly simple to add and remove modules.

See :ref:`getting_started` for information about how to install the |NCS| and about the first steps.
See :ref:`dev-model` for more information about the |NCS| code base and how to manage it.

Licenses
********

Licenses are located close to the source files.
You can find a :file:`LICENSE` file, containing the details of the license, at the top of every |NCS| repository.
Each file included in the repositories also has an `SPDX identifier`_ that mentions this license.

If a folder or set of files is open source and included in |NCS| under its own license (for example, any of the Apache or MIT licenses), it will have either its own :file:`LICENSE` file included in the folder or the license information embedded inside the source files themselves.

You can use the west :ref:`ncs-sbom <west_sbom>` utility to generate a license report.
It allows you to generate a report for the |NCS|, built application, or specific files.
The tool is highly configurable.
It uses several detection methods, such as:

 * Search based on SPDX tags.
 * Search license information in files.
 * The `Scancode-Toolkit`_.

Depending on your configuration, the report is generated in HTML or SPDX, or in both formats.
See the :ref:`west_sbom` documentation for more information.

Documentation pages
*******************

.. include:: doc_structure.rst
   :start-after: doc_structure_start
   :end-before: doc_structure_end

To access different versions of the |NCS| documentation, use the version drop-down in the top right corner.
A "99" at the end of the version number of this documentation indicates continuous updates on the main branch since the previous major.minor release.

The |NCS| documentation contains all information that is specific to the |NCS| and describes our libraries, samples, and applications.
API documentation is extracted from the source code and included with the library documentation.

For instructions about building the documentation locally, see :ref:`doc_build`.
For more information about the documentation conventions and templates, see :ref:`documentation`.

.. _repos_and_revs:

|NCS| repository revisions
**************************

The following table lists all the repositories (and their respective revisions) that are included as part of |NCS| |version| release:

.. manifest-revisions-table::
   :show-first: zephyr, nrfxlib, mcuboot, trusted-firmware-m, find-my, homekit, matter, nrf-802154, tfm-mcuboot, mbedtls-nrf, memfault-firmware-sdk
