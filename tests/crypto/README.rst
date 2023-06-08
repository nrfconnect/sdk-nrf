.. _crypto_test:

Cryptography tests
##################

.. contents::
   :local:
   :depth: 2

Cryptography tests verify the functionality of the :ref:`nrf_security` by using known test vectors approved by the National Institute of Standards and Technology (NIST) and others.

Requirements
************

The tests support the following development kits:

.. table-from-rows:: /includes/sample_board_rows.txt
   :header: heading
   :rows: nrf5340dk_nrf5340_cpuapp, nrf9160dk_nrf9160, nrf52840dk_nrf52840

.. note::
   Nordic Semiconductor devices such as nRF51, nRF52810, or nRF52811 cannot run the full test suite because of limited flash capacity.
   A recommended approach in such case is to run subsets of the tests one by one.

Overview
********

Cryptography tests use Zephyr Test Framework (Ztest) to run the tests.
See :ref:`zephyr:test-framework` for details.
The tests do not use the standard Ztest output but provide custom output for the test reports.
See :ref:`crypto_test_ztest_custom` for details.

The tests are executed if the cryptographic functionality is enabled in Kconfig.
Make sure to configure :ref:`nrf_security` and all available hardware or software backends to enable the tests.
See :kconfig:option:`CONFIG_NORDIC_SECURITY_BACKEND`.

+--------------------+-------------+----------------------------------------------------------------------------+----------------------------------------------------------------------------+
| Cryptographic mode |   Sub-mode  |                          Link to standard                                  |                              Test Vector Source                            |
+====================+=============+============================================================================+============================================================================+
| AES                | CBC         | `NIST - AES`_                                                              | `NIST - AES`_                                                              |
+--------------------+-------------+----------------------------------------------------------------------------+----------------------------------------------------------------------------+
|                    | CFB         |                                                                            | `NIST - AES`_                                                              |
+--------------------+-------------+----------------------------------------------------------------------------+----------------------------------------------------------------------------+
|                    | ECB         |                                                                            | `NIST - AES`_                                                              |
+--------------------+-------------+----------------------------------------------------------------------------+----------------------------------------------------------------------------+
|                    | CTR         | `NIST SP 800-38A`_                                                         | `NIST SP 800-38A`_                                                         |
+--------------------+-------------+----------------------------------------------------------------------------+----------------------------------------------------------------------------+
|                    | CBC MAC     | CBC MAC                                                                    | No official test vectors                                                   |
+--------------------+-------------+----------------------------------------------------------------------------+----------------------------------------------------------------------------+
|                    | CMAC        | `NIST SP 800-38B`_                                                         | `NIST SP 800-38B`_                                                         |
+--------------------+-------------+----------------------------------------------------------------------------+----------------------------------------------------------------------------+
| AEAD               | CCM         | `NIST - CCM`_                                                              | `NIST - CCM`_                                                              |
+--------------------+-------------+----------------------------------------------------------------------------+----------------------------------------------------------------------------+
|                    | CCM*        | Formal Specification of the CCM*                                           | Formal Specification of the CCM*                                           |
|                    |             | Mode of Operation - September 9, 2005                                      | Mode of Operation - September 9, 2005                                      |
+--------------------+-------------+----------------------------------------------------------------------------+----------------------------------------------------------------------------+
|                    | EAX         | `The EAX Mode of Operation`_                                               | `The EAX Mode of Operation`_                                               |
+--------------------+-------------+----------------------------------------------------------------------------+----------------------------------------------------------------------------+
|                    | GCM         | `NIST - GCM`_                                                              | `NIST - GCM`_                                                              |
+--------------------+-------------+----------------------------------------------------------------------------+----------------------------------------------------------------------------+
|                    | ChaCha-Poly | `RFC 7539 - ChaCha20 and Poly1305 for IETF Protocols`_                     | `RFC 7539 - ChaCha20 and Poly1305 for IETF Protocols`_                     |
+--------------------+-------------+----------------------------------------------------------------------------+----------------------------------------------------------------------------+
| ECDH               | secp160r1   | `NIST - ECDH`_                                                             | GEC 2: Test Vectors for SEC 1                                              |
+--------------------+-------------+----------------------------------------------------------------------------+----------------------------------------------------------------------------+
|                    | secp160r2   |                                                                            | No test vectors                                                            |
+--------------------+-------------+----------------------------------------------------------------------------+----------------------------------------------------------------------------+
|                    | secp192r1   |                                                                            | `NIST - ECDH`_                                                             |
+--------------------+-------------+----------------------------------------------------------------------------+----------------------------------------------------------------------------+
|                    | secp224r1   |                                                                            | `NIST - ECDH`_                                                             |
+--------------------+-------------+----------------------------------------------------------------------------+----------------------------------------------------------------------------+
|                    | secp256r1   |                                                                            | `NIST - ECDH`_                                                             |
+--------------------+-------------+----------------------------------------------------------------------------+----------------------------------------------------------------------------+
|                    | secp384r1   |                                                                            | `NIST - ECDH`_                                                             |
+--------------------+-------------+----------------------------------------------------------------------------+----------------------------------------------------------------------------+
|                    | secp521r1   |                                                                            | `NIST - ECDH`_                                                             |
+--------------------+-------------+----------------------------------------------------------------------------+----------------------------------------------------------------------------+
|                    | secp160k1   |                                                                            | No test vectors                                                            |
+--------------------+-------------+----------------------------------------------------------------------------+----------------------------------------------------------------------------+
|                    | secp192k1   |                                                                            | No test vectors                                                            |
+--------------------+-------------+----------------------------------------------------------------------------+----------------------------------------------------------------------------+
|                    | secp224k1   |                                                                            | No test vectors                                                            |
+--------------------+-------------+----------------------------------------------------------------------------+----------------------------------------------------------------------------+
|                    | secp256k1   |                                                                            | No test vectors                                                            |
+--------------------+-------------+----------------------------------------------------------------------------+----------------------------------------------------------------------------+
|                    | bp256r1     | `RFC 7027 - ECC Brainpool Curves for TLS`_                                 | `RFC 7027 - ECC Brainpool Curves for TLS`_                                 |
+--------------------+-------------+----------------------------------------------------------------------------+----------------------------------------------------------------------------+
|                    | bp384r1     |                                                                            | `RFC 7027 - ECC Brainpool Curves for TLS`_                                 |
+--------------------+-------------+----------------------------------------------------------------------------+----------------------------------------------------------------------------+
|                    | bp512r1     |                                                                            | `RFC 7027 - ECC Brainpool Curves for TLS`_                                 |
+--------------------+-------------+----------------------------------------------------------------------------+----------------------------------------------------------------------------+
|                    | Curve25519  | `RFC 7748 - Elliptic Curves for Security`_                                 | `RFC 7748 - Elliptic Curves for Security`_                                 |
+--------------------+-------------+----------------------------------------------------------------------------+----------------------------------------------------------------------------+
| ECDSA              | secp160r1   | `NIST - ECDSA`_                                                            | No test vectors                                                            |
+--------------------+-------------+----------------------------------------------------------------------------+----------------------------------------------------------------------------+
|                    | secp160r2   |                                                                            | No test vectors                                                            |
+--------------------+-------------+----------------------------------------------------------------------------+----------------------------------------------------------------------------+
|                    | secp192r1   |                                                                            | No test vectors                                                            |
+--------------------+-------------+----------------------------------------------------------------------------+----------------------------------------------------------------------------+
|                    | secp224r1   |                                                                            | `NIST - ECDSA`_                                                            |
+--------------------+-------------+----------------------------------------------------------------------------+----------------------------------------------------------------------------+
|                    | secp256r1   |                                                                            | `NIST - ECDSA`_                                                            |
+--------------------+-------------+----------------------------------------------------------------------------+----------------------------------------------------------------------------+
|                    | secp384r1   |                                                                            | `NIST - ECDSA`_                                                            |
+--------------------+-------------+----------------------------------------------------------------------------+----------------------------------------------------------------------------+
|                    | secp521r1   |                                                                            | `NIST - ECDSA`_                                                            |
+--------------------+-------------+----------------------------------------------------------------------------+----------------------------------------------------------------------------+
|                    | secp160k1   |                                                                            | No test vectors                                                            |
+--------------------+-------------+----------------------------------------------------------------------------+----------------------------------------------------------------------------+
|                    | secp192k1   |                                                                            | No test vectors                                                            |
+--------------------+-------------+----------------------------------------------------------------------------+----------------------------------------------------------------------------+
|                    | secp224k1   |                                                                            | No test vectors                                                            |
+--------------------+-------------+----------------------------------------------------------------------------+----------------------------------------------------------------------------+
|                    | secp256k1   |                                                                            | No test vectors                                                            |
+--------------------+-------------+----------------------------------------------------------------------------+----------------------------------------------------------------------------+
|                    | bp256r1     | `RFC 5639 - ECC Brainpool Standard Curves and Curve Generation`_           | No test vectors                                                            |
+--------------------+-------------+----------------------------------------------------------------------------+----------------------------------------------------------------------------+
|                    | bp384r1     |                                                                            | No test vectors                                                            |
+--------------------+-------------+----------------------------------------------------------------------------+----------------------------------------------------------------------------+
|                    | bp512r1     |                                                                            | No test vectors                                                            |
+--------------------+-------------+----------------------------------------------------------------------------+----------------------------------------------------------------------------+
|                    | EdDSA       | `RFC 8032 - Edwards-Curve Digital Signature Algorithm (EdDSA)`_            | `RFC 8032 - Edwards-Curve Digital Signature Algorithm (EdDSA)`_            |
+--------------------+-------------+----------------------------------------------------------------------------+----------------------------------------------------------------------------+
| Hash               | SHA256      | `NIST - SHA`_                                                              | `NIST - SHA`_                                                              |
+--------------------+-------------+----------------------------------------------------------------------------+----------------------------------------------------------------------------+
|                    | SHA512      |                                                                            | `NIST - SHA`_                                                              |
+--------------------+-------------+----------------------------------------------------------------------------+----------------------------------------------------------------------------+
| HMAC               | HMAC SHA256 | `NIST - HMAC`_                                                             | `NIST - HMAC`_                                                             |
+--------------------+-------------+----------------------------------------------------------------------------+----------------------------------------------------------------------------+
|                    | HMAC SHA512 |                                                                            | `NIST - HMAC`_                                                             |
+--------------------+-------------+----------------------------------------------------------------------------+----------------------------------------------------------------------------+
| HKDF               | HKDF SHA256 | `RFC 5869 - HMAC-based Extract-and-Expand Key Derivation Function (HKDF)`_ | `RFC 5869 - HMAC-based Extract-and-Expand Key Derivation Function (HKDF)`_ |
+--------------------+-------------+----------------------------------------------------------------------------+----------------------------------------------------------------------------+
|                    | HKDF SHA512 |                                                                            | `RFC 5869 - HMAC-based Extract-and-Expand Key Derivation Function (HKDF)`_ |
+--------------------+-------------+----------------------------------------------------------------------------+----------------------------------------------------------------------------+
| EC-JPAKE           | secp256r1   | `J-PAKE: Password-Authenticated Key Exchange by Juggling`_                 | Custom                                                                     |
+--------------------+-------------+----------------------------------------------------------------------------+----------------------------------------------------------------------------+

Building and running
********************

.. |test path| replace:: :file:`tests/crypto/`

.. include:: /includes/build_and_run_test.txt

There are multiple ways to build the tests.
See :ref:`nrf_security` for additional information about configuring the Nordic Security Module.
You can use the following configuration files to build the test in a specific setup:

* :file:`overlay-cc3xx.conf` uses hardware acceleration using the Arm CryptoCell accelerator (for cryptography and entropy for random number generation).
* :file:`overlay-cc3xx-oberon.conf` uses a combination of hardware acceleration, using the Arm CryptoCell, and the Oberon software library, that adds key sizes and algorithms not supported in the CryptoCell.
  This setup uses hardware acceleration as much as possible.
* :file:`overlay-oberon.conf` uses only the Oberon software library for all cryptographic operations.
* :file:`overlay-vanilla.conf` is for software only, except for a hardware-accelerated module to generate entropy for random number generation.
* :file:`overlay-multi.conf` uses a combination of hardware acceleration, using the Arm CryptoCell, and vanilla Mbed TLS and Oberon software implementations to support functionalities not supported by the CryptoCell.
  This setup uses hardware acceleration as much as possible.

You can use one of the listed overlay configurations by adding the ``-- -DOVERLAY_CONFIG=<overlay_config_file>`` flag to your build. Also see :ref:`cmake_options` for instructions on how to add this option.

.. _crypto_test_ztest_custom:

Ztest custom log formatting
===========================

Cryptography tests replace the standard Ztest formatting to assure more efficient reporting of running tests and test results.
Set the configuration option :kconfig:option:`CONFIG_ZTEST_TC_UTIL_USER_OVERRIDE` to replace the Ztest macros ``TC_START`` and ``Z_TC_END_RESULT`` with versions more suited for reporting results of cryptographic tests.

:kconfig:option:`CONFIG_ZTEST_TC_UTIL_USER_OVERRIDE` uses :file:`tests/crypto/include_override/tc_util_user_override.h` to define the custom formatting.

.. _crypto_test_testing:

Testing
=======

1. Compile and program the application.
#. Observe the result of the different test vectors in the log using :ref:`RTT Viewer <zephyr:nordic_segger>` or a terminal emulator.
   The last line of the output indicates the test result::

      PROJECT EXECUTION SUCCESSFUL

Additional test cases and test vectors
======================================

You can add test cases and test vectors to the test suite either by including additional source files or by extending the existing files.


Test case
---------

A test case is a function designed to verify parts of the functionality of a cryptographic operation.
Most cryptographic operations, like hash calculations and ECDH, have multiple test cases to be able to cover all features.
A typical test case is called by looping over the registered test vectors and calling the test case.
The execution logs the verdict for each test vector.

Registering a test case
~~~~~~~~~~~~~~~~~~~~~~~

A new test case must be registered to the ``test_case_data`` section using ``ITEM_REGISTER``, which registers it with the named section ``test_case_hmac_data``::

   ITEM_REGISTER(test_case_hmac_data, test_case_t test_hmac) = {
   .p_test_case_name = "HMAC",
   .setup = hmac_setup,
   .exec = exec_test_case_hmac,
   .teardown = hmac_teardown,
   .vector_type = TV_HMAC,
   .vectors_start = __start_test_vector_hmac_data,
   .vectors_stop = __stop_test_vector_hmac_data,
   };


.. note::
   The macro call to ``ITEM_REGISTER`` must be done in a :file:`.c` file.

Setting up a test case
~~~~~~~~~~~~~~~~~~~~~~

As part of the test case setup, any previously used buffers are cleared.
The next test vector is fetched using the ``ITEM_GET`` macro.
The macro requires the following parameters:

* ``test_vector_hmac_data`` - The section to fetch the test vector from (HMAC in this example).
* ``test_vector_hmac_t`` - Information about which type of test vector to expect in the given section.
  In the example, ``test_vector_hmac_t`` is expected.
  It is the same type that is used when registering HMAC test vectors.
* Information about which index to fetch a test vector from.

The fetched test vector is then unhexified.

Test vector data is stored as strings of hexadecimal characters.
To use them, they must be parsed to binary, which is also done in the setup procedure.

The following example shows a test vector setup::

   void hmac_setup(void)
   {
      hmac_clear_buffers();
      p_test_vector = ITEM_GET(test_vector_hmac_data, test_vector_hmac_t,
                   hmac_vector_n);
      unhexify_hmac();
   }
   void exec_test_case_hmac(void)
   ...

On teardown, the test vector index is incremented, so that the next call to ``hmac_setup`` by the Ztest framework fetches the next test vector::

   void hmac_combined_teardown(void)
   {
      hmac_combined_vector_n++;
   }

Executing a test case
~~~~~~~~~~~~~~~~~~~~~

An example of an HMAC test case in a simplified form is shown below::

   void exec_test_case_hmac(void)
   {
   int err_code = -1;

   /* Initialize the HMAC module. */
   mbedtls_md_init(&md_context);

   const mbedtls_md_info_t *p_md_info =
	   mbedtls_md_info_from_type(p_test_vector->digest_type);
   err_code = mbedtls_md_setup(&md_context, p_md_info, 1);
   if (err_code != 0) {
	   LOG_WRN("mb setup ec: -0x%02X", -err_code);
   }
   TEST_VECTOR_ASSERT_EQUAL(err_code, 0);

   start_time_measurement();
   err_code = mbedtls_md_hmac_starts(&md_context, m_hmac_key_buf, key_len);
   TEST_VECTOR_ASSERT_EQUAL(err_code, 0);

   err_code =
	   mbedtls_md_hmac_update(&md_context, m_hmac_input_buf, in_len);
   TEST_VECTOR_ASSERT_EQUAL(err_code, 0);

   /* Finalize the HMAC computation. */
   err_code = mbedtls_md_hmac_finish(&md_context, m_hmac_output_buf);
   stop_time_measurement();

   TEST_VECTOR_ASSERT_EQUAL(p_test_vector->expected_err_code, err_code);

   /* Verify the generated HMAC. */
   TEST_VECTOR_ASSERT_EQUAL(expected_hmac_len, hmac_len);
   TEST_VECTOR_MEMCMP_ASSERT(m_hmac_output_buf, m_hmac_expected_output_buf,
	   		     expected_hmac_len,
			     p_test_vector->expected_result,
			     "Incorrect hmac");

   mbedtls_md_free(&md_context);
   }


Test vectors
------------

A test vector is a set of inputs and expected outputs to verify the functionality provided in a test case::

   typedef const struct {
	   const uint32_t digest_type;        /**< Digest type of HMAC operation. */
	   const int expected_err_code;    /**< Expected error code from HMAC operation. */
	   const uint8_t expected_result;  /**< Expected result of HMAC operation. */
	   const char *p_test_vector_name; /**< Pointer to HMAC test vector name. */
	   const char
		   *p_input;                   /**< Pointer to input message in hex string format. */
	   const char *p_key;              /**< Pointer to HMAC key in hex string format. */
	   const char *p_expected_output;  /**< Pointer to expected HMAC digest in hex string format. */
   } test_vector_hmac_t;

Registering a test vector
-------------------------

Test vectors are added by registering them for a section defined in the test case code.
The test vector is registered in the section ``test_vector_hmac_data``, which is defined in the test case example ``exec_test_case_hmac``.
The test vector can reuse the already defined hash test vector structure ``test_vector_hmac_t``, as shown in the code block below::

   /* HMAC - Custom test vector */
   ITEM_REGISTER(test_vector_hmac_data,
	         test_vector_hmac_t test_vector_hmac256_min_key_min_message_0) = {
	   .digest_type = MBEDTLS_MD_SHA256,
	   .expected_err_code = 0,
	   .expected_result = EXPECTED_TO_PASS,
	   .p_test_vector_name = TV_NAME("SHA256 key_len=1 message_len=1 zeros"),
	   .p_input = "00",
	   .p_key = "00",
	   .p_expected_output =
		   "6620b31f2924b8c01547745f41825d322336f83ebb13d723678789d554d8a3ef"
   };


Output logging
--------------

The test project generates a test log using RTT or UART output.
Executing ``exec_test_case_hmac`` with its registered test vectors adds the following output to the test log::

   Running test suite HMAC
   357: SHA256 key_len=131 message_len=152 -- PASS -- [../test_cases/test_vectors_hmac.c:259]
   358: SHA256 key_len=131 message_len=54 -- PASS -- [../test_cases/test_vectors_hmac.c:240]
   359: SHA256 key_len=25 message_len=50 -- PASS -- [../test_cases/test_vectors_hmac.c:225]
   360: SHA256 key_len=20 message_len=50 -- PASS -- [../test_cases/test_vectors_hmac.c:210]
   361: SHA256 key_len=4 message_len=28 -- PASS -- [../test_cases/test_vectors_hmac.c:197]
   362: SHA256 key_len=20 message_len=8 -- PASS -- [../test_cases/test_vectors_hmac.c:184]
   363: SHA256 key_len=74 message_len=128 -- PASS -- [../test_cases/test_vectors_hmac.c:164]
   364: SHA256 key_len=64 message_len=128 -- PASS -- [../test_cases/test_vectors_hmac.c:145]
   365: SHA256 key_len=45 message_len=128 -- PASS -- [../test_cases/test_vectors_hmac.c:127]
   366: SHA256 key_len=40 message_len=128 -- PASS -- [../test_cases/test_vectors_hmac.c:109]
   367: SHA256 key_len=1 message_len=1 non-zeros -- PASS -- [../test_cases/test_vectors_hmac.c:96]
   368: SHA256 key_len=1 message_len=1 zeros -- PASS -- [../test_cases/test_vectors_hmac.c:82]
   369: SHA256 invalid - signature changed -- PASS -- [../test_cases/test_vectors_hmac.c:64]
   370: SHA256 invalid - key changed -- PASS -- [../test_cases/test_vectors_hmac.c:46]
   371: SHA256 invalid - message changed -- PASS -- [../test_cases/test_vectors_hmac.c:28]
   Test suite HMAC succeeded
