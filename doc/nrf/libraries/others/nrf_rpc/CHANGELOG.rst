.. _nrf_rpc_changelog:

Changelog
#########

.. contents::
   :local:
   :depth: 2

All the notable changes to this project are documented on this page.

nRF Connect SDK v2.5.0
**********************

Bug fixes
=========

* Fixed an issue where the :c:func:`nrf_rpc_cbor_cmd_rsp` function decodes only the first response element.

Changes
=======

* Enabled the nRF RPC library to be initialized more than once.

nRF Connect SDK v2.2.0
**********************

All the notable changes added to the |NCS| v2.2.0 release are documented in this section.

Added
=====

* Added the :kconfig:option:`CONFIG_NRF_RPC_ZCBOR_BACKUPS` Kconfig option for setting up the number of zcbor backups.

Changes
=======

* Enabled independent initialization of transport in each group.
  This makes RPC available for the groups that are ready, even though initialization of others may have failed.

nRF Connect SDK v2.1.0
**********************

All the notable changes added to the |NCS| v2.1.0 release are documented in this section.

Added
=====

* Added support for multi-instance transport.
  Each group can use a different transport instance, or groups can share a single transport instance.
* Added support for the nRF RPC protocol versioning.

Changes
=======

* Updated documentation.

nRF Connect SDK v2.0.0
**********************

All the notable changes added to the |NCS| v2.0.0 release are documented in this section.

Changes
=======

* Updated figures to follow style guidelines.
* Moved from TinyCBOR to zcbor.

nRF Connect SDK v1.9.0
**********************

No changes in this release.

nRF Connect SDK v1.8.0
**********************

All the notable changes added to the |NCS| v1.8.0 release are documented in this section.

Changes
=======

* Removed Latin abbreviations from the documentation.

nRF Connect SDK v1.7.0
**********************

All the notable changes added to the |NCS| v1.7.0 release are documented in this section.

Changes
=======

* Fixed formatting of kconfig options.

nRF Connect SDK v1.6.0
**********************

All the notable changes added to the |NCS| v1.6.0 release are documented in this section.

Changes
=======

* Renamed :file:`nrf_ernno.h` to :file:`nrf_rpc_ernno.h`.

nRF Connect SDK v1.5.0
**********************

All the notable changes added to the |NCS| v1.5.0 release are documented in this section.

Changes
=======

* Removed "BSD" from LicenseRef text.
* Cleaned up the documentation.

nRF Connect SDK v1.4.0
**********************

All the notable changes added to the |NCS| v1.4.0 release are documented in this section.

Added
=====

Initial release.

* Added Remote Procedure Calls for nRF SoCs.
