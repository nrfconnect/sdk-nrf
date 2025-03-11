.. _ug_tfm_index:

Trusted Firmware-M in the |NCS|
###############################

.. contents::
   :local:
   :depth: 2

Nordic Semiconductor recommends following `Platform Security Architecture (PSA)`_ for product development to ensure appropriate security implementation in IoT devices.

Trusted Firmware-M (TF-M) is the reference implementation of PSA, which follows `PSA Certified IoT Security Framework`_ for securing connected devices.
For more information about the framework, see the :ref:`ug_psa_certified_api_overview` page.

TF-M provides a reference design of a Secure Processing Environment (SPE) for Arm M-profile architectures.
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
   psa_certified_api_overview
   tfm
