.. _liblwm2m_carrier_changelog:

Changelog
#########

All notable changes to this project are documented in this file.


liblwm2m_carrier 0.7.1
**********************

Release for modem firmware version 1.0.2 with support for Verizon Wireless.
This release supports all mandatory features required for certification with the device management platform of Verizon Wireless.
The main change from version 0.6.0 is optimization of resource requirements which ensures that  the library is stable when sharing modem resources with an application.


Certification status
====================

The library is undergoing Verizon Wireless certification.


Additions
=========

* Added APIs to set the following values from the application. The application should set and maintain these values to reflect the state of the device. Updated values are pushed to the servers autonomously.
	* Available Power Sources, Power Source Voltage, Power Source Current, Battery Level, Battery Status, Memory Free, Memory Total, Error Code.
* Added API to set the "Device Type" resource. If not set, this is reported as "Smart Device".
* Added API to set the "Software Version" resource. If not set, this is reported as "LwM2M 0.7.1".
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
