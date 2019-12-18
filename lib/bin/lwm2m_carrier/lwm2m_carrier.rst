.. _liblwm2m_carrier_readme:

LwM2M carrier
#############

You can use the **LwM2M carrier** library in your nRF91 application in order to connect to the operator LwM2M network.
Integrating this library into an application will cause the device to connect to the device management platform autonomously.
Every released version is considered for certification with carriers.
Refer to the release notes for each version of the library to check the certification status of a particular release.
Your final device must go through certification by Verizon Wireless.

Prerequisites
*************
The LwM2M carrier library has dependencies on Zephyr and the nRF Connect SDK.
Check the release notes for the library to see which version of |NCS| it is built for.
To integrate the library in an application, the application must be based on a supported nRF Connect SDK version.

Application integration
***********************
To run the library in an application, it is mandatory to implement the application with the API of the library.
The application has to keep the following values up to date, and the library with automatically report the updated values to the relevant server:
Available Power Sources, Power Source Voltage, Power Source Current, Battery Level, Battery Status, Memory Free, Memory Total, Error Code, Device Type.
The application also needs to implement an event handler and error handlers as shown in the client sample.
The bsdlib_init function is called by the library because of the need to track modem update state.

Application limitations
***********************
While running this module, the application has some limitations.
The library depends on the LTE link controller to maintain connections to carrier servers when required, and the application has to run module.
Several system resources are used to maintain the device management platform connection.
While running the module, the application cannot do the following:

* Use the AT command module. This prevents the application from:
    * subscribing to SMS from the modem,
    * reading net registration status.
* Use DTLS through the modem.

Additionally, you must apply workarounds for the following limitations:

* TLS through the modem is available, but requires that the application respects the LWM2M_CARRIER_EVENT_FOTA_START event.
* TLS handshakes through the modem might fail if the module is performing one at the same time.
  Implement a retry mechanism so that the application can perform its handshake later.

See the changelog for the latest updates in the library, and for the list of known issues and limitations.
The changelog also contains information about the certification status of each library.
Your final product will typically need certification from the carrier as well.
Contact the carrier for information about their device certification program.

.. toctree::
    :maxdepth: 1

    CHANGELOG.rst


API documentation
*****************

| Header files: :file:`lib/bin/lwm2m_carrier/include`
| Source files: :file:`lib/bin/lwm2m_carrier`

LWM2M carrier library API
=========================

.. doxygengroup:: lwm2m_carrier_api
   :project: nrf
   :members:

LWM2M carrier library events
----------------------------

.. doxygengroup:: lwm2m_carrier_event
   :project: nrf
   :members:

LWM2M OS layer
==============

.. doxygengroup:: lwm2m_carrier_os
   :project: nrf
   :members:
