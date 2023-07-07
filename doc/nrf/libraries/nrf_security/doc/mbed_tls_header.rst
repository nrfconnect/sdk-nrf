.. _nrf_security_tls_header:

User-provided Mbed TLS config header
####################################

The nRF Security subsystem provides a Kconfig interface to control compilation and linking of Mbed TLS and the :ref:`nrf_cc3xx_mbedcrypto_readme` or :ref:`nrf_oberon_readme` libraries.

The Kconfig interface and build system ensures that the configuration of nrf_security is valid and working.
It also ensures that dependencies between different cryptographic APIs are met.

It is therefore highly recommended to let the build system generate the Mbed TLS configuration headers.

However, for special use cases that cannot be achieved using the Kconfig configuration tool, it is possible to provide custom Mbed TLS configuration headers.

Make sure that the system is working:

1. Use Kconfig and the build system to create Mbed TLS configuration headers as a starting point.
#. Edit this file to include settings that are not available in Kconfig.

.. note::
   When providing custom Mbed TLS configuration headers with CryptoCell in use, it is important that the following criteria are still met:

   * Entropy length of 144, that is, ``#define MBEDTLS_ENTROPY_MAX_GATHER 144``.
   * SHA-256 is used for entropy, that is, ``#define MBEDTLS_ENTROPY_FORCE_SHA256`` is set.
   * Entropy max sources is set to ``1``, that is ``#define MBEDTLS_ENTROPY_MAX_SOURCES 1``.
