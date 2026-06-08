.. _lts_releases:

Long-term support releases
##########################

.. contents::
   :local:
   :depth: 2

Long-term support (LTS) releases are :ref:`standard nRF Connect SDK releases <standard_releases>` with at least five years of support.

These releases provide a stable code base for long-term product development.
Use an LTS release when you need a predictable foundation for device firmware.
They are based on the following principles:

Stability
   Applications and boards based on an LTS release do not require modification when you update to minor LTS releases.

Security
   Security issues, including vulnerabilities, are fixed in the LTS release for the entire support period.

LTS versioning
**************

A long-term support release is not a single release or tag.
It is a series of tags, called a release series, in a release branch maintained for this purpose.
As with minor releases, you should use the latest available release in a release series.
For more information about release versioning, see :ref:`dm-revisions`.
For information about Git tags, see :ref:`dm_revisions_git_tags`.

LTS releases begin with a major release.
They then evolve through minor releases that include changes deemed necessary for the LTS release series.

For example, a release series starting with the |NCS| v3.4.0 LTS release, if necessary, would also contain the following minor releases:

* |NCS| v3.4.1 LTS
* |NCS| v3.4.2 LTS

In this example, |NCS| v3.4.0 LTS is the first release in the v3.4.x LTS release series.
You should use the latest minor release in the release series, which in this case is |NCS| v3.4.2
LTS.

Each release in a release series includes detailed :ref:`release_notes` that describe the included changes and fixes.
This information helps you decide whether to update.
Update instructions and any other required information are also included.

For every LTS minor release, any required qualification, certification, or conformance updates are performed when applicable.
The updates are made available to you.

Release cadence and support period
**********************************

LTS releases in the |NCS| are tagged every three years, aligned with Zephyr LTS releases.
A release branch is created for each LTS release and actively maintained for at least five years.

Multiple LTS releases can therefore coexist in parallel.
You can choose which LTS release to use for production.
You should consider the remaining support period for each release.

Minor updates in a particular LTS release series are decided and announced as needed.
There is no fixed cadence.
The decision to tag and release a new minor release depends on the number of bugs and security issues found in the release series.

Maintenance scope
*****************

Each LTS branch receives updates for the following changes:

* Vulnerabilities with a Common Vulnerability Scoring System (CVSS) v3.1 score of 7.0 or higher, which indicates high or critical severity.
* Bugs in the |NCS| that can make your products unusable, provided the fix does not break the contract with out-of-tree code and data.
* Selected bug fixes and security updates inherited from open source projects, such as Zephyr, Trusted Firmware-M, and Mbed TLS.

The ``nrfutil toolchain-manager`` command provides the toolchain for an LTS major release.
Tools that handle the lifecycle state of devices, such as ``nrfutil-device``, receive security and bug fixes throughout the LTS support period.

The following items are outside the scope of LTS maintenance:

* Experimental modules and libraries can change incompatibly in an LTS release, although security fixes are still provided.
* Samples and demo applications are not covered.
* External code, including modules, is not covered unless explicitly supported.
* The nRF Connect for VS Code extension is not covered.
* The nRF Connect for Desktop application is not covered.
* Nordic mobile applications are not covered.

Device classification support
*****************************

Device classification determines the level of validation provided for devices in an LTS release.
Devices supported by the |NCS| are classified into two categories:

* Nordic-supported devices
* Community-supported devices

Community-supported devices
===========================

These devices are actively supported in upstream Zephyr.
Active contributions are made to upstream work for these devices.
Dedicated build or test activities are not performed for these devices in the |NCS|.
Their support life cycle is aligned with the upstream Zephyr project.

Reference: *Release Process*, Zephyr Project Documentation.

Nordic-supported devices
========================

Nordic-supported devices are classified into three life cycle categories that reflect their production and support status.

Pre-release
-----------

Devices in this category are delivered as early samples for prototyping and evaluation.
They represent products that are not yet in mass production.
Their software enablement is under active development.
Features, APIs, and behavior might evolve before general availability.
These devices might not comply with the Cyber Resilience Act and other regulations.

Active
------

Devices in this category have been transferred to mass production and are in the active phase of their product life cycle.
These devices receive full engineering focus and are the primary targets for new feature development.

All software components are built and tested for active devices.
These devices receive the highest level of validation and are intended for production-grade development.

Maintenance
-----------

Devices in this category are in mass production, and their feature set is complete.
No new features are introduced.
Only critical bug fixes and security fixes are delivered.
These devices are maintained on LTS branches to provide long-term stability for products in the market.

When devices become obsolete and enter the end-of-life phase, device support is discontinued after Nordic publishes a *Product Discontinue Notice*.

Nordic-delivered companion ICs, including power management integrated circuits (PMICs), do not follow this device classification.

Pre-release devices in LTS branches
===================================

Pre-release devices can be either publicly launched devices or devices that have not been launched.
Only publicly launched devices are expected to move to the supported devices subcategory in the active phase.
No new devices are expected to be added to an LTS branch after the branch is created.

Experimental features in LTS branches
=====================================

It is not recommended to use experimental features in production.
You can use experimental features at your own risk, but they might not comply with the Cyber Resilience Act (CRA) or other regulations.
Non-functional requirements, such as memory usage and power consumption, are not guaranteed for experimental features.
The API of an experimental feature can change or be removed.
For general software maturity definitions, see :ref:`software_maturity`.

Experimental features are expected to move to supported maturity in an LTS branch when they are ready.
Not all experimental features must be promoted to supported maturity.
If promoting an experimental feature to supported maturity requires an API change within the feature, the API change can be accepted.
If the promotion affects APIs or non-functional characteristics in dependent modules, the change requires approval and alignment with the relevant technical owners.

Samples are not part of LTS maintenance.
Samples serve the following purposes:

* Running internal continuous integration (CI).
* Demonstrating new features.

Deprecated features in LTS branches
===================================

It is recommended that new designs avoid deprecated features.
Existing production designs that rely on deprecated features continue to be supported.
For general deprecation policy, see :ref:`api_deprecation`.

LTS release usage requirements
******************************

To use an LTS release, you must meet the LTS-specific requirements in this section.
For general installation requirements, see :ref:`requirements`.
For operating system support, see :ref:`supported_OS`.

* Provide a computer with an operating system marked as supported in the LTS release, such as an x86-64 computer with Windows 11 installed.
* Provide the required third-party tools at the versions stated as supported, such as SEGGER J-Link tools.
* Use standard commands to debug and program the integrated circuit (IC), such as ``west debug`` and ``west flash``.
* Provide enough volatile and non-volatile memory for applications built against the latest LTS release.
* Verify that updating to a newer release in an LTS release series does not affect application execution.
  Changes that could affect application behavior are carefully avoided.
  However, small timing and memory usage changes might be unavoidable and could trigger different behavior in applications.
* Rerun any additional provisioning steps that might be required after a firmware update.

Migration between LTS releases
******************************

API changes can be required when you migrate from one LTS release to another LTS release on a different branch.

When two LTS releases are supported in parallel, it is recommended that new designs start with the latest |NCS| LTS release.
For general migration information, see :ref:`migration_guides`.
