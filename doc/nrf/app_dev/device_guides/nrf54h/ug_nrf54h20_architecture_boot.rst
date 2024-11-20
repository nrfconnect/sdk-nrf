.. _ug_nrf54h20_architecture_boot:

nRF54H20 Boot Sequence
######################

.. contents::
   :local:
   :depth: 2

The nRF54H20 SoC performs its startup procedure from cold boot on the following events:

 * Power on reset
 * Pin reset
 * System Controller watchdog reset
 * Ctrl-AP reset (debugging Secure Domain or System Controller)
 * Secure Domain reset request
 * Secure Domain watchdog reset
 * Tamper detection

If the system was active and entered the Soft Off sleep state, the system performs the cold boot procedure on the following events:

  * GPIO event
  * LPCOMP event
  * NFC field detection
  * USB VBUS rising
  * Entering the debug interface mode
  * GRTC event

During cold boot the system has no (or little application-defined) state retained in its RAM memory or hardware registers and performs full initialization.

Cold Boot Sequence
******************

The nRF54H20 boot sequence has two key features:

* An immutable boot ROM provides the initial :term:`Root of Trust (RoT)`.
  This boot ROM is responsible for verifying the secure domain firmware signature before allowing the code to be executed.
* The secure domain acts as the boot master in the system.
  It completes the allocation of all the global resources before any other local domain is allowed to execute.
  This order of operation is needed for robustness and security of the system.
  It ensures that global resources are allocated for other local domains before any of them has opportunity to access global resources (correct access to allocated resources, or tries of malicious access to resources owned by other domains).

Boot stages
***********

.. to review

The Secure Domain boots the System Controller, the application core, and the radio core:

* VPRs (PPR, FLPR) are started by their owners when the owners decide.
* PPR and FLPR are owned by the application core in most applications, but any of them can be reassigned to the radio core.

See the following overview of the boot sequence, where the left-most block indicates what starts first from when power-on reset is applied.

.. figure:: images/nRF54H20_bootsequence.svg
   :alt: Boot sequence of the nRF54H20

   Boot order of the nRF54H20

The following is a description of the boot sequence steps:

1. Immediately after reset, the SysCtrl CPU starts executing code from a ROM memory in Global Domain.
   This ROM code perform chip calibration, like trim and power-up the MRAM macro.
   This ROM code does not affect the runtime services offered by SysCtrl firmware during the secondary boot stage.

#. The SysCtrl ROM powers up the secure domain and then halts.

#. The secure domain is taken out of reset (as the first local domain), and the Cortex-M33 CPU inside the secure domain automatically starts executing code from a local ROM memory.
   As the MRAM is now calibrated and working correctly, the secure domain ROM can perform signature verification of the secure domain firmware components installed into the MRAM.
   The secure domain ROM also configures the device according to the current life-cycle state (LCS) and extracts silicon-unique fingerprints from the physical unclonable function (PUF) in GD , retaining this inside CRACEN.

#. If the secure domain firmware signature is valid, the secure domain ROM reconfigures ROM memory as non-executable and non-readable and then branches into the firmware stored in MRAM.
   This is the first step of the primary boot stage where a user-installable firmware component is executed by any CPU in the system.

#. At the end of the primary boot stage, the secure domain firmware configures and restricts access to all global resources, and initiates the secondary boot stage.

#. In the secondary boot stage, SysCtrl and other local domains are released from reset and in parallel start to execute their respective MRAM firmware components.

#. Each of the local domains is responsible to configure its local resources partitioning.
