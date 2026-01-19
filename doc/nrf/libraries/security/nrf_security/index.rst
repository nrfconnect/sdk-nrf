.. _nrf_security_readme:
.. _nrf_security:

nRF Security
############

The nRF Security subsystem (``nrf_security``) integrates cryptographic services for SoCs from Nordic Semiconductor.

The nRF Security subsystem provides:

* A unified interface to both :ref:`PSA Crypto APIs <ug_psa_certified_api_overview_crypto>` and `Mbed TLS`_ APIs
* Hardware acceleration through dedicated cryptographic libraries on selected SoCs (``nrf_cc3xx``, CRACEN), with binary versions of the libraries listed in :ref:`nrfxlib:crypto`
* Software fallbacks when hardware acceleration is unavailable (``nrf_oberon``)
* A PSA driver abstraction layer enabling simultaneous use of hardware and software implementations
* Compatibility with the specific Mbed TLS version included in the |NCS| through `sdk-mbedtls`_
* Integration logic for the Oberon PSA Crypto core (`sdk-oberon-psa-crypto`_)
* Source code for the CRACEN driver
* Integration with the |NCS| build system

The nRF Security subsystem can interface with the :ref:`nrf_cc3xx_mbedcrypto_readme`.
This library conforms to the specific revision of Mbed TLS that is supplied through the |NCS|.

.. toctree::
   :maxdepth: 2
   :caption: Subpages:

   doc/configuration
   doc/backend_config
