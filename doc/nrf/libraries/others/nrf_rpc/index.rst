.. _nrf_rpc:

Remote procedure call library (nRF RPC)
#######################################

nRF RPC is a remote procedure call library for |NCS| enabling inter-processor communication on Nordic Semiconductor SoCs.
The library is RTOS-agnostic and implements serialization of function calls.
It is designed to be used with an underlying transport layer, for example `OpenAMP`_.

The nRF RPC library allows for calling a function on remote processors from a local processor, in both synchronous and asynchronous way.

Depending on the transport layer, the remote processor is not limited to a single device.
It can also be a separate device of any type (for example, a PC), or another core on the same system.

The nRF RPC library simplifies the serialization of user APIs, such as a Bluetooth stack, and executing of functions implementing those APIs on a remote CPU.
The library is operating system independent so it can be used with any operating system after porting the OS-dependent layers of the library.

The API layer on top of the core nRF RPC API uses the `TinyCBOR`_ library for serialization.
nRF RPC requires the Zephyr Project fork of TinyCBOR, because of API differences.

.. toctree::
   :maxdepth: 2
   :caption: Subpages:

   architecture
   usage
   CHANGELOG
   api
