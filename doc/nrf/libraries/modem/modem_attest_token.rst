.. _lib_modem_attest_token:

Modem attestation token
#######################

.. contents::
   :local:
   :depth: 2

The Modem attestation token library is used to obtain an attestation token from an nRF91 Series device.

Attestation tokens are used to verify the authenticity of the device.
An attestation token includes the device's internal UUID and the UUID of the modem firmware that is installed in the device.
An attestation token consists of two base64url strings.
The format is ``base64url1.base64url2``.
The first base64url string (``base64url1``) is the Device Identity Attestation message, which is a CBOR encoded payload containing the device UUID, device type, and modem firmware UUID.
The second base64url string (``base64url2``) is the `CBOR Object Signing and Encryption (COSE)`_ authentication metadata.

To use the library to obtain the attestation token, complete the following steps:

* Enable the Modem attestation token library.
* Initialize the :ref:`nrf_modem_lib_readme` and :ref:`at_cmd_parser_readme` libraries.
* Call the :c:func:`modem_attest_token_get` function to obtain the two base64url strings in the :c:struct:`nrf_attestation_token` structure.
* Enable token parsing (:kconfig:option:`CONFIG_MODEM_ATTEST_TOKEN_PARSING`).
* Call the :c:func:`modem_attest_token_parse` function to parse the token.

The function :c:func:`modem_attest_token_parse` parses :c:struct:`nrf_attestation_token` and populates the :c:struct:`nrf_attestation_data` structure with the data.
To obtain only the device UUID string or the modem firmware UUID string or both, you must call :c:func:`modem_attest_token_get_uuids`.

Configuration
*************

Configure the following options when using this library:

* :kconfig:option:`CONFIG_MODEM_ATTEST_TOKEN`
* :kconfig:option:`CONFIG_MODEM_ATTEST_TOKEN_PARSING`

API documentation
*****************

| Header file: :file:`include/modem/modem_attest_token.h`
| Source file: :file:`lib/modem/modem_attest_token.c`

.. doxygengroup:: modem_attest_token
   :project: nrf
   :members:
