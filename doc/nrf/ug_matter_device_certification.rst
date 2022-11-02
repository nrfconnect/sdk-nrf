.. _ug_matter_device_certification:

Matter certification
####################

.. contents::
   :local:
   :depth: 2

At some point in the future, Matter will offer full certification, including test specification, certification program, and certification framework.

.. _ug_matter_device_certification_reqs:

Certification requirements
**************************

.. ug_matter_certification_sdo_start

Because Matter is an application layer, it relies on proven technologies for network connectivity.
These technologies come with their own certification processes governed by different Standard Development Organizations (SDOs).

.. ug_matter_certification_sdo_end

Matter over Thread certification requirements
=============================================

The following table lists Matter over Thread certification requirements for when a product moves to production.

+-------------------------------+---------------------------+-----------------------------+----------------------------------------+
| Technology certification      | Stage for certification   | SDO to join                 | Minimum SDO membership level required  |
+===============================+===========================+=============================+========================================+
| Bluetooth QDID                | Production                | `Join Bluetooth SIG`_       | Adopter                                |
+-------------------------------+---------------------------+-----------------------------+----------------------------------------+
| Thread Group Certification    | Production                | `Join Thread Group`_        | Implementer                            |
+-------------------------------+---------------------------+-----------------------------+----------------------------------------+
| Matter Certification          | Production                | `Join CSA`_                 | Adopter                                |
+-------------------------------+---------------------------+-----------------------------+----------------------------------------+

Bluetooth and Thread certifications can be inherited from Nordic Semiconductor (see the following section).
Matter certification requires testing performed at Authorized Test Laboratory (ATL).

To read more about Matter and dependent certification, get familiar with CSA's Certification Policy and Procedures document.

.. _ug_matter_device_certification_reqs_security:

Matter Attestation of Security
==============================

For a Matter end product to be certified, CSA's policies require an Attestation of Security that provides detailed information about the security level of the end product.
The attestation document lists robustness security requirements based on the Matter specification.
The product developer must indicate the level of compliance and briefly justify the choice.

The attestation must be filled by the person responsible for end product certification who meets the following requirements:

* The person's organization is a `member of the Matter community <Join CSA_>`_.
* The person has an account on the `Connectivity Standards Alliance Certification Web Tool`_.

Once both these requirements are met, the responsible person can download the `Matter Attestation of Security template`_, fill it in, and submit it in the certification web tool when applying for the certification.

Matter certification inheritance
********************************

If your product uses qualified Bluetooth stack and certified Thread libraries provided as part of the |NCS|, you can *inherit* certification from Nordic Semiconductor, provided that you do not introduce any changes to these stacks.
In practice, this means reusing Nordic Semiconductor's certification identifiers, which were obtained as a result of the official certification procedures.

Nordic Semiconductor provides the following certification identifiers:

* Bluetooth Qualified Design IDs (Bluetooth QDIDs) - obtained in accordance with `Bluetooth SIG's Qualification Process`_.
* Thread Certification IDs (Thread CIDs) - obtained in accordance with `Thread Group's certification information`_.

You can visit the following pages on Nordic Semiconductor Infocenter to check the Bluetooth QDIDs and Thread CIDs valid for SoCs that support Matter applications:

* `nRF5340 DK Compatibility Matrix`_
* `nRF52840 DK Compatibility Matrix`_

.. note::
   The inheritance is granted by the related SDO after you join the organization and apply for certification by inheritance.
   The procedure differs from SDO to SDO.
   For details, contact the appropriate certification body in the SDO.
