.. _jwt_application:

JWT generator
#############

.. contents::
   :local:
   :depth: 2

This sample demonstrates how Application core can generate a signed JWT.

Requirements
************

The sample supports the following development kits:

.. table-from-sample-yaml::

.. include:: /includes/tfm.txt

Overview
********

This code will generate two separate JWT : simplified (less fields) signed with a user generated key, and a complete JWT signed with the IAK key.
The sample goes through the following steps to generate a JWTs:

1. Fill in the fields for the JWT.

   For the purposes of this Sample, an ECDSA keypair is created and fed to the Secure Domain using PSA Crypto interfaces, the returned key_id is used as a `sec_tag` for signing the JWT.

   The key has to be of type ECDSA secp256r1 and the usage needs to be for message signing and message signature verification.

   The simplified JWT version, only the `sub` claim and the `exp` claim fields are filled.

   The full version, `sub`, `aud`, `iss`, `iat` and `exp` are filled and `kid` is added to the header.

2. Generates a JWT.

   The generated JWT is printed to serial terminal if project configuration allows it.

Configuration
*************

|config|

LIB JWT APP
===========

As per provided on the project config, the used APIs requier the usage of lib::app_jwt.

JWT signing verification
========================

Flag :kconfig:option:`CONFIG_APP_JWT_VERIFY_SIGNATURE` allow to verify the JWT signature against the signing key.

Export public part of the signing key
=====================================

You might be interrested in the DER formatted key for later verifications of the generated JWT, the flag :kconfig:option:`CONFIG_APP_JWT_PRINT_EXPORTED_PUBKEY_DER` allows printing the DER formatted key to debug terminal.
The DER key can be turned into PEM format by encoding it in base64 and adding the PEM markers `-----BEGIN PUBLIC KEY-----` and `-----END PUBLIC KEY-----`.

Building and running
********************

   .. code-block:: console

      west build -p -b nrf54h20dk/nrf54h20/cpuapp nrf/samples/jwt --build-dir build_cpuapp_nrf54h20dk_jwt_logging_serial/ -T samples.jwt.logging.uart

Testing
=======

|test_sample|

1. |connect_terminal_specific|
#. Reset the kit.
#. Observe the following output (DER formatted public IAK key, and the JWT):

   .. code-block:: console

    jwt_sample: Application JWT sample (nrf54h20dk)
    jwt_sample: JWT simplified token :
    app_jwt: sec_tag provided, value = 0x7FFFFFE2
    app_jwt: sec_tag provided, value = 0x7FFFFFE2
    app_jwt: pubkey_der (91) =  3059301306072a8648ce3d020106082a8648ce3d0301070342000469fa6c852bfc0b749838708b8c15292a78f1c26050cb6c32d53b69eb6c3589e4581477c417cb7c1eabc74ea2addb979a38ea3f5810b4be2eb5c4a98fc6ae2f4c
    app_jwt: issue_time is 0, will use value "1735682400"
    jwt_sample: JWT(218) :
    jwt_sample: eyJ0eXAiOiJKV1QiLCJhbGciOiJFUzI1NiJ9.eyJzdWIiOiJucmY1NGgyMGRrLjJiNzZmNjA2NWEyYTgyODcxYTIxOGYwMTVhMDhkZmM4IiwiZXhwIjoxNzM2Mjg3MjAwfQ.jQSl_p8LE1VCBxb1gZWbu0AyuTOslDxY5Oue1jX4UZyfKbcdS2GHnGn4VD2q1SNfeC_Ncpzh0Y1c4L3s-4uYSg
    jwt_sample: JWT full token :
    app_jwt: pubkey_der (91) =  3059301306072a8648ce3d020106082a8648ce3d03010703420004017e627cc237c5a37d9142d0cba1530a5653c4f41e6ba6e06d3b74fdf5c308b09afffd761d99946d5deb4dd97dbd0dbcba62c3d9ba518fc9e43be88b780b1484
    app_jwt: issue_time is 0, will use value "1735682400"
    jwt_sample: JWT(505) :
    jwt_sample: eyJ0eXAiOiJKV1QiLCJhbGciOiJFUzI1NiIsImtpZCI6IjRkMmM2NjdmNTYzMzExY2Q4OWE2N2M2ZWQ4ZDQwZDBmYWIyMzAyNTc3MjIyNjNjNDNkMThiNTM4YzZjN2JjZTkifQ.eyJqdGkiOiJucmY1NGgyMGRrLmI2ZjNkOGI1YjQzNmY5MDhkNjExYzYzZTE1ZTI1OTA1IiwiaXNzIjoibnJmNTRoMjBkay4zNzU4ZTE5NC03MjgyLTExZWYtOTMyYi03YjFmMDcwY2NlN2EiLCJzdWIiOiIzNzU4ZTE5NC03MjgyLTExZWYtOTMyYi03YjFmMDcwY2NlN2EiLCJhdWQiOiJKU09OIHdlYiB0b2tlbiBmb3IgZGVtb25zdHJhdGlvbiIsImV4cCI6MTczNjI4NzIwMH0.9CM8z1tdmDjqmrBd32EqmCWzCNj8wrnCusC7qFqXloYoU9GtowGNhliwt3ENqrAc2Rur2-szqazWWVrKQw5uMA

   If an error occurs, the sample prints an error message.

.. note::
   Currently, the provided Sample doesn't support other boards than nrf54h20dk.

Dependencies
************

This sample uses the following |NCS| libraries:

* :ref:`lib_app_jwt`
