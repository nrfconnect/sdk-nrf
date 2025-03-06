.. _ug_tfm_index:

Trusted Firmware-M in the |NCS|
###############################

.. contents::
   :local:
   :depth: 2

Nordic Semiconductor recommends following `Platform Security Architecture (PSA)`_ for product development to ensure appropriate security implementation in IoT devices.
PSA offers the `PSA Certified IoT Security Framework`_ for securing connected devices, which consists of four steps:

* Analyze the threats that have the potential to compromise your device and generate a set of security requirements based on these risks.
* Architect the right level of security for your product by using unique security requirements to identify and select components and specifications.
* Implement the trusted components and firmware, making use of high-level APIs to build-in security and create an interface to the hardware Root of Trust (RoT).
* Certify device, platform, or silicon by following independent security evaluation.

Trusted Firmware-M (TF-M) is the reference implementation of PSA.
It provides a reference design of a Secure Processing Environment (SPE) for Arm M-profile architectures.
The SPE relies on security by separation to protect sensitive assets and code.
TF-M also provides security services to the application, such as Protected Storage, Cryptography, and Attestation.

`ARM TrustZone`_ technology included in Nordic Semiconductor's SoCs that implement the Armv8-M architecture (such as nRF5340, the nRF54L Series or the nRF91 Series) provides hardware-enforced separation of the Secure and Non-secure Processing Environments (SPE and NSPE, respectively) into Trusted and Non-Trusted worlds.

Starting from the |NCS| v2.0.0, TF-M is enabled by default for applications and samples that support hardware-enforced separation of the SPE and the NSPE.

The pages in this section describe the architecture and configuration of TF-M in the |NCS|.
For more information about TF-M, see the `Trusted Firmware-M documentation <TF-M documentation_>`_, which is oriented towards TF-M developers.

.. important::
   Currently, only the :ref:`Minimal TF-M configuration <tfm_minimal_build>` is :ref:`supported <software_maturity_security_features_tfm>` in the |NCS|.
   Configuring TF-M to use features beyond the minimal configuration (with so called :ref:`tfm_configurable_build`) is :ref:`experimental <software_maturity_security_features_tfm>`.

.. toctree::
   :maxdepth: 2
   :caption: Subpages:

   tfm_architecture
   processing_environments
   tfm
