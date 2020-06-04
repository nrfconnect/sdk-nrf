.. _liblwm2m_carrier_changelog:

Changelog
#########

All notable changes to this project are documented in this file.

liblwm2m_carrier 0.9.0
**********************

Snapshot release for the upcoming modem firmware version 1.2.1 and the |NCS| 1.3.0.

This release is intended to let users begin integration towards the AT&T and Verizon device management platforms.
It can be used for development and testing only.
It is not ready for certification.

Certification status
====================

The library is not certified with any carrier.

Changes
=======

* Added new APIs to create and access portfolio object instances.
  A new portfolio object instance can be created using ``lwm2m_carrier_portfolio_instance_create()``.
  ``lwm2m_carrier_identity_read()`` and ``lwm2m_carrier_identity_write()`` are used to read and write to the corresponding Identity resource fields of a given portfolio object instance.
* Expanded API with "certification_mode" variable that chooses between certification or live servers upon the initialization of the LwM2M carrier library.
* Expanded API with "apn" variable to set a custom APN upon the initialization of the LwM2M carrier library.
* PSK Key is now set independently of custom URI.

  * Added the LWM2M_CARRIER_USE_CUSTOM_BOOTSTRAP_PSK and LWM2M_CARRIER_CUSTOM_BOOTSTRAP_PSK Kconfig options.

* PSK format has been modified to be more user-friendly.

  * Previous format: Byte array. For example, ``static const char bootstrap_psk[] = {0x01, 0x02, 0xab, 0xcd, 0xef};``.
  * Current format: A null-terminated string that must be composed of hexadecimal numbers. For example "0102abcdef".

* Added lwm2m_carrier_event_deferred_t to retrieve the event reason and timeout.
* Added FOTA errors to LWM2M_CARRIER_EVENT_ERROR event.


liblwm2m_carrier 0.8.1+build1
*****************************

Release for modem firmware version 1.1.0, with support for Verizon Wireless.

Certification status
====================

The library is certified with Verizon Wireless.

Changes
=======

* Fixed a memory leak.


liblwm2m_carrier 0.8.1
**********************

Release for modem firmware version 1.1.0, with support for Verizon Wireless.

Certification status
====================

The library is certified with Verizon Wireless.

Changes
=======

* Numerous stability fixes and improvements.
* Updated bsdlib version dependency.
* Fixed an issue where high LTE network activity could prevent modem firmware updates over LwM2M.

* Added the following library events:
   * LWM2M_CARRIER_EVENT_CONNECTING, to indicate that the LTE link is about to be brought up.
   * LWM2M_CARRIER_EVENT_DISCONNECTING, to indicate that the LTE link is about to be brought down.
   * LWM2M_CARRIER_EVENT_DEFERRED, to indicate that the LwM2M operation is deferred for 24 hours.
   * LWM2M_CARRIER_EVENT_ERROR, to indicate that an error has occurred.

* Renamed the following library events:
   * LWM2M_CARRIER_EVENT_CONNECT to LWM2M_CARRIER_EVENT_CONNECTED.
   * LWM2M_CARRIER_EVENT_DISCONNECT to LWM2M_CARRIER_EVENT_DISCONNECTED.


liblwm2m_carrier 0.8.0
**********************

Release for modem firmware version 1.1.0 and |NCS| v1.1.0, with support for Verizon Wireless.

Certification status
====================

The library is not certified with Verizon Wireless.

Changes
=======

* Abstracted several new functions in the glue layer to improve compatibility on top of the master branch.
* Reorganized NVS keys usage to make it range-bound (0xCA00, 0xCAFF).
  This range is not backward compatible, so you should not rely on pre-existing information saved in flash by earlier versions of this library.
* Added APIs to set the following values from the application:

   * Available Power Sources
   * Power Source Voltage
   * Power Source Current
   * Battery Level
   * Battery Status
   * Memory Total
   * Error Code

  The application must set and maintain these values to reflect the state of the device.
  Updated values are pushed to the servers autonomously.

* Added API to set the "Device Type" resource. If not set, this is reported as "Smart Device".
* Added API to set the "Software Version" resource. If not set, this is reported as "LwM2M 0.8.0".
* Added API to set the "Hardware Version" resource. If not set, this is reported as "1.0".

Known issues and limitations
============================

* It is not possible to use a DTLS connection in parallel with the library.
* It is not possible to use a TLS connection in parallel with LwM2M-managed modem firmware updates.
  The application should close any TLS connections when it receives the LWM2M_CARRIER_EVENT_FOTA_START event from the library.


liblwm2m_carrier 0.6.0
**********************

Initial public release for modem firmware version 1.0.1.
This release is intended to let users begin the integration on the Verizon Wireless device management platform and start the certification process with Verizon Wireless.
We recommend upgrading to the next release when it becomes available.
The testing performed on this release does not meet Nordic standard for mass production release testing.


Known issues and limitations
============================

* It is not possible to use a DTLS connection in parallel with the library.
* It is not possible to use a TLS connection in parallel with LwM2M-managed modem firmware updates. The application should close any TLS connections when it receives the LWM2M_CARRIER_EVENT_FOTA_START event from the library.
* The API to query the application for resource values is not implemented yet.
	* The "Available Power Sources" resource is reported as "DC power (0)" and "External Battery (2)".
	* The following resources are reported to have value "0" (zero):
		* Power Source Voltage, Power Source Current, Battery Level, Battery Status, Memory Free, Memory Total, Error Code.
	* The "Device Type" resource is reported as "Smart Device".
	* The "Software Version" resource is reported as "LwM2M 0.6.0".
	* The "Hardware Version" is reported as "1.0".
* The following values are reported as dummy values instead of being fetched from the modem:
	* "IP address", reported as 192.168.0.0.
* The "Current Time" and "Timezone" resources do not respect write operations, instead, read operations on these resources will return the current time and timezone as kept by the nRF9160 modem.
