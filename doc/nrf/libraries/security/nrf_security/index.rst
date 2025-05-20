.. _nrf_security_readme:
.. _nrf_security:

nRF Security
############

The nRF Security subsystem (nrf_security) provides an integration between `Mbed TLS`_ and software libraries that provide hardware-accelerated cryptographic functionality on selected Nordic Semiconductor SoCs as well as alternate software-based implementations of the Mbed TLS APIs.
These libraries include the binary versions of accelerated cryptographic libraries listed in :ref:`nrfxlib:crypto`, and the open source Mbed TLS implementation in the |NCS| located in `sdk-mbedtls`_.
The subsystem includes a PSA driver abstraction layer to enable both hardware-accelerated and software-based implementation at the same time.

The nRF Security subsystem can interface with the :ref:`nrf_cc3xx_mbedcrypto_readme`.
This library conforms to the specific revision of Mbed TLS that is supplied through the |NCS|.

.. toctree::
   :maxdepth: 2
   :caption: Subpages:

   doc/configuration
   doc/drivers
   doc/driver_config
   doc/mbed_tls_header
   doc/backend_config
