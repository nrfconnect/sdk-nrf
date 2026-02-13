.. _ug_tfm:
.. _ug_tfm_index:

Trusted Firmware-M in the |NCS|
###############################

Nordic Semiconductor recommends following `Platform Security Architecture (PSA)`_ for product development to ensure appropriate security implementation in IoT devices.

Trusted Firmware-M (TF-M) is the reference implementation of PSA, which follows `PSA Certified IoT Security Framework`_ for securing connected devices.
For more information about the framework, see the :ref:`ug_psa_certified_api_overview` page.

TF-M provides a reference design of a Trusted Execution Environment (TEE) for Arm M-profile architectures.
Using a highly configurable set of software components, it creates the Secure Processing Environment (SPE), which relies on security by separation to protect sensitive assets and code.
TF-M also provides a set of secure runtime services to the application, such as Protected Storage, Cryptography, and Attestation.
Additionally, secure boot through MCUboot in TF-M ensures integrity of runtime software and supports firmware upgrade.

`ARM TrustZone`_ technology included in Nordic Semiconductor's SoCs that implement the Armv8-M architecture (such as nRF5340, most of the nRF54L Series or the nRF91 Series) provides hardware-enforced separation of the Secure and Non-secure Processing Environments (SPE and NSPE, respectively) into Trusted and Non-Trusted worlds.

The TF-M implementation in the |NCS| is demonstrated in the following samples:

* All :ref:`tfm_samples` in this SDK
* All :ref:`cryptography samples <crypto_samples>` in this SDK
* A series of :zephyr:code-sample-category:`tfm_integration` samples available in Zephyr (these include :ref:`ug_tfm_supported_services_tfm_services` from the |NCS| when they are built from the |NCS| context)

|samples_tfm_info|

The pages in this section describe the architecture and configuration of TF-M in the |NCS|.
For more information about TF-M, see the `Trusted Firmware-M documentation <TF-M documentation_>`_, which is oriented towards TF-M implementation developers.

.. toctree::
   :maxdepth: 2
   :caption: Subpages:

   tfm_supported_services
   tfm_architecture
   processing_environments
   tfm_building
   tfm_services
   tfm_provisioning
   tfm_logging
