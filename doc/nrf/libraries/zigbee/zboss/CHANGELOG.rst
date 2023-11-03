.. _zboss_changelog:

Changelog
#########

.. contents::
   :local:
   :depth: 2

All notable changes to this project in the |NCS| are documented in this file.

nRF Connect SDK v2.3.0
**********************

Updated
=======

* The ZBOSS stack to version ``3.11.2.1+5.1.3``.
  For detailed information, see `ZBOSS stack release notes`_ for the |NCS| v2.3.0 release.

Bug fixes
=========

* Fixed an issue where buffer would not be freed at the ZC after a secure rejoin of a ZED.

nRF Connect SDK v2.0.0
**********************

All the notable changes included in the |NCS| v2.0.0 release are documented in this section.

Added
=====

* New ZBOSS API addons, allowing to declare ZCL8-compatible devices.

Updated
=======

* The ZBOSS stack to version ``3.11.2.0+5.1.2``.
  For detailed information, see `ZBOSS stack release notes`_ for the |NCS| v2.0.0 release.
* The development ZBOSS stack to version ``3.12.1.0+5.2.0``.
  For detailed information, see `ZBOSS stack release notes`_ for the |NCS| v2.0.0 release.
* ZCL in all variants of ZBOSS to specification version 8.
  This version is backward-compatible with earlier versions of ZCL.
* Improved the NCP communication stability.

Removed
=======

* Precompiled libraries for experimental Zigbee Green Power Proxy Basic functionality for End Device role.
* Legacy HA device definitions due to ZCL8 incompatibilities.
  The sample device definitions can be found inside the :file:`include` directory in every Zigbee sample and application.

Bug fixes
=========

* Fixed an issue where incorrect Read Attributes Response would be sent on reading multiple attributes when the first attribute is unsupported.

nRF Connect SDK v1.9.0
**********************

All the notable changes included in the |NCS| v1.9.0 release are documented in this section.

Added
=====

* Development ZBOSS stack library version based on the ZBOSS build v3.11.1.177+v5.1.1.
  This library version is dedicated for testing ZCL v8 features.
* ZBOSS libraries variant with ZBOSS Traces enabled.
* ZBOSS signal on Permit Joining Request.
* IdentifyTime parameter to zb_bdb_finding_binding_target().

Updated
=======

* Updated the ZBOSS stack to version ``3.11.1.0+5.1.1``.
  For detailed information, see `ZBOSS stack release notes`_ for the |NCS| v1.9.0.
* Updated BDB to specification version ``3.0.1``.
* Replaced ZB_IC_GET_TYPE_FROM_REC macro with ZB_IC_GET_TYPE_FROM_OPT.

Bug fixes
=========

* Fixed an issue where ZBOSS based device fails OTA Test Case OTA-TC-17C.
* Fixed an issue where ZCL Occupancy Sensing cluster was missing an implementation.
* Fixed an issue where device association fails if the Request Key packet is retransmitted.
* Fixed an issue where a device enters undefined state after leave request, sent right after successful association.

nRF Connect SDK v1.8.0
**********************

All the notable changes included in the |NCS| v1.8.0 release are documented in this section.

Added
=====

* Added APS command duplicate detection.
* Added NCP protocol version checks.

Updated
=======

* Updated the ZBOSS stack to version ``3.9.0.1+4.1.0``.
  For detailed information, see `ZBOSS stack release notes`_ for the |NCS| v1.8.0.
* Removed precompiled libraries for Zigbee Green Power Combo Basic functionality.

Bug fixes
=========

 * Fixed an issue where Zigbee Router would go out of capacity due to too many joins or rejoins.
 * Fixed an issue where unsolicited NCP commands would not be handled correctly.

nRF Connect SDK v1.7.0
**********************

All the notable changes included in the |NCS| v1.7.0 release are documented in this section.

Changes
=======

* Added API for reading active neighbor list.
* Extended NCP protocol with vendor-specific commands set.
* Updated the ZBOSS stack to version ``3.8.0.1+4.0.0``.
  For detailed information, see `ZBOSS stack release notes`_ for the |NCS| v1.7.0.

nRF Connect SDK v1.6.0
**********************

Changes
=======

* Reduced the number of ZBOSS libraries to a single set.
* Added sources of the BDB and ZCL layers and removed them from the main ZBOSS library.
* Moved scheduler API prototypes to :file:`zb_osif.h`.
* Added API for sending and receiving inter-PAN frames.
* Extended NCP protocol with inter-PAN commands.
* Added bootloader commands in the NCP protocol.
* Updated the ZBOSS stack to version ``3.6.0.0+3.0.0``.
  For detailed information, see `ZBOSS stack release notes`_ for the |NCS| v1.6.0.

nRF Connect SDK v1.5.1
**********************

Changes
=======

* Updated the production ZBOSS stack to version ``3.3.0.7+03_22_2021``.
  This certification-ready version includes several bugfixes.
  For detailed information, see `ZBOSS stack release notes`_ for the |NCS| v1.5.1.

nRF Connect SDK v1.5.0
**********************

Added
=====

* Development ZBOSS stack library version based on the ZBOSS build v3.5.0.0.
  This library version is dedicated for :ref:`NCP development <nrf:ug_zigbee_platform_design_ncp_details>`.
* Development libraries for the Cortex-M33 CPU.

Changes
=======

* Updated the production ZBOSS stack to version ``3.3.0.6+11_30_2020``.
  For detailed release notes for this stack version, see `ZBOSS stack release notes`_ for the |NCS| v1.5.0.

nRF Connect SDK v1.4.0
**********************

Added
=====

* Added ZBOSS changelog.

Changes
=======

* Updated ZBOSS stack to version ``10_06_2020``.
  For detailed release notes for this stack version, see `ZBOSS stack release notes`_ for the |NCS| v1.4.0.
* Unified OSIF layer for LEDs and buttons.
* Updated the ZBOSS release naming convention for nrfxlib commit.
* Changed MAC LL API.
* ZBOSS IO buffer size reconfigured to allow for sending 802.15.4 MAC frame with maximum payload.

Limitations
***********

This sections lists limitations that apply to all versions of the |NCS|.

* On average, ZBOSS alarms last longer by 6.4 percent than Zephyr alarms.
* The maximum size of a reportable attribute is 8 bytes.
