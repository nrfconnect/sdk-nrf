.. _nrf_security_tls_header:

User-provided Mbed TLS configuration header
###########################################

The nRF Security subsystem provides a Kconfig- and CMake-based build system to configure the Mbed TLS, :ref:`nrf_cc3xx_mbedcrypto_readme`, and :ref:`nrf_oberon_readme` libraries.
The recommended method for generating the Mbed TLS header is through this build system, as it also enforces dependencies between the libraries.
However, for use cases that cannot be configured through the build system, you must provide custom Mbed TLS configuration headers.

Complete the following steps:

1. Generate the Mbed TLS configuration header.
#. Create a copy of the :file:`nrf-config.h` Mbed TLS header file in the build directory and give it a custom name.
#. Move this custom Mbed TLS header file to your source directory.
#. In the project configuration, make the following changes:

   * `CONFIG_GENERATE_MBEDTLS_CFG_FILE=n`.
   * `CONFIG_MBEDTLS_CFG_FILE="custom-name-nrf-config.h"`.
   * `CONFIG_MBEDTLS_USER_CONFIG_FILE="empty_file.h"`.
#. Create an empty file named :file:`empty_file.h`.
#. Edit :file:`custom-name-nrf-config.h` with your custom configuration.
#. If the header files are not already in the include path, add them by editing the application build scripts.

.. note::
   When providing custom Mbed TLS configuration headers with CryptoCell in use, it is important that the following criteria are still met:

   * Entropy length of 144, that is, ``#define MBEDTLS_ENTROPY_MAX_GATHER 144``.
   * SHA-256 is used for entropy, that is, ``#define MBEDTLS_ENTROPY_FORCE_SHA256`` is set.
   * Entropy max sources is set to ``1``, that is ``#define MBEDTLS_ENTROPY_MAX_SOURCES 1``.

.. note::
   Providing custom Mbed TLS configuration headers is not supported when TF-M is enabled.
