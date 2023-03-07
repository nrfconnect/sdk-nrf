.. _ncs_introduction:

Introduction
############

.. contents::
   :local:
   :depth: 2

The |NCS| is a scalable and unified software development kit for building low-power wireless applications based on the Nordic Semiconductor nRF52, nRF53, nRF70, and nRF91 Series wireless devices.
It offers an extensible framework for building size-optimized software for memory-constrained devices as well as powerful and complex software for more advanced devices and applications.

It integrates the Zephyr™ real-time operating system (RTOS) and a wide range of complete applications, samples, and protocol stacks such as Bluetooth® Low Energy, Bluetooth mesh, Matter, Thread/Zigbee, Wi-Fi®, and LTE-M/NB-IoT/GPS, TCP/IP.
It also includes middleware such as CoAP, MQTT, LwM2M, various libraries, hardware drivers, Trusted Firmware-M for security, and a secure bootloader (MCUboot).

Repositories
************

The |NCS| is a combination of software developed by Nordic Semiconductor and open source projects, hosted as `Git`_ repositories in the `nrfconnect GitHub organization`_.

The ``sdk-nrf`` repository is the manifest repository.
It contains the SDK's `west manifest file`_ that lists all the SDK's repositories and their revisions.
This :ref:`code base <dm_code_base>` is managed with the :ref:`ncs_west_intro` tool.

Some notable repositories include:

* `sdk-nrf`_ repository - Contains applications, samples, libraries, and drivers that are specifically targeted at Nordic Semiconductor devices.
* `sdk-nrfxlib`_ repository - Contains closed-source libraries and modules in binary format.
  See the :doc:`nrfxlib documentation <nrfxlib:README>`.
* `sdk-zephyr`_ repository - Contains a fork of the `Zephyr`_ project, which provides samples, libraries, and drivers for a wide variety of devices, including Nordic Semiconductor devices.
  See the :doc:`documentation <zephyr:index>` in Nordic Semiconductor's Zephyr fork.

  .. note::

     The `sdk-zephyr`_ repository is a :term:`soft fork` that Nordic Semiconductor maintains.
     It is not the same as Zephyr SDK, which is a set of :ref:`installation tools <gs_installing_toolchain>` used while installing the |NCS|.

* `sdk-mcuboot`_ repository - Contains a fork of the `MCUboot`_ project, which provides a secure bootloader application.
  You can find the fork in :file:`bootloader/mcuboot` after obtaining the |NCS| source code.
  See the :doc:`documentation <mcuboot:index-ncs>` in Nordic Semiconductor's MCUboot fork.

All repositories with the prefix ``sdk`` contain the |NCS| firmware and code.
Every |NCS| release consists of a combination of all included repositories at different revisions.
See the :ref:`repos_and_revs` section for a comprehensive list of repositories and their current revisions.
The revision of each of those repositories is determined by the current revision of the main (manifest) repository ``sdk-nrf``.

.. _intro_vers_revs:

Versions and revisions
**********************

The |NCS| uses a versioning scheme similar to `Semantic versioning`_, but with important semantic differences.
Every release of the |NCS| is identified with a version string, in the format ``MAJOR.MINOR.PATCH``.
The version numbers are incremented based on the following criteria:

* The ``MAJOR`` version number is increased seldom, whenever a release is deemed to be introducing a large number of substantial changes across the board.
* The ``MINOR`` version number is increased every time a major release is cut.
  Minor releases are the default types of an |NCS| release.
  They introduce new functionality and may break APIs.
* The ``PATCH`` version number is increased whenever a minor or bugfix release is cut.
  Patch releases only address functional issues but do not introduce new functionality.

In between releases, |NCS| is not static.
Instead, it changes its revision every time a Git commit is merged into the `sdk-nrf`_ repository.
The revision of the SDK is considered to be equivalent to the repository revision of ``sdk-nrf``, because it is the :ref:`manifest repository <zephyr:west-manifests>`.
This means that, by virtue of containing the `west manifest file`_, its revision uniquely identifies the revisions of all other repositories included in the SDK.

A special value of ``99`` for the ``PATCH`` version number indicates that the version string does not belong to a release, but rather a point in between two major releases.
For example, ``2.2.99`` indicates that this particular revision of the |NCS| is somewhere between versions ``2.2.0`` and ``2.3.0``.

.. include:: developing/code_base.rst
   :start-after: dev_tag_definition_start
   :end-before: dev_tag_definition_end

Revisions can either be Git SHAs or tags, depending on whether the current revision is associated with a release (in which case it is a tag) or is just any revision in between releases.

For a more formal description of versions and revisions, see :ref:`dm-revisions`.

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

Some of west’s features are similar to those provided by :term:`submodules <Submodule>` of Git and Google’s Repo tool.
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

Applications
************

To start developing your application you need to understand a few fundamental concepts.
Follow the :ref:`Zephyr guide to Application Development <zephyr:application>` and browse through the included :ref:`reference applications <applications>` in the |NCS| to get familiar with the basics.

You also need to decide how to structure your application.
You can choose from a few alternative :ref:`user workflows <dm_user_workflows>`, but having the :ref:`application as the manifest repository <dm_workflow_4>` is recommended.
An `ncs-example-application`_ repository is provided to serve as a reference or starting point.

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

.. include:: documentation/structure.rst
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
   :show-first: zephyr, nrfxlib, mcuboot, trusted-firmware-m, find-my, homekit, matter, nrf-802154, mbedtls, memfault-firmware-sdk
