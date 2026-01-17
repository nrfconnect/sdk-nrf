.. _psa_api_wrapper:

PSA Certified API reference documentation
#########################################

This section includes documentation for the PSA Certified APIs from the official `PSA API repository <https://github.com/arm-software/psa-api>`_.
The APIs are published using the sources from the upstream repository.

PSA (Platform Security Architecture) is a set of security specifications and reference implementations developed by Arm to provide a consistent security framework for IoT devices.

The PSA Certified APIs include:

* **PSA Cryptography API**: A standardized interface for cryptographic operations
* **PSA Secure Storage API**: APIs for secure data storage
* **PSA Attestation API**: APIs for device attestation

These pages provide reference documentation for the PSA Certified API headers included in the nRF Connect SDK.

.. note::
   Not all PSA APIs mentioned may be fully implemented or used by the nRF Connect SDK.
   For more information about PSA Crypto support in NCS, see the main nRF Connect SDK documentation.

For complete PSA Certified documentation, visit the `PSA Certified website <https://www.psacertified.org/>`_.

.. toctree::
   :maxdepth: 2
   :caption: API Reference

.. doxygengroup:: psa
   :project: psa_api
   :content-only:
