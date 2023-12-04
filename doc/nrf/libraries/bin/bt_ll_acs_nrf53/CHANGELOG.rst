.. _bt_ll_acs_nrf53_changelog:

Changelog
#########

.. contents::
   :local:
   :depth: 2

Notable changes to this controller are documented in this file.

Controller v3424
****************

The following changes have been introduced in the v3424 of the controller:

Changes
=======

* Improved robustness of the timestamps (the SDU references) received together with the ISO data.
* Changed offset of the timestamps (the SDU references) received together with the ISO data to get correct presentation delay.
* Freed more pins for front-end module (FEM) control.
* Improved ACL and BIS coexistence on the same device.
* Fixed an issue where the peripheral did not always accept timing values from the same central.
* Fixed an issue where the PA sync skip parameter was not used.
* Fixed an issue where it was not possible to create BIG sync after terminating a pending BIG sync.

Controller v3393
****************

The following changes have been introduced in the v3393 of the controller:

Changes
=======

* Updated the controller code with major changes that improve performance on many levels.
* Fixed all failing EBQ tests for claiming the QDID.
* Fixed an issue where the timing of the TXEN FEM pin was wrong and would cause unwanted radio noise.
* Fixed timestamps that are returned when using the LE_READ_ISO_TX_SYNC HCI command.


Controller v3357
****************

The following changes have been introduced in the v3357 of the controller:

Changes
=======

* Updated the controller to be able to read RSSI from the CIS ISO channel using the ``HCI_Read_RSSI`` HCI command.
* Fixed an issue where pins P0.28 to P0.31 cannot be allocated for controlling front-end modules (FEMs).
* Fixed an issue where the FEM control pins cannot work when the TX output power setting equals 0 dBm.


Controller v3349
****************

The following changes have been introduced in the v3349 of the controller:

Changes
=======

* Fixed the Direct Test Mode (DTM), which was broken after version 3307.
* Fixed an issue where an update to the connection parameter could lead to a disconnection.
* Fixed an issue where the "stream stopped" and "disconnected" events would not be triggered.
* Fixed an issue where the controller would stop responding when a disconnection happened.


Controller v3330
****************

The following changes have been introduced in the v3330 of the controller:

Changes
=======

* Fixed issue where resetting one headset caused the other to disconnect.


Controller v3322
****************

The following changes have been introduced in the v3322 of the controller:

Changes
=======

* Improvements to support creating CIS connections in any order.
* Changes to accommodate BIS + ACL combinations.
* Basic support for interleaved broadcasts.


Known issues
************

See the :ref:`nRF5340 Audio application known issues <known_issues_nrf5340audio>` for the list of known issues for the controller.


Limitations
***********

The controller is closely related to the nRF5340 Audio application.
See the :ref:`nRF5340 Audio application feature support table <software_maturity_application_nrf5340audio_table>` for the list of supported features and limitations.
