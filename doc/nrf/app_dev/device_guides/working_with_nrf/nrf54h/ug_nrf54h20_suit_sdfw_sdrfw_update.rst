.. _ug_nrf54h20_suit_sdfw_sdrfw_update:

Secure Domain Firmware Update
#############################

.. contents::
   :local:
   :depth: 2


Both the Secure Domain Firmware (SDFW) and the Secure Domain Recovery Firmware (SDRFW) are launched on a core responsible for establishing the Root of Trust as a Secure Domain.
Due to security requirements, the update process for these binaries differs significantly from regular updates in local domains.

To ensure that trusted execution can be established on the device, a valid SDRFW must be present on the device before performing a SDFW update.
An update is initiated only if the update candidate is verified by the Secure Domain ROM (SDROM) using a key that matches the requested update type.

.. note::
   The bundle that the user programs already contains both SDFW image and SDRFW image.

The SUIT framework allows flexible firmware launching and updating while keeping parts of the codebase unmodifiable by end users.
A common approach is to use a platform-specific layer that maps firmware blocks to device memory ranges, simplifying updates for basic systems.
Manifests can describe actions required to launch or update each firmware block.
In the nRF54H20, this approach is used in some areas, but the SDFW and the SDRFW require the SDROM to handle launching and updates, rather than direct copying to a destination address.


Implementation
**************

SUIT manifests use components to define executable or updatable entities.
On a given platform, components may have different characteristics that require varied approaches for launching or updating (for example, execution in place instead of copying to RAM).

As described in :ref:`suit_component_types`, a special SUIT component type (``SOC_SPEC``) describes components that are controlled by Nordic Semiconductor.

For the nRF54H20 SoC, there are two components of this type: one for the SDFW and the other for the SDRFW.
These are identified by component IDs 1 and 2, respectively:

* ``SOC_SPEC/1`` - Secure Domain Firmware
* ``SOC_SPEC/2`` - Secure Domain Recovery Firmware

As shown in :ref:`suit_default_manifest_topology_for_the_nrf54h20_soc`, both components are included in a single SUIT manifest.
This manifest is managed by the ``Nordic top`` manifest, which is, in turn, controlled by the ``Root`` manifest.

Update process
**************

The update process begins by attempting to update the SDRFW image.
If this update fails, the process still proceeds with updating the SDFW.
To prevent ambiguity if one slot update fails while the other succeeds, the manifest version reflects the version of the SDFW.

.. note::
   Every update attempt involves system reboot. As a result, multiple passes through the root manifest during update sequences are expected.

.. figure:: images/nrf54h20_suit_sdfw_sdrfw_update_flow.svg
   :alt: Secure Domain Firmware and Secure Domain Recovery Firmware update flow

   Update flow for the Secure Domain Firmware and the Secure Domain Recovery Firmware

More information about strategy, configuration and update of Nordic components can be found under :ref:`ug_nrf54h20_suit_soc_binaries`.
