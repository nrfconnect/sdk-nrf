.. _liblwm2m_carrier_readme:

LwM2M carrier
#############

.. contents::
   :local:
   :depth: 2

You can use the LwM2M carrier library in your nRF91 application in order to connect to the LwM2M operator network.
Integrating this library into an application causes the device to connect to the operator's device management platform autonomously.
Two examples of such device management platforms are `Verizon's Thingspace`_ and `AT&T's IoT Platform`_.

.. important::

   It is *mandatory* to include the LwM2M carrier library in any Verizon or AT&T certified products.

This library uses LwM2M protocol to communicate with device manager platforms, but it does not expose an LwM2M API to your application.
If you want to use LwM2M for other purposes, see :ref:`lwm2m_interface`.

The :ref:`lwm2m_carrier` sample demonstrates how to run this library in an application.
The LwM2M carrier library is also used in the :ref:`asset_tracker` application.


.. toctree::
    :maxdepth: 1
    :caption: Subpages:

    doc/certification
    doc/app_integration
    doc/requirements
    doc/msc
    doc/CHANGELOG



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
