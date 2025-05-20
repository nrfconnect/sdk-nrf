.. _liblwm2m_carrier_readme:

LwM2M carrier
#############

.. contents::
   :local:
   :depth: 2

Several mobile network carriers specify their own LwM2M device management.
The LwM2M carrier library facilitates an nRF91 device to connect to the device management servers, regardless of the other actions of the user application.
For more information, see the following pages:

* `nRF9151 Mobile network operator certifications`_
* `nRF9161 Mobile network operator certifications`_
* `nRF9160 Mobile network operator certifications`_
* :ref:`lwm2m_certification`

.. important::

   It is *mandatory* to include the LwM2M carrier library in any Verizon certified product.

The :ref:`lwm2m_carrier` sample demonstrates how to run this library in an application.
In addition to this stand-alone sample, the following applications and samples include an overlay file to easily integrate the LwM2M carrier library:

  * :ref:`modem_shell_application`
  * :ref:`http_application_update_sample`
  * :ref:`serial_lte_modem`

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
