.. _ncs_addons_overview:

Add-ons in the nRF Connect SDK
##############################

.. contents::
   :local:
   :depth: 2

This page explains why NCS offers add-ons, how their releases relate to NCS, what kinds of capabilities they deliver, and what you should find in add-on documentation.

When you know which protocol, ecosystem, or feature you need, open the `nRF Connect SDK Add-on Index <nRF Connect SDK Add-ons_>`_, choose a version, then follow :ref:`ncs_addons_setup` and that add-on's setup page.

For add-on contributors, see the `ncs-app-index repository <ncs-app-index_>`_.

Overview
********

An add-on is a supplementary software package that extends NCS without being part of the main SDK release tree.

* Maintained in its own Git repository, with its own release notes and documentation site.
* Installed through a west manifest (:file:`west.yml`) that declares which NCS revision the add-on release was built and tested against.
* Listed in the add-on index (nRF Connect for VS Code or the web catalog), with a maintainer and support contact per entry.

Add-ons are not a replacement for NCS.
You use the same toolchain, CMake/Kconfig workflow, and Zephyr board targets as in the main SDK; the add-on adds stacks, libraries, samples, or reference applications on top.

Rationale
*********

NCS is released on a fixed SDK cadence with a broad scope: many SoCs, protocols, and samples in one tree.
Add-ons let Nordic Semiconductor and partners ship focused software on a schedule that fits the product, without waiting for the next full NCS release or expanding core NCS with every optional stack.

In practice, add-ons help when:

* A protocol or feature must evolve on a different timeline than NCS (faster fixes and feature drops, or longer stabilization on a pinned NCS line).
* Content is optional, large, or licensed separately and should not be in every customer's default download.
* A vertical reference or partner ecosystem needs its own compatibility story and support boundary.
* New silicon or programs need a documented SDK before everything is merged into core NCS.

The main SDK stays the common foundation; add-ons are optional extensions you enable when your product needs them.

Release model and compatibility
*******************************

An add-on release does not automatically mean a new NCS major or minor version.
Each add-on version documents which NCS revision it supports, typically in release notes and a compatibility table (add-on version -> NCS version -> relevant third-party stacks).

That separation is intentional:

* The add-on team can publish bugfix or feature releases while staying on the same pinned NCS tag until they qualify an update to a newer NCS.
* You can standardize on an NCS line and take add-on updates that improve the stack you care about, without upgrading the whole workspace until you choose to.
* Qualification and maturity are stated per add-on (and sometimes per SoC), not only at the core NCS release.

Before you change NCS or add-on version in production, refer to the add-on compatibility table and release notes.
Do not assume the latest NCS tag works with an arbitrary add-on tag.

.. _ncs_addons_coverage:

What add-ons cover
******************

This section describes types of capabilities, not a product catalog.
Use the add-on index to find the entry that matches your needs.

.. list-table::
   :header-rows: 1
   :widths: 26 38 36

   * - Type of capability
     - What the add-on provides
     - How to choose one
   * - Protocol or stack
     - Stack, libraries, and samples for a wireless or network technology on a pinned NCS version.
     - NCS protocol or release notes when support is not in core for your device; index tags and docs URL.
   * - Reusable capability
     - Libraries and samples you can integrate into many applications (for example on-device ML or specialized middleware).
     - Index description and add-on samples; may be imported by a solution add-on's west manifest.
   * - Solution or vertical
     - End-to-end reference application combining protocols and domain logic.
     - Add-on documentation and compatibility table; not a substitute for core NCS getting started.
   * - Third-party ecosystem
     - Partner stacks with their own licensing, tooling, or acceptance steps.
     - Add-on setup page and index entry; read license and prerequisite sections before install.
   * - Program or silicon
     - Family- or program-specific SDK, sometimes before full core NCS integration.
     - Index when public; otherwise documentation from your Nordic program contact.

If your boards, samples, and applications are already covered in the main NCS documentation, the default SDK workspace will be sufficient.
When support is delivered as an add-on, the NCS documentation points you to the index or hosted add-on documentation.

Documentation structure
***********************

Each add-on publishes its own documentation set, that is versioned and linked from the add-on index.
Content varies by product, but you will consistently find:

* Setup — Hardware requirements, software prerequisites, and workspace install.
  :ref:`ncs_addons_setup` describes the shared west and VS Code flow, while the add-on page adds product-specific steps (extra tools, ``pip install``, license acceptance, partner accounts).
* Release notes and compatibility — Version history and a table mapping add-on releases to supported NCS revisions (and third-party stacks when relevant).
  Use the respective table to choose a supported combination for production.
* Samples and libraries — Reference applications and APIs, with supported boards in documentation and often in :file:`sample.yaml`.
* Configuration — Kconfig and devicetree information.
* Maturity and support — Marking the maturity of the add-on.
* Core NCS topics — Installing the SDK, generic build and flash, toolchains, and device guides remain in NCS and are cross-linked from the add-on.

Hosted documentation may be tied to tags or :file:`versions.json` so you can browse docs that match the add-on version you installed.

Related material
****************

* :ref:`ncs_addons_setup` — workspace setup (prerequisites, VS Code, command line).
* `ncs-app-index repository <ncs-app-index_>`_ — contributing an add-on to the index and recommended repo layout.
* :ref:`Creating application from add-ons <creating_add_on_index>` — using the add-on index in VS Code.
* :ref:`integrations` — add-ons in the |NCS| doc set.
