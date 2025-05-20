.. _lwm2m_client:

Cellular: LwM2M Client
######################

.. contents::
   :local:
   :depth: 2

The LwM2M Client demonstrates usage of the :term:`Lightweight Machine to Machine (LwM2M)` protocol to connect a Thingy:91 or an nRF91 Series DK to an LwM2M Server through LTE.
This sample uses the :ref:`lib_lwm2m_client_utils` and :ref:`lib_lwm2m_location_assistance` libraries.

The sample has implementation for many cellular-specific features, such as location services, firmware update, and power saving optimizations.
For a network-agnostic and basic LwM2M client sample, see :zephyr:code-sample:`lwm2m-client`.

See the subpages for detailed documentation on the sample and its features.

.. toctree::
   :maxdepth: 1
   :caption: Subpages:

   sample_description.rst
   fota.rst
   fota_external_mcu.rst
   provisioning.rst
