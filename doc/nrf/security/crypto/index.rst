.. _ug_crypto_index:

Cryptography in the |NCS|
#########################

Nordic Semiconductor requires following `Platform Security Architecture (PSA)`_ for product development with the |NCS| to ensure appropriate security implementation in IoT devices.

The |NCS| implements cryptographic operations through the mandatory `PSA Certified Crypto API`_ standard, which provides an interface for cryptographic functions across different hardware platforms.
The SDK supports the following PSA Crypto API implementations:

* Direct PSA Crypto API access through Oberon PSA Crypto for applications that do not require additional security isolation or devices without TrustZone.
* Access through :ref:`Trusted Firmware-M (TF-M) <ug_tfm>` for applications requiring enhanced security through hardware-enforced separation.

For more information about these implementations, see the :ref:`ug_crypto_architecture` page.

All cryptographic functionality is accessed through the :ref:`nrf_security` library, which integrates and configures the PSA Crypto implementation with various :ref:`cryptographic drivers <crypto_drivers>`.

For practical examples of cryptographic operations, see the :ref:`cryptography samples <crypto_samples>`.
For more information about PSA Crypto within the broader context of the PSA Certified IoT Security Framework, see the :ref:`ug_psa_certified_api_overview` page.

.. toctree::
   :maxdepth: 2
   :caption: Subpages:

   crypto_architecture
   drivers
   crypto_supported_features
