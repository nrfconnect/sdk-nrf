.. _ncs_release_notes_changelog:

Changelog for |NCS| v2.5.99
###########################

.. contents::
   :local:
   :depth: 2

The most relevant changes that are present on the main branch of the |NCS|, as compared to the latest official release, are tracked in this file.

.. note::
   This file is a work in progress and might not cover all relevant changes.

.. HOWTO

   When adding a new PR, decide whether it needs an entry in the changelog.
   If it does, update this page.
   Add the sections you need, as only a handful of sections is kept when the changelog is cleaned.
   "Protocols" section serves as a highlight section for all protocol-related changes, including those made to samples, libraries, and so on.

Known issues
************

Known issues are only tracked for the latest official release.
See `known issues for nRF Connect SDK v2.5.0`_ for the list of issues valid for the latest release.

Changelog
*********

The following sections provide detailed lists of changes by component.

IDE and tool support
====================

|no_changes_yet_note|

Application development
=======================

This section provides detailed lists of changes to overarching SDK systems and components.

Build system
------------

|no_changes_yet_note|

nRF Front-End Modules
---------------------

|no_changes_yet_note|

Working with nRF91 Series
=========================

|no_changes_yet_note|

Working with nRF52 Series
=========================

|no_changes_yet_note|

Working with nRF53 Series
=========================

|no_changes_yet_note|

Protocols
=========

This section provides detailed lists of changes by :ref:`protocol <protocols>`.
See `Samples`_ for lists of changes for the protocol-related samples.

Bluetooth® LE
-------------

|no_changes_yet_note|

Bluetooth mesh
--------------

|no_changes_yet_note|

Matter
------

|no_changes_yet_note|

Matter fork
+++++++++++

The Matter fork in the |NCS| (``sdk-connectedhomeip``) contains all commits from the upstream Matter repository up to, and including, the ``v1.1.0.1`` tag.

The following list summarizes the most important changes inherited from the upstream Matter:

|no_changes_yet_note|

Thread
------

|no_changes_yet_note|

See `Thread samples`_ for the list of changes for the Thread samples.

Zigbee
------

|no_changes_yet_note|

Enhanced ShockBurst (ESB)
-------------------------

|no_changes_yet_note|

nRF IEEE 802.15.4 radio driver
------------------------------

|no_changes_yet_note|

Wi-Fi
-----

|no_changes_yet_note|

HomeKit
-------

HomeKit is now removed, as announced in the :ref:`ncs_release_notes_250`.

Applications
============

This section provides detailed lists of changes by :ref:`application <applications>`.

Asset Tracker v2
----------------

|no_changes_yet_note|

Serial LTE modem
----------------

|no_changes_yet_note|

nRF5340 Audio
-------------


|no_changes_yet_note|

nRF Machine Learning (Edge Impulse)
-----------------------------------

|no_changes_yet_note|

nRF Desktop
-----------

|no_changes_yet_note|

Thingy:53: Matter weather station
---------------------------------

|no_changes_yet_note|

Matter Bridge
-------------

|no_changes_yet_note|

Samples
=======

This section provides detailed lists of changes by :ref:`sample <samples>`.

Bluetooth samples
-----------------

|no_changes_yet_note|

Bluetooth mesh samples
----------------------

|no_changes_yet_note|

Cellular samples (renamed from nRF9160 samples)
-----------------------------------------------

* :ref:`modem_shell_application` sample:

  * Added:

    * Printing of the last reset reason when the sample starts.
    * Support for printing the sample version information using the ``version`` command.

Cryptography samples
--------------------

* Updated:

  * All crypto samples to use ``psa_key_id_t`` instead of ``psa_key_handle_t``.
    These concepts have been merged and ``psa_key_handle_t`` is removed from the PSA API specification.

|no_changes_yet_note|

Debug samples
-------------

|no_changes_yet_note|

Edge Impulse samples
--------------------

|no_changes_yet_note|

Enhanced ShockBurst samples
---------------------------

|no_changes_yet_note|

Gazell samples
--------------

|no_changes_yet_note|

Keys samples
------------

|no_changes_yet_note|

Matter samples
--------------

* :ref:`matter_lock_sample` sample:

  * Fixed an issue that prevented nRF Toolbox for iOS in version 5.0.9 from controlling the sample using :ref:`nus_service_readme`.

Multicore samples
-----------------

|no_changes_yet_note|

Networking samples
------------------

* :ref:`nrf_coap_client_sample` sample:

  * Added support for Wi-Fi and LTE connectivity through the connection manager API.
  * Updated by moving the sample from :file:`cellular/coap_client` folder to :file:`net/coap_client`.
    The documentation is now found in the :ref:`networking_samples` section.

|no_changes_yet_note|

NFC samples
-----------

|no_changes_yet_note|

nRF5340 samples
---------------

|no_changes_yet_note|

Peripheral samples
------------------

|no_changes_yet_note|

PMIC samples
------------

|no_changes_yet_note|

Sensor samples
--------------

|no_changes_yet_note|

Trusted Firmware-M (TF-M) samples
---------------------------------

|no_changes_yet_note|

Thread samples
--------------

|no_changes_yet_note|

Sensor samples
--------------

Zigbee samples
--------------

|no_changes_yet_note|

Wi-Fi samples
-------------

|no_changes_yet_note|

Other samples
-------------

|no_changes_yet_note|

Drivers
=======

This section provides detailed lists of changes by :ref:`driver <drivers>`.

|no_changes_yet_note|

Wi-Fi drivers
-------------

* OS agnostic code is moved to |NCS| (``sdk-nrfxlib``) repository.

   - Low-level API documentation is now available on the :ref:`Wi-Fi driver API <nrfxlib:nrf_wifi_api>`.

Libraries
=========

This section provides detailed lists of changes by :ref:`library <libraries>`.

Binary libraries
----------------

|no_changes_yet_note|

Bluetooth libraries and services
--------------------------------

|no_changes_yet_note|

Bootloader libraries
--------------------

|no_changes_yet_note|

Debug libraries
---------------

|no_changes_yet_note|

DFU libraries
-------------

|no_changes_yet_note|

Modem libraries
---------------

* :ref:`nrf_modem_lib_readme`:

   * Added a mention about enabling TF-M logging while using modem traces in the :ref:`modem_trace_module`.

|no_changes_yet_note|

Libraries for networking
------------------------

* :ref:`lib_aws_iot` library:

  * Added library tests.
  * Updated the library to use the :ref:`lib_mqtt_helper` library.
    This simplifies the handling of the MQTT stack.

Libraries for NFC
-----------------

|no_changes_yet_note|

nRF Security
------------

|no_changes_yet_note|

Other libraries
---------------

|no_changes_yet_note|

Common Application Framework (CAF)
----------------------------------

|no_changes_yet_note|

Shell libraries
---------------

|no_changes_yet_note|

Libraries for Zigbee
--------------------

|no_changes_yet_note|

sdk-nrfxlib
-----------

See the changelog for each library in the :doc:`nrfxlib documentation <nrfxlib:README>` for additional information.

Scripts
=======

This section provides detailed lists of changes by :ref:`script <scripts>`.

|no_changes_yet_note|

MCUboot
=======

The MCUboot fork in |NCS| (``sdk-mcuboot``) contains all commits from the upstream MCUboot repository up to and including ``11ecbf639d826c084973beed709a63d51d9b684e``, with some |NCS| specific additions.

The code for integrating MCUboot into |NCS| is located in the :file:`ncs/nrf/modules/mcuboot` folder.

The following list summarizes both the main changes inherited from upstream MCUboot and the main changes applied to the |NCS| specific additions:

|no_changes_yet_note|

Zephyr
======

.. NOTE TO MAINTAINERS: All the Zephyr commits in the below git commands must be handled specially after each upmerge and each nRF Connect SDK release.

The Zephyr fork in |NCS| (``sdk-zephyr``) contains all commits from the upstream Zephyr repository up to and including ``a768a05e6205e415564226543cee67559d15b736``, with some |NCS| specific additions.

For the list of upstream Zephyr commits (not including cherry-picked commits) incorporated into nRF Connect SDK since the most recent release, run the following command from the :file:`ncs/zephyr` repository (after running ``west update``):

.. code-block:: none

   git log --oneline a768a05e62 ^4bbd91a908

For the list of |NCS| specific commits, including commits cherry-picked from upstream, run:

.. code-block:: none

   git log --oneline manifest-rev ^a768a05e62

The current |NCS| main branch is based on revision ``a768a05e62`` of Zephyr.

.. note::
   For possible breaking changes and changes between the latest Zephyr release and the current Zephyr version, refer to the :ref:`Zephyr release notes <zephyr_release_notes>`.

Additions specific to |NCS|
---------------------------

|no_changes_yet_note|

zcbor
=====

|no_changes_yet_note|

Trusted Firmware-M
==================

|no_changes_yet_note|

cJSON
=====

|no_changes_yet_note|

Documentation
=============

* :ref:`ug_nrf9160`:

   *  Section :ref:tfm_enable_share_uart in the :ref:uge_nrf9160 page.

|no_changes_yet_note|
