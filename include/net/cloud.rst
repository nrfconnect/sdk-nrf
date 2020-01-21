.. _cloud_api_readme:

Cloud API
#########

The cloud API library exposes API to interact with cloud solutions.
Using these APIs, an application can connect to a cloud, and send and receive data.

Functionality
*************
Cloud API defines a set of functions that a cloud driver, also called a cloud backend, must implement.
Some functions of cloud API are optional and can be omitted.

To use the API, you must provide an event handler during the initialization of the cloud backend.
This handler will receive events when, for example, data is received from the cloud.

After successful initialization of the cloud backend, you can establish a connection to the cloud.
If the connection succeeds, the backend emits a "ready event", and you can start interacting with the cloud.

.. _cloud_api_reference:

API Reference
*************

| Header file: :file:`include/net/cloud.h`
| Source files: :file:`subsys/net/lib/cloud/`

.. doxygengroup:: cloud_api
   :project: nrf
   :members:
