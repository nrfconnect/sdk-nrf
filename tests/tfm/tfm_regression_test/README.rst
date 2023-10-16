.. _tfm_regression_test:

TF-M: Regression tests
######################

.. contents::
   :local:
   :depth: 2

Use this test sample to run secure and non-secure Trusted Firmware-M (TF-M) regression tests.

Requirements
************

This test sample requires that the device is provisioned with the PSA root-of-trust security parameters using the :ref:`provisioning image <provisioning_image>` sample.
Build and flash the provisioning image sample before using the test sample.

The tests support the following development kits:

.. table-from-rows:: /includes/sample_board_rows.txt
   :header: heading
   :rows: nrf5340dk_nrf5340_cpuapp_ns, nrf9160dk_nrf9160_ns

Overview
********

Run both the secure and non-secure TF-M regression tests using the Zephyr build system.

The build system will replace the Zephyr application with the non-secure TF-M test application, while the secure tests will be included in the TF-M build itself.

The TF-M regression tests are implemented in the tf-m-tests repo: https://git.trustedfirmware.org/TF-M/tf-m-tests.git/.

Configuration
*************

Tests for both the secure and non-secure area are enabled by default, but can be controlled via the Kconfig options :kconfig:option:`CONFIG_TFM_REGRESSION_S` and :kconfig:option:`CONFIG_TFM_REGRESSION_NS` respectively.

|config|

Building and running
********************

Do not flash with ``--erase``, ``--recover``, or similar, as this will erase the PSA platform security parameters.

.. |test path| replace:: :file:`tests/tfm/tfm_regression_test/`

.. include:: /includes/build_and_run_test.txt

Output
======

   .. code-block:: console

      Non-Secure system starting...

      #### Execute test suites for the Secure area ####

      [...]

      Running Test Suite PS reliability tests (TFM_PS_TEST_3XXX)...
      > Executing 'TFM_PS_TEST_3001'
      Description: 'repetitive sets and gets in/from an asset'
      > Iteration 15 of 15
      TEST: TFM_PS_TEST_3001 - PASSED!
      > Executing 'TFM_PS_TEST_3002'
      Description: 'repetitive sets, gets and removes'
      > Iteration 15 of 15
      TEST: TFM_PS_TEST_3002 - PASSED!
      TESTSUITE PASSED!

      [...]

      *** Secure test suites summary ***
      Test suite 'PSA protected storage S interface tests (TFM_PS_TEST_2XXX)' has  PASSED
      Test suite 'PS reliability tests (TFM_PS_TEST_3XXX)' has  PASSED
      Test suite 'PS rollback protection tests (TFM_PS_TEST_4XXX)' has  PASSED
      Test suite 'PSA internal trusted storage S interface tests (TFM_ITS_TEST_2XXX)' has  PASSED
      Test suite 'ITS reliability tests (TFM_ITS_TEST_3XXX)' has  PASSED
      Test suite 'Crypto secure interface tests (TFM_CRYPTO_TEST_5XXX)' has  PASSED
      Test suite 'Initial Attestation Service secure interface tests(TFM_ATTEST_TEST_1XXX)' has  PASSED
      Test suite 'Platform Service Secure interface tests(TFM_PLATFORM_TEST_1XXX)' has  PASSED
      Test suite 'IPC secure interface test (TFM_IPC_TEST_1XXX)' has  PASSED

      *** End of Secure test suites ***

      #### Execute test suites for the Non-secure area ####

      [...]

      Running Test Suite Core non-secure positive tests (TFM_CORE_TEST_1XXX)...
      > Executing 'TFM_CORE_TEST_1001'
      Description: 'Test service request from NS thread mode'
      TEST: TFM_CORE_TEST_1001 - PASSED!
      > Executing 'TFM_CORE_TEST_1003'
      Description: 'Test the success of service init'
      TEST: TFM_CORE_TEST_1003 - PASSED!
      > Executing 'TFM_CORE_TEST_1007'
      Description: 'Test secure service buffer accesses'
      TEST: TFM_CORE_TEST_1007 - PASSED!
      > Executing 'TFM_CORE_TEST_1008'
      Description: 'Test secure service to service call'
      TEST: TFM_CORE_TEST_1008 - PASSED!
      > Executing 'TFM_CORE_TEST_1010'
      Description: 'Test secure service to service call with buffer handling'
      TEST: TFM_CORE_TEST_1010 - PASSED!
      > Executing 'TFM_CORE_TEST_1015'
      Description: 'Test service parameter sanitization'
      TEST: TFM_CORE_TEST_1015 - PASSED!
      > Executing 'TFM_CORE_TEST_1016'
      Description: 'Test outvec write'
      TEST: TFM_CORE_TEST_1016 - PASSED!
      TESTSUITE PASSED!

      [...]

      *** Non-secure test suites summary ***
      Test suite 'PSA protected storage NS interface tests (TFM_PS_TEST_1XXX)' has  PASSED
      Test suite 'PSA internal trusted storage NS interface tests (TFM_ITS_TEST_1XXX)' has  PASSED
      Test suite 'Crypto non-secure interface test (TFM_CRYPTO_TEST_6XXX)' has  PASSED
      Test suite 'Platform Service Non-Secure interface tests(TFM_PLATFORM_TEST_2XXX)' has  PASSED
      Test suite 'Initial Attestation Service non-secure interface tests(TFM_ATTEST_TEST_2XXX)' has  PASSED
      Test suite 'QCBOR regression test(TFM_QCBOR_TEST_7XXX)' has  PASSED
      Test suite 'T_COSE regression test(TFM_T_COSE_TEST_8XXX)' has  PASSED
      Test suite 'Core non-secure positive tests (TFM_CORE_TEST_1XXX)' has  PASSED
      Test suite 'IPC non-secure interface test (TFM_IPC_TEST_1XXX)' has  PASSED

      *** End of Non-secure test suites ***
