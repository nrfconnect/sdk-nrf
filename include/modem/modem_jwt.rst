.. _lib_modem_jwt:

Modem JWT
#########

.. contents::
   :local:
   :depth: 2

The Modem JWT library provides access to the modem's `JSON Web Token (JWT)`_ generation feature, which is available in modem firmware v1.3.0 or higher.

To use the library to request a JWT, complete the following steps:

* Enable the library.
* Populate the :c:struct:`jwt_data` structure with your desired values.
* Pass the structure to the function that generates JWT (:c:func:`modem_jwt_generate`).

You can configure the following values in the :c:struct:`jwt_data` structure:

* :c:member:`jwt_data.sec_tag` - Required. The sec tag must contain a valid signing key.
* :c:member:`jwt_data.key` - Required. Defines the type of key in the sec tag.
* :c:member:`jwt_data.alg` - Required. Defines the JWT signing algorithm. Currently, only ECDSA 256 is supported.
* :c:member:`jwt_data.exp_delta_s` - Optional. If set, and if the modem has a valid date and time, the ``iat`` and ``exp`` claims are populated.
* :c:member:`jwt_data.subject` - Optional. Corresponds to ``sub`` claim.
* :c:member:`jwt_data.audience` - Optional. Corresponds to ``aud`` claim.
* :c:member:`jwt_data.jwt_buf` - Optional. Buffer for the generated, null-terminated, JWT string. If a buffer is not provided, the library will allocate memory.
* :c:member:`jwt_data.jwt_sz` - Size of JWT buffer. Required if :c:member:`jwt_data.jwt_buf` is set.

If the function executes successfully, :c:member:`jwt_data.jwt_buf` will contain the JSON Web Token.

Configuration
*************

Configure the following options when using the library:

* :option:`CONFIG_MODEM_JWT`
* :option:`CONFIG_MODEM_JWT_MAX_LEN`

API documentation
*****************

| Header file: :file:`include/modem/modem_jwt.h`
| Source file: :file:`lib/modem/modem_jwt.c`

.. doxygengroup:: modem_jwt
   :project: nrf
   :members:
