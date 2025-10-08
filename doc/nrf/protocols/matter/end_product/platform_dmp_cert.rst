.. _ug_matter_platform_and_dmp:

Matter Compliant Platform and Derived Matter Product (DMP)
##########################################################

This document provides an overview of the Matter Compliant Platform certification and the Derived Matter Product (DMP) concept and process.
It explains how to leverage a certified Matter platform to streamline product certification, outlines the responsibilities of platform providers and product makers, and details the steps required to achieve DMP certification.

.. contents::
   :local:
   :depth: 2

.. _ug_matter_test_types:

Test types used in this document
********************************

Throughout this document, the following test types vocabulary is used:

* Product-only tests - Tests that are specific to the end product and are not covered by complaint platform certification. These must be executed during Derived Matter Product (DMP) certification.
* Platform-only tests - Tests that are specific to the platform and are executed during complaint platform certification. These do not need to be repeated for each product built on the platform.
* Both tests - Tests that are relevant to both the platform and the product. These are executed during both platform and product (DMP) certification.
* Platform tests - The set of the tests included Platform-only tests and Both tests.
* Product tests - The set of the tests included Product-only tests and Both tests.

Program summary
***************

This section summarizes the Matter Compliant Platform Certification trial program and the Derived Matter Product (DMP) concept.

Matter Compliant Platform Certification
=======================================

Matter Compliant Platform Certification validates the foundational Matter functionality that can be reused across products.
A certified platform implements the supported transports (Thread, Wi-Fi®, Ethernet), commissioning, the Root Node device type and associated baseline clusters, and documents its Supported Operating Environment (SOE).
The platform is evaluated by an Authorized Test Laboratory (ATL) using the CSA Test Harness against platform tests.

.. note::
   Matter Certified Platforms are not listed on the Distributed Compliance Ledger (DCL).
   Certification Transfer cannot be used to transfer only the platform.
   The certification for Matter Compliant Platforms can be found on the `CSA Certified Products Database`_.

Supported Operating Environment (SOE)
=====================================

The Matter Compliant Platform SOE defines the hardware families or boards and software versions (SDK/OS) supported by the platform.
If you follow within the Matter Platform SOE and the platform's certified scope, you can skip platform tests at DMP time.

.. _ug_matter_platform_and_dmp_matrix:

Matter Compliant Platform certification matrix:
===============================================

The following table presents the certification matrix for the Matter Compliant Platform delivered through the |NCS|.
This matrix lists the |NCS| versions, Matter versions, product names, certification details, CSA certificate links, and supported hardware within the SDK (SOE).
The table will be updated as new platform versions are certified.

+-------------------+-------------------+---------------------+---------------------+------------------+----------------------------------------------------+-------------------------------------------+
| NCS version       | Matter version    | Product name        | Certification ID    | Transport        | CSA Certificate link                               | Supported hardware (SOE)                  |
+===================+===================+=====================+=====================+==================+====================================================+===========================================+
| 3.1.1             | 1.4.2             | |NCS|               | CSA25001MCPM0001-24 | Thread, BLE      | `Matter nRF Connect SDK Platform Certificate`_     | nRF52 Series, nRF53 Series, nRF54L Series |
+-------------------+-------------------+---------------------+---------------------+------------------+----------------------------------------------------+-------------------------------------------+

.. note::
   The `Matter nRF Connect SDK Platform Certificate`_ provides access to both the platform certificate and the associated PICS baseline.

Derived Matter Product
======================

A DMP is a complete product built on a Matter Compliant Platform.
During DMP certification, the ATL runs the Product tests; Platform-only tests are skipped because they were already executed for the platform.
A certified DMP has the same rights and status as a standard Certified Matter Product.

Benefits of the DMP approach
----------------------------

Using the DMP approach offers the following advantages:

* Faster path to certification by reusing platform test results and only running Product tests.
* Lower cost and effort due to a smaller test scope.
* Quality and interoperability by building on a validated foundation (commissioning, secure channel, baseline clusters).
* Focus on product value: device type behavior, application clusters, UX.

DMP requirements
----------------

To qualify as a DMP, you must meet the following requirements:

* Match the platform's Matter specification version.
* Stay within the platform's SOE (boards, radios, SDK and OS versions).
* Provide the Matter Security Attestation (covering both platform and product).
* Declare product PICS that are a subset of or compatible with the platform's certified capabilities in core functionalities.
  Products may declare additional features beyond the platform's certified scope, but these additional features must be tested in the DMP certification process.

DMP rules and testing
**********************

This section explains how to build a DMP on top of a certified platform and how testing is selected.

Rules for building a DMP on a platform
======================================

Apply these rules when building on a platform:

* The Matter spec version must match the platform's spec version.
* Remain within the platform's SOE (boards, radios, SDK and OS versions).
* Ensure that the product PICS is a subset of or compatible with the platform's certified capabilities in core functionalities.
  Products may declare additional features beyond the platform's certified scope, but these additional features must be tested in the DMP certification process.
* Provide a Matter Security Attestation covering both platform and product.

Allowed changes and impact
==========================

The following changes are typical and their impact is described:

+---------------------------------------------------------------+----------+-------------------------------------------------------------------------------------------------+
| Change                                                        | Allowed  | Impact                                                                                          |
+===============================================================+==========+=================================================================================================+
| Disable optional features present in the platform             | Yes      | Reflect in product PICS                                                                         |
+---------------------------------------------------------------+----------+-------------------------------------------------------------------------------------------------+
| Enable optional features within platform scope                | Yes      | ATL adds corresponding tests to DMP testing scope                                               |
+---------------------------------------------------------------+----------+-------------------------------------------------------------------------------------------------+
| Exceed platform scope (new transport/band/radio beyond SOE)   | No       | Platform update or recertification required first                                               |
+---------------------------------------------------------------+----------+-------------------------------------------------------------------------------------------------+
| Update only application (no platform or spec change)          | Yes      | Run Product tests                                                                               |
+---------------------------------------------------------------+----------+-------------------------------------------------------------------------------------------------+
| Move to a new Matter spec version                             | No       | Platform software must update first; DMP cannot advance alone                                   |
+---------------------------------------------------------------+----------+-------------------------------------------------------------------------------------------------+

.. note::
   If the platform does not meet your product requirements and you cannot leverage platform certification for your DMP, you may pursue the standard Matter certification process.
   In this case, your end product will undergo the full suite of certification tests as a standalone device, independent of any platform certification.
   This approach is available to all product makers when platform-based DMP certification is not feasible or sufficient.

Test selection at DMP time
==========================

At DMP time, tests are selected and executed as follows:

* The CSA Test Harness uses your PICS and skips the Platform-only tests using the DMP skip configuration (:file:`dmp-test-skip.xml`) generated by the PICS Tool.
* You will run Product tests in product context.

Recertification scenarios
=========================

Use the following scenarios to determine when recertification is required:

* No changes (platform, product, spec) - No recertification.
* Product-only changes (same platform software, same spec) - DMP recertification with testing (Product tests).
* Platform-only software change (same spec) with platform recertified - DMP recertification, no DMP testing required (documentation update).
* Platform software change (same spec) and product changes - DMP recertification with testing (Product tests).
* Spec version change without updated platform software - Not allowed: the platform must update first.
* Spec version change with updated or recertified platform software:

  * If product is unchanged - Spot-check of Product tests: platform must have been recertified.
  * If product also changes - DMP recertification with testing (Product tests).

Use the following guidance when planning changes:

* Widening capabilities beyond the platform SOE requires a platform update and recertification before DMP can proceed.
* Changing optional features within platform scope leads to a focused DMP test run for the affected areas and the Product tests.

.. _ug_matter_dmp_ncs_guideline:

Guidelines for leveraging the platform's |NCS| to build a DMP
*************************************************************

This section provides practical steps for using the |NCS| to move from a platform to a DMP.

.. rst-class:: numbered-step

Choose a supported configuration
================================

Start by reviewing the platform's SOE and selecting a supported SoC and transport.
Use the specific Matter component within the |NCS|, which corresponds to the platform certification.

.. rst-class:: numbered-step

Fetch and check out the certified platform sources
==================================================

Start by initializing and updating the |NCS| repository at the tag corresponding to the certified platform.
This ensures your environment matches the platform as tested and certified:

.. code-block:: console

   west init -m https://github.com/nrfconnect/sdk-nrf --mr <platform_certified_tag>
   west update

.. note::
   The certified tag is documented in the platform's certification information.

.. rst-class:: numbered-step

Build a reference certification template
========================================

With the checked-out platform sources, build the template application that was used during platform certification.
This provides a certified reference for the enabled features and clusters:

.. code-block:: console

   west build -b <board> samples/matter/template -T sample.matter.template.certification
   west flash --recover

.. note::
   The reference data model used during platform testing can be found in: :file:`samples/matter/common/src/certification`

This template is provided as a reference (transport, core clusters).
You may customize your product by enabling or disabling optional clusters, attributes, and features and by building your own application data model.
When customizing, ensure that your PICS reflects the final feature set, remain within the platform's SOE (boards, radios and SDK/OS versions), and do not alter the core platform functionality covered by the platform certification.

.. important::
   The Matter samples provided in the |NCS| are maintained to be as close as possible to Matter specification compliance, but they are not certified and are not part of the compliant platform.
   These samples should not be treated as certified implementations and are provided for reference and development purposes only.

.. rst-class:: numbered-step

Customization examples (PICS deltas and impact on test areas)
=============================================================

The following examples illustrate typical PICS deltas and their test impact:

* Enable the Basic Information cluster's Product Appearance attribute (optional) - The Test Harness includes Basic Information test cases affected by the PICS; rest of the Platform-only tests remain skipped.
* Enable the Time Synchronization cluster's NTP feature (optional) - The Test Harness includes Time Sync test cases affected by the PICS; rest of the Platform-only tests remain skipped.
* Disable the Diagnostic Logs cluster - The Test Harness does not include Diagnostic Logs test cases; Platform-only tests remain skipped.


What not to change (to retain inheritance)
==========================================

To retain test inheritance from the platform, avoid the following changes:

* Upgrading :file:`modules/lib/matter` beyond the tag corresponding to the certified |NCS| tag without a coordinated platform update.
* Changing radio/PHY parameters, Wi-Fi bands, or Thread version beyond the SOE.
* Editing the platform PICS baseline or any platform test list artifacts.


What you can change safely (typical product work)
=================================================

You can safely make the following product-level changes:

* Application code - Device type logic, endpoints, attributes, UI/UX, application cluster handlers, etc.
* Application configuration - Kconfig/DTS overlays for product peripherals, partitions, etc.
* Optional clusters/features within platform scope (enable/disable) with matching PICS updates.
* Manufacturing data and branding - Stock keeping unit (SKU), product strings, documentation, etc.
* Changes to application clusters in :file:`modules/lib/matter` (not part of platform).

.. rst-class:: numbered-step

Prepare certification inputs
============================

Before testing, prepare the following inputs:

* Product PICS with ``PLAT.CERT.TESTS.DONE = True``.
* :file:`dmp-test-skip.xml` file from the PICS Tool (used by Test Harness to skip Platform-only tests).
* Ensure that the product PICS remains a subset of or compatible with platform capabilities in core functionalities.
  Products may declare additional features beyond the platform's certified scope, but these additional features must be tested in the DMP certification process.
* Request the platform's templated security attestation (pre-filled with platform details) through the `DevZone`_ (Nordic's developer portal for technical support and resources).
* Leverage dependent certification identifiers from platform.
  You can visit the following pages to check the Bluetooth QDIDs and Thread CIDs valid for SoCs that support Matter applications:

  * `nRF52840 Compatibility Matrix`_
  * `nRF5340 Compatibility Matrix`_
  * `nRF54L15 Compatibility Matrix`_

* Retrieve the Platform PICS baseline from the CSA website ref :ref:`ug_matter_platform_and_dmp_matrix` to confirm compatibility.

.. rst-class:: numbered-step

Pre-validation using the CSA Test Harness
=========================================

It is recommended to perform a pre-validation of your product test cases using the CSA Test Harness before engaging with the Authorized Test Laboratory (ATL).
In the Test Harness, create a DMP project, upload your PICS, and add the :file:`dmp-test-skip.xml` file.
The Test Harness will automatically select the Product-only tests and all Both tests to be re-run.
This pre-validation helps ensure your PICS and test setup are correct and can identify issues early, streamlining the formal certification process.

For detailed Test Harness setup instructions for DMP projects, see the Matter Test Harness Guide, available in the `CSA Matter Resource Kit`_ in Matter Test Harness section.

If you need more information about which test cases have been executed on the platform during certification, you can request the official Test Report for the certified platform (which includes the list of executed test cases) through the `DevZone`_.

.. rst-class:: numbered-step

Arrange testing and apply for certification with the ATL
========================================================

After pre-validation, coordinate with your chosen ATL to schedule the official certification testing.
Provide the ATL with your PICS, :file:`dmp-test-skip.xml` file, and any required documentation.
The ATL will execute the required tests according to the CSA procedures and correlate your product results with the platform certification results using the Platform Certification ID.
Upon successful completion, submit your Declaration of Conformity (DoC) referencing the Platform Certification ID, along with your PICS, DMP test skip file, security attestation, and any dependent certification evidence.
After approval, register your product in the DCL and follow the logo usage guidelines.

.. _ug_matter_dmp_submission_checklist:

DMP submission checklist
************************

Use this checklist to prepare and submit your DMP efficiently:

* Board and transport selection confirmed within SOE and certified |NCS| tag noted.
* Product partition layout and bootloader configuration prepared (see :ref:`ug_matter_device_bootloader_partition_layout`, :ref:`ug_matter_device_bootloader`).
* Attestation Certificates generated (test or production) (see :ref:`ug_matter_device_attestation`).
* Factory data prepared (VID, PID, discriminator, etc.); onboarding codes generated if needed (see :ref:`ug_matter_device_factory_provisioning`).
* Certification Declaration generated for the product for certification testing (see :ref:`ug_matter_device_configuring_cd`).
* Test Event Triggers enabled for certification testing (see :ref:`ug_matter_test_event_triggers`).
* Versioning aligned (MCUboot image version and Matter software version string) for OTA over Matter, if applicable (see :ref:`ug_versioning_in_matter`).
* Product PICS exported and validated in the PICS Tool.
* :file:`dmp-test-skip.xml` file generated from the PICS Tool to skip Platform-only tests.
* Security attestation prepared (platform + product), consistent with the platform’s certified scope (see :ref:`ug_matter_device_security`).
* Dependent certification evidence collected as applicable based on the platform's certification (see :ref:`ug_matter_device_certification_reqs_dependent`).
* Pre-validation run in the CSA Test Harness completed (optional but recommended) with the same PICS and :file:`dmp-test-skip.xml` file.
* Declaration of Conformity (DoC) drafted, referencing the Platform Certification ID and listing hardware, software, and firmware versions.
* Submission bundle assembled for the ATL (PICS, :file:`dmp-test-skip.xml` file, DoC, Certification Application ID).
* Submission bundle assembled for the CSA (PICS, :file:`dmp-test-skip.xml` file, DoC, security attestation, dependent certs).
* Information provided to the DCL for product certification (see :ref:`ug_matter_device_dcl`).

.. note::
   All necessary document templates (like DoC, Security Attestation, PICS) for CSA certification submission can be found in the `CSA Matter Resource Kit`_.


Responsibilities and deliverables
*********************************

The following responsibilities and deliverables apply to the platform provider and the product maker.

Platform provider deliverables
===============================

The platform provider must supply the following:

* Complaint Platform Certification ID and access for ATL to the platform test report (if necessary).
* SOE specification (supported boards/radios, SDK/OS versions).
* |NCS| release mapping to the Matter software version.
* Dependent certification references applicable to the platform.
* Security attestation (pre-filled with platform details).

Product maker deliverables
==========================

As a product maker, you supply the following:

* CSA membership.
* DoC referencing the Platform Certification ID and listing product hardware, software and firmware versions.
* Product PICS with ``PLAT.CERT.TESTS.DONE = True``.
* The :file:`dmp-test-skip.xml` file generated by the PICS Tool.
* Matter Security Attestation (platform + product).
* Dependent certification evidence (Thread/Wi-Fi/Bluetooth/Ethernet as applicable).

Support
*******

If you are unsure whether a change remains within platform scope, contact the CSA Certification support team (certification@csa-iot.org) to confirm whether it is permissible under platform certification or if a platform update is required before product certification can proceed.
