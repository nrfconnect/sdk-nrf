.. _lib_app_jwt:

Application JWT
###############

.. contents::
   :local:
   :depth: 2

The Application JWT library provides access to the `JSON Web Token (JWT)`_ generation feature from application core using signing and identity services from secure core.

Configuration
*************

To use the library to request a JWT, complete the following steps:

1. Set the following Kconfig options to enable the library:

   * :kconfig:option:`CONFIG_APP_JWT`
   * :kconfig:option:`CONFIG_APP_JWT_VERIFY_SIGNATURE`
   * :kconfig:option:`CONFIG_APP_JWT_PRINT_EXPORTED_PUBKEY_DER`
   * :kconfig:option:`CONFIG_NRF_SECURITY`
   * :kconfig:option:`CONFIG_SSF_DEVICE_INFO_SERVICE_ENABLED`

#. Generate a signing key pair if you do not want to use the IAK Key.
#. Populate the :c:struct:`app_jwt_data` structure with your desired values.
   See :ref:`app_jwt_values` for more information.
#. Pass the structure to the function that generates JWT (:c:func:`app_jwt_generate`).

If the function executes successfully, :c:member:`app_jwt_data.jwt_buf` will contain the JSON web token.

.. note::
   If a timestamp is needed and there is an error getting the time from the clock source (or the returned time in seconds is 0), the **iat** field will contain the value set by the :kconfig:option:`CONFIG_APP_JWT_DEFAULT_TIMESTAMP` Kconfig option.

.. _app_jwt_values:

Possible structure values
=========================

You can configure the following values in the :c:struct:`app_jwt_data` structure:

* :c:member:`app_jwt_data.sec_tag` - Optional, the ``sec_tag`` must contain a valid signing key.
  If set to ``0``, the library will use the IAK for signing.
* :c:member:`app_jwt_data.key_type` - Required if ``sec_tag`` is not zero.
  Defines the type of key in the sec tag.
* :c:member:`app_jwt_data.alg` - Required, always use the value ``JWT_ALG_TYPE_ES256``.
  Defines the JWT signing algorithm.
  Currently, only ECDSA 256 is supported.
* :c:member:`app_jwt_data.add_keyid_to_header` - Optional.
  Corresponds to **kid** claim.
  Use ``false`` if you want to leave out this field.
  If filled with the value ``true``, the claim **kid** will contain the SHA256 of the DER of the public part of the signing key.
* :c:member:`app_jwt_data.json_token_id` - Optional.
  Corresponds to **jti** claim.
  Use ``0`` if you want to leave out this field.
* :c:member:`app_jwt_data.subject` - Optional.
  Corresponds to **sub** claim.
  Use ``0`` if you want to leave out this field.
* :c:member:`app_jwt_data.audience` - Optional.
  Corresponds to **aud** claim.
  Use ``0`` if you want to leave out this field.
* :c:member:`app_jwt_data.issuer` - Optional.
  Corresponds to **iss** claim.
  Use ``0`` if you want to leave out this field.
* :c:member:`app_jwt_data.add_timestamp` - Optional.
  Corresponds to **iat** claim.
  Use ``false`` if you want to leave out this field.
  If filled with the value ``true``, the claim **iat** will be filled with the current timestamp in seconds.
* :c:member:`app_jwt_data.validity_s` - Optional.
  Defines the expiration date for the JWT.
  If set to ``0``, the field **exp** will be omitted from the generated JWT.
* :c:member:`app_jwt_data.jwt_buf` - Required.
  The buffer size must be from 600 to 900 bytes.
  You must provide a valid buffer.
  The library does not do any allocation.
* :c:member:`app_jwt_data.jwt_sz` - Size of the JWT buffer.
  Required, must be equal to the size of :c:member:`app_jwt_data.jwt_buf`.

API documentation
*****************

| Header file: :file:`include/app_jwt.h`
| Source file: :file:`lib/app_jwt/app_jwt.c`

.. doxygengroup:: app_jwt
