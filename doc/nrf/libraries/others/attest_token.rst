.. _lib_attest_token:
.. _lib_modem_attest_token:

Attestation token
#################

.. contents::
   :local:
   :depth: 2

The Attestation token library is used to obtain an attestation token from an nRF91 Series or nRF92 Series device.

Attestation tokens are used to verify the authenticity of the device.
An attestation token includes the device's internal UUID.
If the device is an nRF91 Series device, it also includes the UUID of the modem firmware that is installed in the device.
An attestation token consists of two base64url strings.
The format is ``base64url1.base64url2``.
The first base64url string (``base64url1``) is the Device Identity Attestation message, which is a CBOR encoded payload containing the device UUID, device type, and modem firmware UUID.
The second base64url string (``base64url2``) is the `CBOR Object Signing and Encryption (COSE)`_ authentication metadata.

To use the library to obtain an attestation token, complete the following steps:

1. Enable the Attestation token library (:kconfig:option:`CONFIG_ATTEST_TOKEN`).
#. Call the :c:func:`attest_token_get` function to obtain the two base64url strings in the :c:struct:`nrf_attestation_token` structure.

You can also use the library to parse an attestation token.
To parse an attestation token, complete the following steps:

1. Enable token parsing (:kconfig:option:`CONFIG_ATTEST_TOKEN_PARSING`).
#. Call the :c:func:`attest_token_parse` function to parse the token.

The function :c:func:`attest_token_parse` parses :c:struct:`nrf_attestation_token` and populates the :c:struct:`nrf_attestation_data` structure with the data.
To obtain only the device UUID string or the modem firmware UUID string or both, call the :c:func:`attest_token_get_uuids` function.

Configuration
*************

Configure the following options when using this library:

* :kconfig:option:`CONFIG_ATTEST_TOKEN`
* :kconfig:option:`CONFIG_ATTEST_TOKEN_PARSING`

API documentation
*****************

| Header file: :file:`include/modem/attest_token.h`
| Source file: :file:`lib/attest_token/attest_token.c`

.. doxygengroup:: attest_token
