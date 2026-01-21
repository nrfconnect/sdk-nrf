.. _nrf53_audio_app_dk_legal:
.. _nrf53_audio_feature_support:

nRF5340 Audio feature support and QDIDs
#######################################

.. contents::
   :local:
   :depth: 2

The following table lists features of the nRF5340 Audio application and their respective limitations and maturity level.
For an explanation of the maturity levels, see :ref:`Software maturity levels <software_maturity>`.

.. note::
   Features not listed are not supported.

.. include:: /releases_and_maturity/software_maturity.rst
   :start-after: software_maturity_application_nrf5340audio_table:
   :end-before: software_maturity_protocol

.. _nrf5340_audio_dns_and_qdids:

nRF5340 Audio DNs and QDIDs
***************************

See `nRF5340 Bluetooth DNs and QDIDs Compatibility Matrix`_ for the Design Numbers (DNs) and Qualified Design IDs (QDIDs) for nRF5340 LE Audio applications.

A full Audio product DN will typically require DNs and QDIDs for the Controller component, Host component, Profiles, and Services component and LC3 codec component.
The exact DN/QDID numbers depend on the project configuration and the features used in the application.

.. note::
   * The DNs/QDIDs listed in the Compatibility Matrix might not cover all use cases or combinations.
     The full details of what is supported by a given DN or QDID can be found in the associated Implementation Conformance Statement (ICS).

   * The Audio applications do not demonstrate the full capabilities of the underlying DNs and QDIDs.
     At the same time, the Audio applications may demonstrate features not available in the underlying DNs or QDID.
