.. nrf_rpc_api:

API documentation
#################

.. contents::
   :local:
   :depth: 2

.. _nrf_rpc_core_api_documentation:

Core API documentation
----------------------

This API uses pointers to raw packet data.
:ref:`nrf_rpc_cbor_api_documentation` provides serialization layer over it that uses TinyCBOR.

.. doxygengroup:: nrf_rpc
   :project: nrfxlib
   :members:

.. _nrf_rpc_cbor_api_documentation:

TinyCBOR API documentation
--------------------------

This API is built on top of the core nRF RPC API and it is not independent.
See :ref:`nrf_rpc_core_api_documentation` for more information on how to use nRF RPC together with TinyCBOR.

.. doxygengroup:: nrf_rpc_cbor
   :project: nrfxlib
   :members:
