.. _ug_nrf54h20_suit_device_config:

nRF54H20 SUIT device configuration
##################################

.. contents::
   :local:
   :depth: 2


The SUIT manifest defines both the installation and invocation logic using SUIT commands, classed either as conditions or directives.
This allows you to shape the logic to meet the end product's requirements, even when using a generic Secure Domain Firmware (SDFW).

In addition to the logic, certain properties of the SUIT subsystem must be exposed as configurable parameters:

* The number of SUIT manifests supported by the end product.
* Update and boot policies for the manifests.
* Attributes of the manifests.

The following sections define these properties for products based on the nRF54H20 SoC, and describe the default configuration.

SUIT manifests supported by the nRF54H20 SoC
********************************************

In the most complex predicted case, there can be 11 manifests in total, assigned across the 3 domains.

Application domain
==================

In the application domain, the following five SUIT manifests are supported:

* The root manifest
* Three local app manifests
* A recovery manifest

Local manifests are manifests that operate on the same set of local resources.
In most cases, using a single manifest is expected to be sufficient.
However, this document describes what is supported for the most complex scenarios, such as an A/B approach where two executable images are managed alongside a separate manifest for non-executable resources (for example, images or other media).

Radio domain
============

In the radio domain, the following three SUIT manifests are supported:

* Two local radio manifests
* A recovery manifest

Secure Domain and System Controller
===================================

The Secure Domain and the System Controller support the following manifests:

* The Nordic top manifest
* A System Controller manifest
* A Secure Domain manifest

The Secure Domain Manifest is responsible solely for the delivery and initiation of the update process for the Secure Domain Firmware and Secure Domain Recovery Firmware images.
In the following diagrams, the Secure Domain Manifest is labeled as ``SDFW`` or ``SDFW_RECOVERY``.

.. figure:: images/nrf54h20_suit_manifests.svg

   SUIT manifests supported by the nRF54H20 SoC in the most complex predicted case

This diagram illustrates the most complex predicted configuration.
You can choose a subset of the manifests supported by the nRF54H20.

.. figure:: images/nrf54h20_suit_manifests_minimal.svg

   SUIT manifests enabled in nRF54H20, Application and Radio images, without recovery support

This diagram illustrates the minimal predicted configuration that supports the application and radio domains.

Properties associated to SUIT manifest
**************************************

.. figure:: images/suit_manifest_properties.svg

   An example of property values of the SUIT Application Local Manifest

This diagram presents a set of properties associated with the SUIT manifest type, along with *example* values.
These properties are not part of the manifest itself but are configuration parameters stored on the device, defining the boundaries for allowable manifest behavior.
The properties covered in this document include:

* The property that enables the manifest
* The vendor ID (VID)
* The manifest class ID
* The downgrade prevention policy
* The independent updateability policy
* The key ID ranges for SUIT manifest verification and signature verification policy
* The property granting the ability to operate on device resources

The properties of Nordic-controlled manifests are hard-coded into the SDFW and cannot be modified.
However, some properties of user-customizable manifests are configurable using the manifest provisioning information data structure.

Vendor ID and manifest class ID
===============================

Vendor ID and manifest class ID allow users to specify the vendor (for example, ``Nordic``, or ``ACME Corp``) and the manifest class.
In other words, they define the intended usage of the manifest, such as the manifest controlling firmware images associated with the application domain.

Since it is impractical to store and process long, variable-length strings, RFC4122 UUID version 5 (uuid5) is used instead.

There is no strict requirement on how to generate these ID values, as long as they remain unique.
However, the recommended method for creating a VID is ``Vendor ID = UUID5(DNS_PREFIX, vendor domain name)``.
The following example shows how to generate a VID for ``nordicsemi.com`` using Python::

   uuid5(uuid.NAMESPACE_DNS, 'nordicsemi.com')

This generates ``7617daa5-71fd-5a85-8f94-e28d735ce9f4``.

The recommended method for creating a manifest class ID is ``Manifest Class ID = UUID5(Vendor ID, Class-Specific-Information)``.
The following example shows how to generate a manifest class ID using Python::

   uuid5(nordic_vid, 'nRF54H20_sample_app')

Assuming ``nordic_vid`` is equal to ``7617daa5-71fd-5a85-8f94-e28d735ce9f4``, it generates ``08c1b599-55e8-5fbc-9e76-7bc29ce1b04d``.

How manifest class ID-based filtering works
-------------------------------------------

The SDFW has access to a list of manifest class IDs supported by the device.
During the update candidate evaluation process, the SDFW compares the manifest class ID of each candidate manifest against this list.
If the manifest class ID of a candidate manifest is not recognized by the device, the update candidate is rejected.

Expressing Class-specific information
-------------------------------------

The following example illustrates this.
ACME Corp has two product types in its portfolio: roller shutter motors and light bulbs.
Both product types are powered by the nRF54H20 SoC and share the hardware design of the radio-related part, to the extent that they can share the same set of executable binary images for the radio domain.
At the same time, hardware differences between motors and light bulbs are significant and it makes sense to have a separated set of executable binary images for the application domain.

This means that it is worth considering to assign the same manifest class ID for Radio Local Manifest for both products, for example::

   uuid5(acme_vid, 'nRF54H20_radio')

And two distinct manifest class ID values for Application Local Manifest, for example::

    uuid5(acme_vid, 'nRF54H20_light_bulb_app')

    uuid5(acme_vid, 'nRF54H20_roller_shutter_app')

.. figure:: images/suit_acme_manifests.svg

   Manifest hierarchies for ACME Corp devices.

This approach allows ACME Corp to share the same radio domain update package (executable binaries and SUIT manifest) across both products.
Additionally, if an application domain update package intended for the light bulb is accidentally delivered to the roller shutter motor, the motor will reject it due to the unrecognized (unsupported) manifest class ID.

Downgrade prevention policy
===========================

Product requirements can impose different downgrade prevention policies for different manifests.
The following values are supported:

* ``downgrade_prevention_disabled``
* ``downgrade_prevention_enabled``

How downgrade prevention works
------------------------------

The ``suit-manifest-sequence-number`` (an element of the SUIT manifest) is a monotonically increasing anti-rollback counter.
As part of the update candidate evaluation process, the ``suit-manifest-sequence-number`` of the candidate manifest is compared with the installed counterpart.
The result of this comparison, along with the associated downgrade prevention policy, determines whether a candidate is accepted or rejected.

Assuming ``downgrade_prevention_enabled`` is enabled, a candidate manifest will be accepted only if its ``suit-manifest-sequence-number`` is equal or bigger than the ``suit-manifest-sequence-number`` of the installed manifest.

Independent updateability policy
================================

This policy allows the expression of whether a candidate manifest of a specific manifest class ID can be updated independently of its parent.
In some cases, this can be the intended behavior, while in others, the opposite can be desired.

The following policy values are supported:

* ``independent_update_allowed``
* ``independent_update_denied``

How independent updateability policy works
------------------------------------------

The system is designed to allow the expression of an update package as a candidate manifest along with its dependencies (candidate, child manifests).
In the simplest case, an update package composed of just one manifest (for example, a Local App Manifest), the policy ``independent_update_denied`` associated with the Local App Manifest would block the update.

This be considered intended behavior because images for the Application Core (controlled by the Local App Manifest) can rely on functionality provided by images controlled by other manifests, such as the radio manifest.
In such a case, updating the app images individually could cause interoperability issues.
Blocking the independent update of the manifest implies that the update must go through the parent manifest.
If two manifests share the same parent, the installation sequence of the parent manifest can manage compatibility between the two manifests.

Key ID ranges for SUIT manifest verification and signature verification policy
==================================================================================

Signature verification helps ensure the following:

* The signed material has not been altered.
* The material is signed by the proper signing authority.

This is essential for detecting situations where a valid signing key, intended for one type of material (for example, Local App Manifest), is maliciously used to sign another type of material (for example, Local Radio Manifest).

Nordic-related SUIT manifests are authenticated using the Edwards-curve Digital Signature Algorithm, specifically Ed25519.
Algorithms for verifying user-specific SUIT manifests are beyond the scope of this document.

Signing Authorities
-------------------

Each manifest type in  SoC is associated with a range of Key IDs that are allowed to be used for signing and verifying the signature of a specific manifest.
The user does not have the ability to override these settings.
The range of Key IDs associated with distinct manifests is hard-coded in the SDFW.

Provisioning of key material
----------------------------

The respective Nordic-related public keys for SUIT manifest verification are embedded in the SDFW.
You can provision these public keys as part of the device provisioning process.

The key provisioning process for user-specific keys is beyond the scope of this document.

Signature verification policy
-----------------------------

The policy allows you to verify if a manifest must be verified before processing.
The following signature verification policy values are supported:

* ``signature_check_disabled``
* ``signature_check_enabled_on_update``
* ``signature_check_enabled_on_update_and_boot``

.. note::

   For SUIT manifests related to the application and radio domains, signature verification must be skipped, regardless of the configured signature verification policy, if the respective domain is in one of the following lifecycle states (LCS):

   * ``LCS_EMPTY``
   * ``LCS_ROT``
   * ``LCS_ROT_DEBUG``

Ability to operate on device resources
==========================================

As the manifest operates on device resources, such as accessing memory (whether NVM, RAM, or both) and starting the processor, access rights are associated with the manifest class ID.
This ensures that if a manifest belonging to one local domain (for example, the application domain) attempts to declare components that span into areas belonging to another local domain (for example, the radio domain or, more critically, the Secure Domain), the system can detect and block this behavior.

SUIT topologies
*****************

The supported SUIT topologies are the following.

Invocation path (normal booting)
================================

.. figure:: images/nrf54h20_invocation_topology.svg

   SUIT topology for nRF54H20 invocation path in the most complex predicted case

In the event of a device cold boot, following the standard invocation procedure, the SDFW will first execute the validation, loading, and invocation sequences defined in the top manifest from Nordic Semiconductor.
Next, it will execute these sequences as specified in the root manifest.

The invocation of Nordic-related SUIT elements is fully controlled by SUIT manifests signed by Nordic Semiconductor.
This approach ensures that entities other than Nordic cannot manipulate the order of execution of Nordic-controlled elements.

Both the Nordic top and root manifests control the boot process of their respective dependency manifests.

Update path
===========

Updating regular resources (such as SUIT manifests and images) alongside those responsible for device recovery in a single update increases the risk of placing the device in an unrecoverable state.
To minimize this risk, recovery-dedicated SUIT manifests cannot be updated together with other manifests.

Update path for elements not dedicated to SUIT recovery:

.. figure:: images/nrf54h20_update_topology_for_non_recovery_elements.svg

   SUIT topology for the nRF54H20 SoC update path in the most complex predicted case for elements not dedicated to SUIT recovery.

Update path for elements dedicated to SUIT recovery:

.. figure:: images/nrf54h20_update_topology_for_recovery_elements.svg

   SUIT topology for the nRF54H20 SoC update path for elements dedicated to SUIT recovery.

Recovery path
=============

Verification failures (for the SUIT manifest signature, SUIT manifest digest, and images) in the invocation path force the device to reboot into the recovery path.
The recovery path is essentially an invocation path with a specific purpose: to download missing elements (such as images and manifests) and provide them to the SDFW as update candidates.

.. note::
   If the device is in a state requiring recovery and the application recovery manifest is not activated the SDFW will boot from the root manifest.
   If the application recovery manifest is activated, but the application recovery manifest is damaged, the SDFW will NOT boot any local images.

In the event of a device cold boot in the recovery path, the SDFW will first execute the respective validation, load, and invocation sequences from the nordic top manifest, followed by the sequences from the application recovery manifest.
Any potential failure of the nordic top manifest or its dependencies will NOT interrupt the boot process.

The application recovery manifest has the ability to directly control local application images and manage the boot process using the respective dependency manifests (such as the application and radio local manifests, and the radio recovery manifest).
This gives the user flexibility in defining the device's behavior in recovery scenarios.
One possible scenario is reusing the regular radio image to download a damaged application image.

.. figure:: images/nrf54h20_recovery_invocation_topology.svg

   SUIT topology for the nRF54H20 SoC recovery path in the most complex predicted case.

Properties of user-controlled SUIT manifests configurable by the users
**********************************************************************

A user-controlled SUIT manifest must be explicitly enabled or configured to be functional.

The following tables contain *proposed* configuration parameter values selected to render R&D activities more convenient.

Root manifest
=============

+----------------------------------+--------------------------------------+-----------------------------------------------------------+
| Property                         | Default value                        | Note                                                      |
+==================================+======================================+===========================================================+
| Vendor ID                        | 7617daa5-71fd-5a85-8f94-e28d735ce9f4 | RFC4122 uuid5(uuid.NAMESPACE_DNS, 'nordicsemi.com')       |
+----------------------------------+--------------------------------------+-----------------------------------------------------------+
| Class ID                         | 3f6a3a4d-cdfa-58c5-acce-f9f584c41124 | RFC4122 uuid5(nordic_vid, 'nRF54H20_sample_root')         |
+----------------------------------+--------------------------------------+-----------------------------------------------------------+
| Downgrade prevention policy      | downgrade_prevention_disabled        |                                                           |
+----------------------------------+--------------------------------------+-----------------------------------------------------------+
| Independent updateability policy | independent_update_allowed           |                                                           |
+----------------------------------+--------------------------------------+-----------------------------------------------------------+
| Signature verification policy    | signature_check_disabled             |                                                           |
+----------------------------------+--------------------------------------+-----------------------------------------------------------+

Application local manifest 1
============================

+----------------------------------+--------------------------------------+-----------------------------------------------------------+
| Property                         | Default value                        | Note                                                      |
+==================================+======================================+===========================================================+
| Vendor ID                        | 7617daa5-71fd-5a85-8f94-e28d735ce9f4 | RFC4122 uuid5(uuid.NAMESPACE_DNS, 'nordicsemi.com')       |
+----------------------------------+--------------------------------------+-----------------------------------------------------------+
| Class ID                         | 08c1b599-55e8-5fbc-9e76-7bc29ce1b04d | RFC4122 uuid5(nordic_vid, 'nRF54H20_sample_app')          |
+----------------------------------+--------------------------------------+-----------------------------------------------------------+
| Downgrade prevention policy      | downgrade_prevention_disabled        |                                                           |
+----------------------------------+--------------------------------------+-----------------------------------------------------------+
| Independent updateability policy | independent_update_denied            |                                                           |
+----------------------------------+--------------------------------------+-----------------------------------------------------------+
| Signature verification policy    | signature_check_disabled             |                                                           |
+----------------------------------+--------------------------------------+-----------------------------------------------------------+

Radio local manifest 1
======================

+----------------------------------+--------------------------------------+-----------------------------------------------------------+
| Property                         | Default value                        | Note                                                      |
+==================================+======================================+===========================================================+
| Vendor ID                        | 7617daa5-71fd-5a85-8f94-e28d735ce9f4 | RFC4122 uuid5(uuid.NAMESPACE_DNS, 'nordicsemi.com')       |
+----------------------------------+--------------------------------------+-----------------------------------------------------------+
| Class ID                         | 816aa0a0-af11-5ef2-858a-feb668b2e9c9 | RFC4122 uuid5(nordic_vid, 'nRF54H20_sample_rad')          |
+----------------------------------+--------------------------------------+-----------------------------------------------------------+
| Downgrade prevention policy      | downgrade_prevention_disabled        |                                                           |
+----------------------------------+--------------------------------------+-----------------------------------------------------------+
| Independent updateability policy | independent_update_denied            |                                                           |
+----------------------------------+--------------------------------------+-----------------------------------------------------------+
| Signature verification policy    | signature_check_disabled             |                                                           |
+----------------------------------+--------------------------------------+-----------------------------------------------------------+

Properties of user-controlled SUIT manifests fixed in the SDFW implementation
*****************************************************************************

Root manifest
=============

+----------------------------------+--------------------------------------+-----------------------------------------------------------+
| Property                         | Proposed value                       | Note                                                      |
+==================================+======================================+===========================================================+
| Independent updateability policy | independent_update_allowed           |                                                           |
+----------------------------------+--------------------------------------+-----------------------------------------------------------+
| Signing Key ID Range             | MANIFEST_PUBKEY_OEM_ROOT_GEN0-2      | 0x4000AA00 - 0x4000AA02                                   |
+----------------------------------+--------------------------------------+-----------------------------------------------------------+
| Resource Access Rights           |                                      | Does not operate on local resources                       |
+----------------------------------+--------------------------------------+-----------------------------------------------------------+

Application recovery manifest
=============================

+----------------------------------+--------------------------------------+-----------------------------------------------------------+
| Property                         | Proposed value                       | Note                                                      |
+==================================+======================================+===========================================================+
| Independent updateability policy | independent_update_allowed           |                                                           |
+----------------------------------+--------------------------------------+-----------------------------------------------------------+
| Signing Key ID Range             | MANIFEST_PUBKEY_APPLICATION_GEN0-2   | 0x40022100 - 0x40022102                                   |
+----------------------------------+--------------------------------------+-----------------------------------------------------------+
| Resource Access Rights           |                                      | Ability to boot the App Core,                             |
|                                  |                                      | Memory access based on App Domain UICR                    |
+----------------------------------+--------------------------------------+-----------------------------------------------------------+

Application local manifest 1, 2, 3
==================================

+----------------------------------+--------------------------------------+-----------------------------------------------------------+
| Property                         | Proposed value                       | Note                                                      |
+==================================+======================================+===========================================================+
| Signing Key ID Range             | MANIFEST_PUBKEY_APPLICATION_GEN0-2   | 0x40022100 - 0x40022102                                   |
+----------------------------------+--------------------------------------+-----------------------------------------------------------+
| Resource Access Rights           |                                      | Ability to boot the App Core,                             |
|                                  |                                      | Memory access based on App Domain UICR                    |
+----------------------------------+--------------------------------------+-----------------------------------------------------------+

Radio Recovery Manifest, Radio Local manifest 1, 2
==================================================

+----------------------------------+--------------------------------------+-----------------------------------------------------------+
| Property                         | Proposed value                       | Note                                                      |
+==================================+======================================+===========================================================+
| Signing Key ID Range             | MANIFEST_PUBKEY_RADIO_GEN0-2         | 0x40032100 - 0x40032102                                   |
+----------------------------------+--------------------------------------+-----------------------------------------------------------+
| Resource Access Rights           |                                      | Ability to boot the Radio Core,                           |
|                                  |                                      | Memory access based on radio domain UICR                  |
+----------------------------------+--------------------------------------+-----------------------------------------------------------+

Properties of Nordic-controlled SUIT manifests
**********************************************

All values given in this section are hard-coded in the SDFW and cannot be altered by the user.
Support for all the manifests described in the following tables are enabled by default.

Nordic top manifest
===================

+----------------------------------+--------------------------------------------+-----------------------------------------------------------+
| Property                         | Default value                              | Note                                                      |
+==================================+============================================+===========================================================+
| Vendor ID                        | 7617daa5-71fd-5a85-8f94-e28d735ce9f4       | RFC4122 uuid5(uuid.NAMESPACE_DNS, 'nordicsemi.com')       |
+----------------------------------+--------------------------------------------+-----------------------------------------------------------+
| Class ID                         | f03d385e-a731-5605-b15d-037f6da6097f       | RFC4122 uuid5(nordic_vid, 'nRF54H20_nordic_top')          |
+----------------------------------+--------------------------------------------+-----------------------------------------------------------+
| Downgrade prevention policy      | downgrade_prevention_enabled               |                                                           |
+----------------------------------+--------------------------------------------+-----------------------------------------------------------+
| Independent updateability policy | independent_update_allowed                 |                                                           |
+----------------------------------+--------------------------------------------+-----------------------------------------------------------+
| Signature verification policy    | signature_check_enabled_on_update_and_boot |                                                           |
+----------------------------------+--------------------------------------------+-----------------------------------------------------------+
| Resource Access Rights           |                                            | Does not operate on local resources                       |
+----------------------------------+--------------------------------------------+-----------------------------------------------------------+

System Controller manifest
==========================

+----------------------------------+--------------------------------------------+-----------------------------------------------------------+
| Property                         | Default value                              | Note                                                      |
+==================================+============================================+===========================================================+
| Vendor ID                        | 7617daa5-71fd-5a85-8f94-e28d735ce9f4       | RFC4122 uuid5(uuid.NAMESPACE_DNS, 'nordicsemi.com')       |
+----------------------------------+--------------------------------------------+-----------------------------------------------------------+
| Class ID                         | c08a25d7-35e6-592c-b7ad-43acc8d1d1c8       | RFC4122 uuid5(nordic_vid, 'nRF54H20_sys')                 |
+----------------------------------+--------------------------------------------+-----------------------------------------------------------+
| Downgrade prevention policy      | downgrade_prevention_enabled               |                                                           |
+----------------------------------+--------------------------------------------+-----------------------------------------------------------+
| Independent updateability policy | independent_update_denied                  |                                                           |
+----------------------------------+--------------------------------------------+-----------------------------------------------------------+
| Signature verification policy    | signature_check_enabled_on_update_and_boot |                                                           |
+----------------------------------+--------------------------------------------+-----------------------------------------------------------+
| Resource Access Rights           |                                            | Ability to boot the System Controller,                    |
+----------------------------------+--------------------------------------------+-----------------------------------------------------------+

Secure Domain manifest
======================

+----------------------------------+--------------------------------------------+-----------------------------------------------------------+
| Property                         | Proposed value                             | Note                                                      |
+==================================+============================================+===========================================================+
| Vendor ID                        | 7617daa5-71fd-5a85-8f94-e28d735ce9f4       | RFC4122 uuid5(uuid.NAMESPACE_DNS, 'nordicsemi.com')       |
+----------------------------------+--------------------------------------------+-----------------------------------------------------------+
| Class ID                         | d96b40b7-092b-5cd1-a59f-9af80c337eba       | RFC4122 uuid5(nordic_vid, 'nRF54H20_sec')                 |
+----------------------------------+--------------------------------------------+-----------------------------------------------------------+
| Downgrade prevention policy      | downgrade_prevention_enabled               |                                                           |
+----------------------------------+--------------------------------------------+-----------------------------------------------------------+
| Independent updateability policy | independent_update_denied                  |                                                           |
+----------------------------------+--------------------------------------------+-----------------------------------------------------------+
| Signature verification policy    | signature_check_enabled_on_update_and_boot |                                                           |
+----------------------------------+--------------------------------------------+-----------------------------------------------------------+
| Resource Access Rights           |                                            | Ability to trigger installation of SDFW, SDFW_UPDATE      |
+----------------------------------+--------------------------------------------+-----------------------------------------------------------+
