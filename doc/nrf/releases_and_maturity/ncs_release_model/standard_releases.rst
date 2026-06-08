.. _standard_releases:

Standard releases
#################

.. contents::
   :local:
   :depth: 2

This page explains how standard |NCS| releases are structured, documented, and maintained.
For detailed information about version strings, see :ref:`dm-revisions`.
For information about Git tags, see :ref:`dm_revisions_git_tags`.

Release purpose
***************

The purpose of a release of the |NCS| from Nordic Semiconductor is to provide a stable, validated, and versioned software baseline for products that use Nordic Semiconductor's nRF platforms.

Each release serves the following purposes:

Provide a consistent development baseline
   Includes a known-good combination of Zephyr and Nordic modules, drivers, stacks, and tools.

Ensure stability and quality
   Integrates and validates features through regression testing, hardware validation, and system-level verification.

Enable product development
   Gives you a reliable foundation for commercial products without requiring you to track unstable mainline changes.

Provide conformance-ready software
   Aligns the SDK with relevant standards, such as Bluetooth®, Thread, Matter, and security requirements, where applicable.

Bundle features and fixes
   Groups new features, improvements, bug fixes, and security fixes into predictable releases.

Manage platform evolution
   Introduces new capabilities while maintaining backward compatibility and controlled deprecation.

Support long-term maintenance
   Supports long-term support and maintenance branches for products that require long-term stability.

Each |NCS| release turns continuous development into a predictable, tested, and supportable software snapshot for product-grade use.

Release frequency
*****************

Release tags for the |NCS| are in the `sdk-nrf`_ repository.
The |NCS| follows a time-based release cadence with two or three releases a year.
This schedule provides regular, validated releases that are qualified for standards conformance without overly frequent updates.

Release versioning
******************

Release versioning defines the type of each |NCS| release tag.
For more information about release versioning, see :ref:`dm-revisions`.
For information about Git tag formats, see :ref:`dm_revisions_git_tags`.

Release tags follow semantic versioning and use the following categories:

Standard releases
   * Preview tag
   * Major release tag
   * Minor release tag
   * Maintenance release or patch tag

Long-term support (LTS)
   * Long-term supported release tag
   * Long-term supported patch tag

   For more information about LTS releases, see :ref:`lts_releases`.

Pre-release
   * Release candidate (RC) tags are also used as internal tags before releases.

Release documentation
*********************

The following documents describe each |NCS| release:

* :ref:`release_notes`
  Provide the primary entry point for understanding what changed in a release.
  They include key highlights, major feature additions, important bug fixes, security updates, and notable subsystem changes.

* :ref:`migration_guides`
  Help you upgrade to a new |NCS| version.
  They describe API changes, removed or deprecated features, Kconfig and DTS updates, and required project changes.

* :ref:`software_maturity`
  Defines the lifecycle state of |NCS| features and components.
  It helps you assess stability, API consistency, and suitability for commercial use.

* :ref:`known_issues`
  Provides a consolidated list of validated issues for a specific |NCS| release.
  When possible, it includes workarounds, mitigation steps, and references to patches or future fixes.

Release dependency management
*****************************

|NCS| releases normally align with stable Zephyr releases.
A release can occasionally use a Zephyr ``*.99`` development release if required content is not yet available in a stable Zephyr release.
In that case, the release branch is updated to the corresponding stable Zephyr tag when it becomes available.

This alignment lets you build products on well-tested Zephyr baselines with the |NCS| layered on top.
It improves quality assurance and enables consistent delivery of future bug fixes and security updates where applicable.

Deprecation policy
******************

In the ``sdk-nrf`` repository, features, modules, samples, APIs, or configuration options can be deprecated as the SDK evolves.
Deprecations are communicated in official release notes and marked in the codebase with Kconfig options or equivalent build-time indicators.

A deprecated feature remains available for a defined transition period, typically at least two subsequent |NCS| releases.
This gives you time to evaluate alternatives, adapt applications, and migrate to supported replacements.

After the deprecation period, the feature is removed from the SDK.
This keeps the codebase maintainable and aligned with the current architecture while minimizing long-term technical debt.

This policy follows the principles of the Zephyr Project deprecation policy.
The alignment provides consistency with upstream practices and predictable lifecycle management for users who build on the |NCS|.
For more information about API deprecation in the |NCS|, see :ref:`api_deprecation`.

Updating to a new release
*************************

Use the latest |NCS| LTS tag when long-term stability is required.
For more information about LTS releases, see :ref:`lts_releases`.

Before you update, review the :ref:`release_notes` and the relevant :ref:`migration_guides`.
If a product requires devices or features that are not available in the latest LTS release, you can start product development with a newer |NCS| release.

For installation instructions, see :ref:`install_ncs`.
For repository and tool update instructions, see :ref:`updating_repos`.
