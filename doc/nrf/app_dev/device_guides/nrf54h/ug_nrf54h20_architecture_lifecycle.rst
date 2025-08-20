.. _ug_nrf54h20_architecture_lifecycle:

nRF54H20 SoC lifecycle states
#############################

.. contents::
   :local:
   :depth: 2

The Secure Domain ROM defines the lifecycle states (LCS) for the nRF54H20 SoC.
The states are based on the Arm PSA Security Model and allow for programming and safely erasing the device assets.

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

   nRF54H20 lifecycle states and transitions available on the nRF54H20 SoC.

You can change the SoC lifecycle state to streamline development and testing:

* During application development, set the SoC to the ``Root of Trust`` (RoT) state.
* To validate behavior in a production environment, use the ``DEPLOYED`` state.


If the device is in LCS ``EMPTY``, transition it to LCS ``RoT`` by following the :ref:`nRF54H20 DK bring-up <ug_nrf54h20_gs_bringup>` procedure.

.. caution::
   The transition from ``EMPTY`` to ``RoT`` is permanent and cannot be reversed.

For more information, see the following pages:

* :ref:`ug_nrf54h20_gs`
* :ref:`ug_nrf54h20_custom_pcb`
* :ref:`ug_nrf54h20_keys`
