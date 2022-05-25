.. _lib_modem_jwt:

Modem JWT
#########

.. contents::
   :local:
   :depth: 2

The Modem JWT library provides access to the modem's `JSON Web Token (JWT)`_ generation feature, which is available in modem firmware v1.3.0 or higher.

Configuration
*************

To use the library to request a JWT, complete the following steps:

1. Set the following Kconfig options to enable the library:

   * :kconfig:option:`CONFIG_MODEM_JWT`
   * :kconfig:option:`CONFIG_MODEM_JWT_MAX_LEN`

#. Populate the :c:struct:`jwt_data` structure with your desired values.
   See `Possible structure values`_ for more information.
#. Pass the structure to the function that generates JWT (:c:func:`modem_jwt_generate`).

If the function executes successfully, :c:member:`jwt_data.jwt_buf` will contain the JSON Web Token.

Possible structure values
=========================

You can configure the following values in the :c:struct:`jwt_data` structure:

* :c:member:`jwt_data.sec_tag` - Optional.
  The ``sec_tag`` must contain a valid signing key.
  If zero is given, the modem uses its own key for signing.
* :c:member:`jwt_data.key` - Required if ``sec_tag`` is not zero.
  Defines the type of key in the sec tag.
* :c:member:`jwt_data.alg` - Required if ``sec_tag`` is not zero.
  Defines the JWT signing algorithm. Currently, only ECDSA 256 is supported.
* :c:member:`jwt_data.exp_delta_s` - Optional.
  If set, and if the modem has a valid date and time, the ``iat`` and ``exp`` claims are populated.
* :c:member:`jwt_data.subject` - Optional.
  Corresponds to ``sub`` claim.
  Use ``NULL`` if you want to leave out this field.
* :c:member:`jwt_data.audience` - Optional.
  Corresponds to ``aud`` claim.
  Use ``NULL`` if you want to leave out this field.
* :c:member:`jwt_data.jwt_buf` - Optional.
  Buffer for the generated, null-terminated, JWT string.
  If a buffer is not provided, the library will allocate memory.
* :c:member:`jwt_data.jwt_sz` - Size of JWT buffer.
  Required if :c:member:`jwt_data.jwt_buf` is set.

API documentation
*****************

| Header file: :file:`include/modem/modem_jwt.h`
| Source file: :file:`lib/modem/modem_jwt.c`

.. doxygengroup:: modem_jwt
   :project: nrf
   :members:
