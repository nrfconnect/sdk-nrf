.. _migration_2.9:

Migration guide for |NCS| v2.9.0 (Working draft)
################################################

.. contents::
   :local:
   :depth: 3

This document describes the changes required or recommended when migrating your application from |NCS| v2.8.0 to |NCS| v2.9.0.

.. HOWTO

   Add changes in the following format:

   Component (for example, application, sample or libraries)
   *********************************************************

   .. toggle::

      * Change1 and description
      * Change2 and description

.. _migration_2.9_required:

Required changes
****************

The following changes are mandatory to make your application work in the same way as in previous releases.

nRF54H20
========

This section describes the changes specific to the nRF54H20 SoC and DK support in the |NCS|.
For more information on changes related to samples and applications usage on the nRF54H20 DK, see :ref:`migration_2.9_required_nrf54h`.

DK compatibility
----------------

.. toggle::

   * The |NCS| v2.9.0 is compatible only with the Engineering C - v0.8.3 and later revisions of the nRF54H20 DK, PCA10175.
     The nRF54H20 DK Engineering A and Engineering B (up to v0.8.2) are no longer compatible with the |NCS| v2.9.0.

    Check the version number on your DK's sticker to verify its compatibility with the |NCS|.

Dependencies
------------

The following required dependencies for the nRF54H20 SoC and DK have been updated.


Samples and applications
========================

This section describes the changes related to samples and applications.

|no_changes_yet_note|

.. _migration_2.9_required_nrf54h:

nRF54H20
--------

|no_changes_yet_note|

Libraries
=========

This section describes the changes related to libraries.

|no_changes_yet_note|

.. _migration_2.9_recommended:

Recommended changes
*******************

The following changes are recommended for your application to work optimally after the migration.

Samples and applications
========================

This section describes the changes related to samples and applications.

|no_changes_yet_note|

Libraries
=========

This section describes the changes related to libraries.

|no_changes_yet_note|
