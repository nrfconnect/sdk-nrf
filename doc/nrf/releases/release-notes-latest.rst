:orphan:

.. _ncs_release_notes_latest:

Changes in |NCS| v1.4.99
########################

.. contents::
   :local:
   :depth: 2

The most relevant changes that are present on the master branch of the |NCS|, as compared to the latest release, are tracked in this file.

Highlights
**********


Changelog
*********

The following sections provide detailed lists of changes by component.

Bluetooth LE
------------


nRF5
====

Zigbee
------


nRF9160
=======

Modem libraries
---------------

* Modem library integration layer:

  * Fixed a bug in the socket offloading component, where the :c:func:`recvfrom` wrapper could do an out-of-bounds copy of the sender's address, when the application is compiled without IPv6 support. In some cases, the out of bounds copy could indefinitely block the :c:func:`send` and other socket API calls.

Common
======




MCUboot
=======






Mcumgr
======





Zephyr
======



Documentation
=============


Samples
-------



User guides
-----------



Known issues
************

Known issues are only tracked for the latest official release.
