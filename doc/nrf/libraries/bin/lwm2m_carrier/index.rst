.. _liblwm2m_carrier_readme:

LwM2M carrier
#############

.. contents::
   :local:
   :depth: 2

Several mobile network carriers specify their own LwM2M device management.
The LwM2M carrier library facilitates an nRF91 device to connect to the device management servers, regardless of the other actions of the user application.

.. important::

   It is *mandatory* to include the LwM2M carrier library in any AT&T, T-Mobile, or Verizon certified products.

For the `applicable carriers <Mobile network operator certifications_>`_, it is mandatory to include this library in your application for the certification to your end product.

The :ref:`lwm2m_carrier` sample demonstrates how to run this library in an application.
The :ref:`asset_tracker_v2` application also uses the LwM2M carrier library.

.. note::

   See the Zephyr documentation on :ref:`lwm2m_interface` if you want to use LwM2M for other purposes.

.. toctree::
    :maxdepth: 1
    :caption: Subpages:

    certification
    app_integration
    requirements
    msc
    API_documentation
    CHANGELOG
