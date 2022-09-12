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
