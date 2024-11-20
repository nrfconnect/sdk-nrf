.. _ug_nrf54h20_architecture_lifecycle:

nRF54H20 lifecycle states
#########################

.. contents::
   :local:
   :depth: 2

The Secure Domain ROM firmware defines the lifecycle states (LCS) for the nRF54H20 SoC.
The states are based on the Arm PSA Security Model and allow for programming and safely erasing the device assets.

.. note::
    During the customer sampling, the LCS of the nRF54H20 SoC must be set to Root of Trust (RoT).
    If the LCS is set to ``EMPTY``, it must be transitioned to ``RoT``.
    For more information, see :ref:`ug_nrf54h20_gs_bringup`.

    However, the forward transition to LCS ``RoT`` is permanent.
    After the transition, it is not possible to transition backward to LCS ``EMPTY``.

The LCS available are the following:

.. list-table:: nRF54H20 lifecycle states
   :header-rows: 1
   :align: center
   :widths: auto

   * - LCS
     - Supply chain stage
     - Description
   * - EMPTY
     - Production
     - RAM empty and Secure Domain Firmware unprogrammed.
   * - Root of Trust
     - Production
     - Secure Domain Firmware and certificates provisioned.
   * - DEPLOYED
     - In-field
     - Secure Domain debug access port is locked, and unlock is only possible using an authenticated operation.
   * - ANALYSIS
     - End-of-life
     - All device assets in MRAM are erased to allow for Nordic RMA procedures.
   * - DISCARDED
     - End-of-life
     - All device assets in MRAM are erased.

See the following diagram:

.. figure:: images/nRF54H20_lifecycle_states.svg
   :alt: nRF54H20 lifecycle states and transitions

   nRF54H20 lifecycle states and transitions available on the final silicon.

This figure shows the states and transitions (both forward and backward ones) that will be available on the final silicon.

Changing the lifecycle state will be useful during development.
Test devices in their final configuration would require the device to be in the deployed state, however, updating the Secure Domain firmware and the System Controller firmware will be easier with the device in RoT state.
