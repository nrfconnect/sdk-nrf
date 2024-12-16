.. _jwt_application:

jwt generator
###################

.. contents::
   :local:
   :depth: 2

This sample shows how Application core can generate a JWT signed with the IAK.

Requirements
************

The sample supports the following development kits:

.. list-table:: Supported development kits
   :widths: 50 50
   :header-rows: 1

   * - Board
     - Support
   * - nrf9280pdk/nrf9280/cpuapp
     - Yes
   * - nrf54h20dk/nrf54h20/cpuapp
     - No

Overview
********

The sample goes through the following steps to generate a JWT:

1. Reads the device UUID.

   The returned UUID is compliant with UUID v4 defined by ITU-T X.667 | ISO/IEC 9834-8.

2. Generates a JWT.

   Uses the user provided fields for audiance and expiration delta, will always use IAK key to sign the JWT.
   The generated JWT is printed to serial terminal if project configuration allows it.

Configuration
*************

|config|

LIB JWT APP
===========

As per provided on the project config, the used APIs requier the usage of lib::app_jwt.

JWT signing verification
========================

Flag :kconfig:option:`CONFIG_APP_JWT_VERIFY_SIGNATURE` allow to verify the JWT signature against the IAK key.

Export public IAK key
=====================

User might be interrested in the DER formatted IAK key for later verifications of the generated JWT, the flag :kconfig:option:`CONFIG_APP_JWT_PRINT_EXPORTED_PUBKEY_DER` allows printing the DER formatted key to debug terminal.

Building and running
********************

   .. code-block:: console

      west build -p -b nrf9280pdk/nrf9280/cpuapp nrf/samples/jwt --build-dir build_cpuapp_nrf9280pdk_jwt_sample_logging_uart/ -T samples.jwt.logging.uart

Testing
=======

|test_sample|

1. |connect_terminal_specific|
#. Reset the kit.
#. Observe the following output (DER formatted public IAK key, and the JWT):

   .. code-block:: console

    jwt_sample: Application custom JWT sample (nrf9280pdk)
    user_app_jwt: pubkey_der (91) =  3059301306072a8648ce3d020106082a8648ce3d0301070342000491402ab677f6d49a0595a99a77156aa6e501b8f93efb23eccd41ee69e19c001f6e3da05925f953eff37ca9d7dba10fccfa747e6db28afbdc1f2be3d1867d3be1
    jwt_sample: jwt_generate generated json(517) : eyJ0eXAiOiJKV1QiLCJhbGciOiJFUzI1NiIsImtpZCI6ImQxOWU4YzY3Y2QzOTVkOGZiNTVkZTY5MmM1MmI1NjM3YWVkMWNiNTAzZDg0ZDI3MTExZjI3MmIwOWQwOWQxZTYifQ.eyJpYXQiOjQ4MzgsImp0aSI6Im5yZjkyODBwZGsuNjI3ZWI0ZmEtOGY4Yi1lYTI5LTJmMWQtMGQwOTg0OTg0ZWJlIiwiaXNzIjoibnJmOTI4MHBkay41MzEyNjhjZS0zYzA0LTExZWYtMzMwMS1mYmE5YzBmMGE0NzYiLCJzdWIiOiI1MzEyNjhjZS0zYzA0LTExZWYtMzMwMS1mYmE5YzBmMGE0NzYiLCJhdWQiOiJKU09OIHdlYiB0b2tlbiBmb3IgZGVtb25zdHJhdGlvbiIsImV4cCI6NTQzOH0.mFWn9Nj75KIzAGFdotB_PKXjTGr_L3uQiD9bMuwWxuRJiQ9vBt93gVK1ipukt9GTSAROvp7eBtY9RRqQTUiXbQ

   If an error occurs, the sample prints an error message.

.. note::
   Due to the extra long strings that the terminal has to print, be sure to configure :kconfig:option:`CONFIG_LOG_BUFFER_SIZE` with enough size to avoid truncated strings on the output.

.. note::
   Currently, the provided Sample doesn't support other boards than nrf9280pdk.

Dependencies
************

This sample uses the following |NCS| libraries:

* :ref:`lib_app_jwt`
