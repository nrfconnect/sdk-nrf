.. _nrf_security_readme:
.. _nrf_security:

Nordic Security Module
######################

The Nordic Security Module (nrf_security) provides an integration between Mbed TLS and software libraries that provide hardware-accelerated cryptographic functionality on selected Nordic Semiconductor SoCs as well as alternate software-based implementations of the Mbed TLS APIs.
This module includes a PSA driver abstraction layer to enable both hardware-accelerated and software-based implementation at the same time.

The nrf_security module can interface with the :ref:`nrf_cc3xx_mbedcrypto_readme`.
This library conforms to the specific revision of Mbed TLS that is supplied through the nRF Connect SDK.

.. toctree::
   :maxdepth: 2
   :caption: Subpages:

   doc/configuration
   doc/drivers
   doc/driver_config
   doc/mbed_tls_header
   doc/backend_config
