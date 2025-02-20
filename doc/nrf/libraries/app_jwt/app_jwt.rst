.. _app_jwt:

Application JWT
###################

The Application JWT library provides access to the `JSON Web Token (JWT)`_ generation feature from application core using signing and identity services from secure core.

Configuration
*************

To use the library to request a JWT, complete the following steps:

1. Set the following Kconfig options to enable the library:

   * :kconfig:option:`CONFIG_APP_JWT`
   * :kconfig:option:`CONFIG_APP_JWT_VERIFY_SIGNATURE`
   * :kconfig:option:`CONFIG_APP_JWT_PRINT_EXPORTED_PUBKEY_DER`
   * :kconfig:option:`CONFIG_NRF_SECURITY`
   * :Kconfig:option:`CONFIG_SSF_PSA_CRYPTO_SERVICE_ENABLED`
   * :Kconfig:option:`CONFIG_SSF_DEVICE_INFO_SERVICE_ENABLED`

#. Read the device UUID (:c:func:`app_jwt_get_uuid`)
#. Populate the :c:struct:`app_jwt_data` structure with your desired values.
   See `Possible structure values`_ for more information.
#. Pass the structure to the function that generates JWT (:c:func:`app_jwt_generate`).

If the function executes successfully, :c:member:`app_jwt_data.jwt_buf` will contain the JSON Web Token.

.. note::
   The library doesn't check the validity of the time source, it is up to the caller to make sure that the system has access to a valid one, otherwise "iat" field will containt the time since boot in seconds.

Possible structure values
=========================

You can configure the following values in the :c:struct:`app_jwt_data` structure:

* :c:member:`app_jwt_data.sec_tag` - Optional, kept for compatibility reasons.
  The ``sec_tag`` must contain a valid signing key.
  Will always be ignored, the library will use the IAK for signing.
* :c:member:`app_jwt_data.key` - Required if ``sec_tag`` is not zero.
  Defines the type of key in the sec tag.
  Will always be ignored, the library will use the IAK for signing.
* :c:member:`app_jwt_data.alg` - Required, always use the value JWT_ALG_TYPE_ES256.
  Defines the JWT signing algorithm. Currently, only ECDSA 256 is supported.
* :c:member:`app_jwt_data.validity_s` - Optional.
  Defines the expiration date for the JWT.
  If set to 0, the field ``exp`` will be omitted from the generated JWT.
* :c:member:`app_jwt_data.subject` - Optional.
  Corresponds to ``sub`` claim.
  Use ``NULL`` if you want to leave out this field.
* :c:member:`app_jwt_data.audience` - Optional.
  Corresponds to ``aud`` claim.
  Use ``NULL`` if you want to leave out this field.
* :c:member:`app_jwt_data.jwt_buf` - Required.
  Buffer for the generated, null-terminated, JWT string.
  Buffer size has to be al least 600 bytes, at most 900 bytes.
  The user has to provide a valid buffer, library doesn't do any allocation.
* :c:member:`app_jwt_data.jwt_sz` - Size of JWT buffer.
  Required, has to be equal to the size of :c:member:`app_jwt_data.jwt_buf`.

Hardcoded values in the generated JWT
=====================================

The generated JWT will always contain the following information in the next fields:

* ``kid`` - KeyID.
  SHA256 of the signing key.
* ``jti`` - Json Token ID.
  will always contain <Board_ID>.<16 Random bytes formatted as UUID>.
* ``iss`` - Issuer.
  will always contain <Board_ID>.<content of ``sub`` claim>.


API documentation
*****************

| Header file: :file:`include/app_jwt.h`
| Source file: :file:`lib/app_jwt/app_jwt.c`

.. doxygengroup:: app_jwt
