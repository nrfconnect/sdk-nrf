.. _liblwm2m_carrier_readme:

LwM2M carrier
#############

You can use the **LwM2M carrier** library in your nRF91 application in order to connect to the operator LwM2M network.
Integrating this library into an application causes the device to connect to the device management platform autonomously.

Every released version is considered for certification with carriers.
Refer to the :ref:`liblwm2m_carrier_changelog` to check the certification status of a particular release.
Your final device must go through certification by the carrier.

The :ref:`lwm2m_carrier` sample demonstrates how to run this library in an application.

Prerequisites
*************
The LwM2M carrier library has dependencies on Zephyr and the |NCS|.
See the :ref:`liblwm2m_carrier_changelog` of the library to check which version of |NCS| it is built for.
To integrate the library into an application, the application must be based on a supported |NCS| version.

Application integration
***********************
To run the library in an application, you must implement the application with the API of the library.

The following values that reflect the state of the device must be kept up to date by the application:

* Available Power Sources
* Power Source Voltage
* Power Source Current
* Battery Level
* Battery Status
* Memory Total
* Error Code
* Device Type

The library automatically reports the updated values to the relevant server.

The application must also implement an event handler and error handlers, as shown in the :ref:`lwm2m_carrier` sample.
The ``bsdlib_init`` function is called by the library because of the need to track modem update state.

Application limitations
***********************
While running this module, the application has some limitations.

Several system resources are used to maintain the device management platform connection.

While running the module, the application cannot use DTLS through the modem.

Additionally, you must apply workarounds for the following limitations:

* TLS through the modem is available, but requires that the application respects the :c:type:`LWM2M_CARRIER_EVENT_FOTA_START` event.
* TLS handshakes through the modem might fail if the module is performing one at the same time.
  Implement a retry mechanism so that the application can perform its handshake later.

Certification
*************
The changelog contains information about the certification status of each version of the library.
Your final product will typically need certification from the carrier as well.
Contact the carrier for information about their device certification program.

Library changelog
*****************
See the changelog for the latest updates in the library, and for a list of known issues and limitations.

.. toctree::
    :maxdepth: 1

    doc/CHANGELOG.rst


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
