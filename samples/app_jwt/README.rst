.. _app_jwt_sample:

Application JWT
###############

.. contents::
   :local:
   :depth: 2

This sample demonstrates how the application core can generate a signed JWT.

Requirements
************

The sample supports the following development kit:

.. table-from-sample-yaml::

.. include:: /includes/tfm.txt

Overview
********

This sample generates two separate JWTs - Simplified (less fields) signed with a user generated key, and a complete JWT signed with the IAK key.
The sample completes the following steps to generate a JWT:

#. Fill in the fields for the JWT.

   An ECDSA keypair is created and fed to the Secure Domain using PSA Crypto interfaces and the returned ``key_id`` is used as a ``sec_tag`` for signing the JWT.
   The key must be of type ECDSA secp256r1 and must be used for message signing and message signature verification.

   * For the simplified JWT version, only the **sub** claim and the **exp** claim fields are filled.
   * For the full version, **sub**, **aud**, **iss**, **iat**, and **exp** claim fields are filled and the **kid** field is added to the header.

#. Generate a JWT.

   The generated JWT is printed to the serial terminal.

Configuration
*************

|config|

JWT signing verification
========================

The :kconfig:option:`CONFIG_APP_JWT_VERIFY_SIGNATURE` Kconfig option allows to verify the JWT signature against the IAK key.

Export public part of the signing key
=====================================

If you want the key in the DER format for later verifications of the generated JWT, use the :kconfig:option:`CONFIG_APP_JWT_PRINT_EXPORTED_PUBKEY_DER` Kconfig option that prints the DER-formatted key to the debug terminal.
You can convert the DER key into PEM format by encoding it in base64 and adding the PEM markers ``-----BEGIN PUBLIC KEY-----`` and ``-----END PUBLIC KEY-----``.

Building and running
********************

.. |sample path| replace:: :file:`samples/app_jwt`

.. include:: /includes/build_and_run_ns.txt

Testing
=======

|test_sample|

1. |connect_terminal_specific|
#. Reset the kit.
#. Observe the following output (DER-formatted public key, and the JWT):

   .. code-block:: console

    jwt_sample: Application JWT sample (nrf54h20dk)
    jwt_sample: Generating simplified JWT token
    app_jwt: pubkey_der (91 bytes) =  3059301306072a8648ce3d020106082a8648ce3d0301070342000477284f32d5b9a68a69220b92ea1e48549dd25d225e12f44a74f4d3ab1559c544dbe1db85891ce77aec84e95ba7cc2764c4ab18dd19e505e63ccee9db2a98fd47
    jwt_sample: JWT(length 218):
    jwt_sample: eyJ0eXAiOiJKV1QiLCJhbGciOiJFUzI1NiJ9.eyJzdWIiOiJucmY1NGgyMGRrLmUwZmE3OGQ1NjkxNzM3ZmVmZjdhOTM3YTgyODc3ZTBkIiwiZXhwIjoxNzM2Mjg3MjAwfQ.p8YA6nBSDhmaHBaGXYfji7CFvmc22KtD_zMO8qOgsWVQi7iRnhSHzIbOfJcxcD3r38xNBsN2wSyvmJUqx2Vesg
    jwt_sample: Generating full JWT token
    app_jwt: pubkey_der (91 bytes) =  3059301306072a8648ce3d020106082a8648ce3d03010703420004017e627cc237c5a37d9142d0cba1530a5653c4f41e6ba6e06d3b74fdf5c308b09afffd761d99946d5deb4dd97dbd0dbcba62c3d9ba518fc9e43be88b780b1484
    jwt_sample: JWT(length 528):
    jwt_sample: eyJ0eXAiOiJKV1QiLCJhbGciOiJFUzI1NiIsImtpZCI6IjRkMmM2NjdmNTYzMzExY2Q4OWE2N2M2ZWQ4ZDQwZDBmYWIyMzAyNTc3MjIyNjNjNDNkMThiNTM4YzZjN2JjZTkifQ.eyJpYXQiOjE3MzU2ODI0MDAsImp0aSI6Im5yZjU0aDIwZGsuMmMwN2VmYTc2MTA4NjViYzQ5YjE1N2I5NDMwNTc2N2MiLCJpc3MiOiJucmY1NGgyMGRrLjM3NThlMTk0LTcyODItMTFlZi05MzJiLTdiMWYwNzBjY2U3YSIsInN1YiI6IjM3NThlMTk0LTcyODItMTFlZi05MzJiLTdiMWYwNzBjY2U3YSIsImF1ZCI6IkpTT04gd2ViIHRva2VuIGZvciBkZW1vbnN0cmF0aW9uIiwiZXhwIjoxNzM2Mjg3MjAwfQ.sc6tcj5j4s-TTd0nRXW77yr6lfu62575_LzzHSMXsR32HEvnx30wIUR-t1Diidy2eC0lL1inYR_8nUNDTREaRQ

   If an error occurs, the sample prints an error message.


Dependencies
************

This sample uses the following |NCS| library:

* :ref:`lib_app_jwt`
