.. _ug_wifi_certification:

Wi-Fi certification
###################

.. contents::
   :local:
   :depth: 2

The Wi-Fi Alliance® offers a full `certification program <Wi-Fi Certification_>`_ that validates the interoperability of a Wi-Fi® end product with other Wi-Fi certified equipment.
The program offers three different paths of certification to Contributor-level `members of the Wi-Fi Alliance <Join Wi-Fi Alliance_>`_.
Implementer-level members can take advantage of the program by implementing Wi-Fi modules that have been previously certified by a Contributor-level member, for example, one of the `Nordic third-party modules`_.

Qualified Solution
******************

Nordic Semiconductor supports programs and paths to create Qualified Solutions that you can use to accelerate the Wi-Fi certification of their end products.
As a solution provider, it creates Qualified Solutions and Qualified Solution Variants for its product evaluation platforms based on the nRF70 Series devices and the |NCS|.
This enables you to use the QuickTrack Wi-Fi certification process.

Wi-Fi certification paths
*************************

An end product based on an nRF70 Series device can achieve Wi-Fi certification through the QuickTrack or Derivative paths.
The QuickTrack certification path is applicable for all end products designed from either the Nordic Semiconductor Qualified Solutions or Qualified Solution Variant evaluation platforms, where limited modifications are allowed.
Derivative certification is applicable for end products based on third-party Wi-Fi CERTIFIED™ modules containing an nRF70 Series IC without any modification.

For details about the Wi-Fi certification program and the available paths for the nRF70 Series, read the `Wi-Fi Alliance Certification for nRF70 Series`_ document.

For instructions on how to specifically use the QuickTrack certification path, see the :ref:`wifi_wfa_qt_app_sample` page.

Recertification process after end product software upgrade
**********************************************************

When your Wi-Fi CERTIFIED end product receives an update with new software corresponding to a newer |NCS| release, you must initiate the Wi-Fi CERTIFIED end product variant process.
The certification process is controlled by the Wi-Fi Alliance.

Under the QuickTrack certification path, the recertification process involves rerunning the QuickTrack Wi-Fi certification testing on the end product with the updated software and basing it on the Qualified Solution or variant from Nordic Semiconductor corresponding to that version of the |NCS| software.

Under the Derivative certification path, the recertification process does not involve any additional certification testing on the end product.
You need to reapply for the new derivative certification, basing it on the new qualified product variant as required.

See the following links for more information on the certification process:

* `QuickTrack New Product Application Training`_
* `QuickTrack Member Conformance Test Laboratory Setup`_
* `Wi-Fi_CERTIFIED_Derivative_Certifications_Overview`_
