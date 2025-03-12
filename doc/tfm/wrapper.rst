.. _tfm_wrapper:

Trusted Firmware-M documentation
################################

This section includes the official `Trusted Firmware-M (TF-M) <https://www.trustedfirmware.org/projects/tf-m/>`_ documentation.
It is provided for reference only and is intended for the developers working on the integration of TF-M in the nRF Connect SDK.

The section renders the content of the `official TF-M documentation <https://trustedfirmware-m.readthedocs.io/en/latest/index.html>`_ as-is using the sources from the downstream `TF-M repository <https://github.com/nrfconnect/sdk-trusted-firmware-m>`_.

For information on how TF-M is integrated in the nRF Connect SDK, see the `Security section in the nRF Connect SDK documentation <https://docs.nordicsemi.com/bundle/ncs-latest/page/nrf/security.html>`_.

The following table provides an overview of the TF-M features supported and not supported in the nRF Connect SDK.

.. list-table:: TF-M feature support in nRF Connect SDK
   :header-rows: 1

   * - TF-M feature
     - nRF Connect SDK support status
     - TF-M reference documentation
     - nRF Connect SDK configuration steps
   * - TF-M Initial Attestation
     - Supported
     - `Initial Attestation service <https://trustedfirmware-m.readthedocs.io/en/latest/components/initial_attestation.html>`_
     - `Initial Attestation configuration <https://docs.nordicsemi.com/bundle/ncs-latest/page/nrf/security.html#initial-attestation>`_
   * - TF-M Crypto
     - Supported (either fully or experimentally)
     - `Crypto service integration guide <https://docs.nordicsemi.com/bundle/ncs-latest/page/tfm/integration_guide/services/tfm_crypto_integration_guide.html>`_
     - `Crypto configuration <https://docs.nordicsemi.com/bundle/ncs-latest/page/nrf/security.html#crypto>`_
   * - TF-M Internal Trusted Storage
     - Supported
     - `Internal Trusted Storage service <https://trustedfirmware-m.readthedocs.io/en/latest/components/internal_trusted_storage.html>`_
     - `Internal Trusted Storage configuration <https://docs.nordicsemi.com/bundle/ncs-latest/page/nrf/security.html#internal-trusted-storage>`_
   * - Platform
     - Supported
     - `Platform service <https://trustedfirmware-m.readthedocs.io/en/latest/components/platform.html>`_
     - `Platform configuration <https://docs.nordicsemi.com/bundle/ncs-latest/page/nrf/security.html#platform>`_
   * - Protected Storage
     - Supported
     - `Protected Storage service <https://trustedfirmware-m.readthedocs.io/en/latest/components/ps.html>`_
     - `Protected Storage configuration <https://docs.nordicsemi.com/bundle/ncs-latest/page/nrf/security.html#protected-storage>`_
