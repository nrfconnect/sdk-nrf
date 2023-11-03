.. _nrf_802154_changelog:

Changelog
#########

.. contents::
   :local:
   :depth: 2

All notable changes to this project are documented in this file.
See also :ref:`nrf_802154_limitations` for permanent limitations.

nRF Connect SDK v2.5.0 - nRF 802.15.4 Radio Driver
**************************************************

Notable changes
===============

* The callout function :c:func:`nrf_802154_energy_detected` now takes a parameter of type :c:struct:`nrf_802154_energy_detected_t` and provides the ED result in dBm.
  This change in public API can be enabled by setting the ``NRF_802154_ENERGY_DETECTED_VERSION`` to 1. (KRKNWK-17141)
* Include files with API common for both driver and serialization interfaces are now available in the ``common`` directory.
  This change only affects users who are not using the CMake build system. (KRKNWK-17186)

Added
=====

* Added :c:func:`nrf_802154_timestamp_end_to_phr_convert` and :c:func:`nrf_802154_timestamp_phr_to_shr_convert` that can be used to convert the timestamps used by the driver to the timestamp of the first symbol of frame's PHR. (KRKNWK-17153)
* Added support for :c:func:`nrf_802154_pan_coord_get` through serialization (disabled by default via ``NRF_802154_PAN_COORD_GET_ENABLED``). (KRKNWK-10908)
* Added the possibility to perform multiple CCA attempts before a delayed transmission in case the first CCA attempt detects busy channel. (KRKNWK-17304)

Bug fixes
=========
* Fixed an issue causing CSMA/CA procedure to not be terminated correctly in certain Wi-Fi Coexistence scenarios. (KRKNWK-17422)
* Fixed an issue causing data corruption when transmitting frames and ACKs containing IE elements. (KRKNWK-17627)
* Fixed an issue causing an incorrect driver state after transmission setup failure resulting in failing subsequent calls to the 802.15.4 driver. (KRKNWK-17628)

Other changes
=============

* Changed the value of ``ED_RSSISCALE`` to ``4`` for the nRF5340 and nRF52833. (KRKNWK-16902)
* Deprecated :c:func:`nrf_802154_first_symbol_timestamp_get` and :c:func:`nrf_802154_mhr_timestamp_get` functions.
* Improved the modulation filtering when using an external power amplifier on the nRF5340, fixing potential certification issues. (KRKNWK-16949)
* Removed deprecated functions :c:func:`nrf_802154_wifi_coex_enable` and :c:func:`nrf_802154_wifi_coex_disable` and accompanying configuration option ``NRF_802154_COEX_INITIALLY_ENABLED``. (KRKNWK-14574)
* The :c:macro:`NRF_802154_IFS_ENABLED` is disabled by default. IFS feature is marked as experimental. (KRKNWK-17198).

nRF Connect SDK v2.4.0 - nRF 802.15.4 Radio Driver
**************************************************

Notable changes
===============

* Improved frame filtering routine which reduces the likelihood of encountering ``NRF_802154_RX_ERROR_RUNTIME`` error during heavier loads. (KRKNWK-15525)
* Delayed transmissions and receptions are triggered by a hardware timer what makes them more immune to software latencies. (KRKNWK-8615)

Added
=====

* Added :c:func:`nrf_802154_security_global_frame_counter_set_if_larger`. (KRKNWK-16133)

Bug fixes
=========
* Fixed an issue causing the notification about transmission failure to be generated twice what led to a crash on the nRF5340 network core. (KRKNWK-16825)
* Fixed an issue with the receive filter, which led to the receiver not being able to receive a frame shorter than 5 bytes in promiscuous mode. (KRKNWK-16977)

Other changes
=============

* Removed the ``NRF_802154_DISABLE_BCC_MATCHING`` config option. Setting this option to ``NRF_802154_DISABLE_BCC_MATCHING=1`` had been not functional for multiple releases. (KRKNWK-15525)
* Removed the ``NRF_802154_TX_STARTED_NOTIFY_ENABLED`` config option. (KRKNWK-16364)
* The total times measurement feature is turned off. (KRKNWK-16189)
* Removed the ``NRF_802154_TOTAL_TIMES_MEASUREMENT_ENABLED`` config option and support for the total times measurement feature. (KRKNWK-16374)
* CSL Phase is calculated assuming that provided CSL anchor time points to a time where the first bit of MAC header of the frame received from a peer happens. (KRKNWK-16647)


nRF Connect SDK v2.3.0 - nRF 802.15.4 Radio Driver
**************************************************

Notable changes
===============

* Added the possibility to disable the continuous and modulated carrier functions by setting the ``NRF_802154_CARRIER_FUNCTIONS_ENABLED`` define to ``0``.

nRF Connect SDK v2.2.0 - nRF 802.15.4 Radio Driver
**************************************************

Notable changes
===============

* The CSL phase calculation method now depends on the anchor time instead of the nearest scheduled reception window. (KRKNWK-15150)

Added
=====

* Added :c:func:`nrf_802154_csl_writer_anchor_time_set`. (KRKNWK-15150)

Bug fixes
=========

* Implemented a workaround for the YOPAN-158 errata for nRF5340. (KRKNWK-15473)

nRF Connect SDK v2.1.0 - nRF 802.15.4 Radio Driver
**************************************************

Bug fixes
=========

* Fixed an issue where the channel for the delayed transmission on the nRF5340 SoC when passing NULL metadata would be set to 11.
  This was inconsistent with the behavior on nRF52 Series' SoCs and the channel now defaults to the value in the Personal Area Network Information Base (PIB). (KRKNWK-13539)
* Fixed an issue causing the calculated CSL phase to be too small. (KRKNWK-13782)
* Fixed an issue causing the nRF5340 SoC to prematurely run out of buffers for received frames on the application core. (KRKNWK-12493)
* Fixed an issue causing the nRF5340 SoC to transmit with minimum power when the requested transmit power was greater than 0 dBm. (KRKNWK-14487)

nRF Connect SDK v2.0.0 - nRF 802.15.4 Radio Driver
**************************************************

Notable changes
===============

* Reworked the implementation of the internal timer to support 64-bit timestamps. (KRKNWK-8612)
* The transmit power is now expressed as antenna output power, including any front-end module used.

Added
=====

* The transmit power can be set for each transmission request through the transmit metadata. (KRKNWK-13484)
* The use of runtime gain control of the front-end module is now provided by the MPSL library. (KRKNWK-13713)

Bug fixes
=========

* Fixed a stability issue where switching the GRANT line of the coexistence interface could cause a crash. (KRKNWK-11900)
* Fixed an issue where the setting ``NRF_802154_DELAYED_TRX_ENABLED=0`` would make the build fail.
* Fixed an issue where the CSMA-CA procedure was not aborted by pending operations with higher priority.
* Fixed an issue where a notification about an HFCLK change could be delayed by a high priority ISR and could cause a crash. (KRKNWK-11466)
* Fixed an issue where canceling a delayed time slot (for CSMA-CA, delayed transmission, and delayed reception operations) after the preconditions were requested could cause a crash. (KRKNWK-13175)
* Fixed an issue where a coexistence request would not be released at the end of the time slot while operating in multiprotocol mode.
* Fixed an issue where the reported ED values with temperature correction were imprecise. (KRKNWK-13599)
* Disabled the build of CSMA-CA when using the open-source service layer.

Other changes
=============

* Removed the files :file:`nrf_802154_ack_timeout.c` and :file:`nrf_802154_priority_drop_swi.c`.

nRF Connect SDK v1.9.0 - nRF 802.15.4 Radio Driver
**************************************************

Added
=====

* Delayed transmission and reception feature support for nRF5340. (KRKNWK-12074)
* Backforwarding of transmitted frames to support retransmissions through serialization for nRF5340. (KRKNWK-10114)
* Serialization of API required by Thread 1.2 (KRKNWK-12077) and other API for nRF5340.

Bug fixes
=========

* Fixed an issue where interleaving transmissions of encrypted and unencrypted frames could cause memory corruption. (KRKNWK-12261)
* Fixed an issue where interruption of a reception of encrypted frame could cause memory corruption. (KRKNWK-12622)
* Fixed an issue where transmission of an encrypted frame could transmit a frame filled partially with zeros instead of proper ciphertext. (KRKNWK-12770)
* Fixed stability issues related to CSMA-CA occurring with enabled experimental coexistence feature from :ref:`mpsl`. (KRKNWK-12701)

nRF Connect SDK v1.8.0 - nRF 802.15.4 Radio Driver
**************************************************

Notable changes
===============

* Incoming frames with Header IEs present but with no payload IEs and with no payload do not need IE Termination Header provided anymore. (KRKNWK-11875)

Bug fixes
=========

* Fixed an issue where the notification queue would be overflowed under stress. (KRKNWK-11606)
* Fixed an issue where ``nrf_802154_transmit_failed`` callout would not always correctly propagate the frame properties. (KRKNWK-11605)

nRF Connect SDK v1.7.0 - nRF 802.15.4 Radio Driver
**************************************************

Added
=====

* Adopted usage of the Zephyr temperature platform for the RSSI correction.
* Support for the coexistence feature from :ref:`mpsl`.
* Support for nRF21540 FEM GPIO interface on nRF53 Series.

Notable changes
===============

* Modified the 802.15.4 Radio Driver Transmit API.
  It now allows specifying whether to encrypt or inject dynamic data into the outgoing frame, or do both.
  The :c:type:`nrf_802154_transmitted_frame_props_t` type is used for this purpose.

Bug fixes
=========

* Fixed an issue where it would not be possible to transmit frames with invalid Auxiliary Security Header if :kconfig:option:`CONFIG_NRF_802154_ENCRYPTION` was set to ``n``. (KRKNWK-11218)
* Fix an issue with the IE Vendor OUI endianness. (KRKNWK-10633)
* Fixed various bugs in the MAC Encryption layer. (KRKNWK-10646)

nRF Connect SDK v1.6.0 - nRF 802.15.4 Radio Driver
**************************************************

Initial common release.

Added
=====

* Added the source code of the 802.15.4 Radio Driver.
* Added the 802.15.4 Service Layer library.
* Added the source code of the 802.15.4 Radio Driver API serialization library.
* Added the possibility to schedule two delayed reception windows.
* Added CSL phase injection.
* Added outgoing frame encryption and frame counter injection.
* Added Thread Link Metrics IEs injection.

Notable Changes
===============

* The release notes of the legacy versions of the Radio Driver are available in the `Radio Driver section`_ of the Infocenter.
* The changelog of the previous versions of the 802.15.4 SL library is now located at the bottom of this page.
* The Radio Driver documentation will now also include the Service Layer documentation.
* Future versions of the Radio Driver and the Service Layer will follow NCS version tags.
* The 802.15.4 Radio Driver API has been modified to support more than a single delayed reception window simultaneously.
  The :c:func:`nrf_802154_receive_at`, :c:func:`nrf_802154_receive_at_cancel`, and :c:func:`nrf_802154_receive_failed` functions take an additional parameter that identifies a given reception window unambiguously.

nRF 802.15.4 Service Layer 0.5.0
********************************

* Added the possibility to check the 802.15.4 capabilities.

Added
=====

* Added the possibility to check the 802.15.4 capabilities.
  Built from commit *2966ae8b4b3fcf2b64d8b987703cbf4ecc0dd60b*.

nRF 802.15.4 Service Layer 0.4.0
********************************

* Added multiprotocol support for the nRF53 family.

Added
=====

* Added multiprotocol support for the nRF53 family.
  Built from commit *5d2497b78683687bdd57fcd6854b1bc3c26871be*.

nRF 802.15.4 Service Layer 0.3.0
********************************

* PA/LNA implementation has been moved to MPSL.
  Obsolete implementation and API have been removed.

Removed
=======

* Removed PA/LNA implementation and API.
  Built from commit *e268db75108016ee02965556aa52cf8437f5e071*.

nRF 802.15.4 Service Layer 0.2.0
********************************

Initial release.

Added
=====

* Added the :file:`libnrf_802154_sl.a` library.
  Built from commit *4c5ff68c4eb4ba817774bbd6c711a67dfde7d905*.
