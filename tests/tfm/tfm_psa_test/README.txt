TF-M: Platform security architecture test
#########################################

The TF-M platform security architecture test sample provides a basis for validating compliance with PSA Certified requirements using the Arm® Platform Security Architecture (PSA) test suites.

Requirements
************

The test supports the following development kits:

* nrf5340dk_nrf5340_cpuapp_ns
* nrf9151dk_nrf9151_ns
* nrf9160dk_nrf9160_ns
* nrf9161dk_nrf9161_ns

Overview
********

Run the PSA test suite tests with Zephyr and TF-M.

To choose a test suite, use the `CONFIG_TFM_PSA_TEST_*` Kconfig options.
Only one of these suites can be run at a time.

Configuration
*************

The following Kconfig options can be used to choose a test suite:

* `CONFIG_TFM_PSA_TEST_CRYPTO`
* `CONFIG_TFM_PSA_TEST_PROTECTED_STORAGE`
* `CONFIG_TFM_PSA_TEST_INTERNAL_TRUSTED_STORAGE`
* `CONFIG_TFM_PSA_TEST_STORAGE`
* `CONFIG_TFM_PSA_TEST_INITIAL_ATTESTATION`

Building and running
********************

You can indicate the desired test suite by using a configuration flag when building (replace *board_target* with your board target name, for example `nrf5340dk/nrf5340/cpuapp/ns`):

    west build -b *board_target* nrf/tests/tfm/tfm_psa_test -- -DCONFIG_TFM_PSA_TEST_STORAGE=y

When the test suite `CONFIG_TFM_PSA_TEST_INITIAL_ATTESTATION` is selected, it is required that the device is provisioned with the PSA root-of-trust security parameters using the provisioning image sample.
To provision the device, build and flash the provisioning image sample.
Then run the PSA test suite with the following command, where *board_target* is your board target name:

    west build -b *board_target* nrf/tests/tfm/tfm_psa_test -- -DCONFIG_TFM_PSA_TEST_INITIAL_ATTESTATION=y -DCONFIG_TFM_PARTITION_INITIAL_ATTESTATION=y -DCONFIG_TFM_NRF_PROVISIONING=y -DCONFIG_NRF_APPROTECT_LOCK=y -DCONFIG_NRF_SECURE_APPROTECT_LOCK=y


Not all test suites are valid on all boards.
See the `testcase.yaml` file for the list of valid test suites for each board.

**Note:**
   The following test cases are currently failing:
    - 216: Testing crypto generator functions APIs
    - 601: Testing attestation initial attestation APIs

Test files can be found under `/modules/tee/tf-m/psa-arch-tests` in the nRF Connect SDK folder structure.
They are checked out from https://github.com/nrfconnect/sdk-psa-arch-tests, a fork of the https://github.com/zephyrproject-rtos/psa-arch-tests repository, which in turn is a fork of the https://github.com/ARM-software/psa-arch-tests  source repository.

Output
======

The test has the following output:

      *** Booting Zephyr OS build zephyr-v2.5.0-456-g06f4da459a99  ***

      ***** PSA Architecture Test Suite - Version 1.0 *****

      Running.. Storage Suite
      ******************************************

      TEST: 401 | DESCRIPTION: UID not found check
      [Info] Executing tests from non-secure

      [Info] Executing ITS tests
      [Check 1] Call get API for UID 6 which is not set
      [Check 2] Call get_info API for UID 6 which is not set
      [Check 3] Call remove API for UID 6 which is not set
      [Check 4] Call get API for UID 6 which is removed
      [Check 5] Call get_info API for UID 6 which is removed
      [Check 6] Call remove API for UID 6 which is removed
      Set storage for UID 6
      [Check 7] Call get API for different UID 5
      [Check 8] Call get_info API for different UID 5
      [Check 9] Call remove API for different UID 5

      [Info] Executing PS tests
      [Check 1] Call get API for UID 6 which is not set
      [Check 2] Call get_info API for UID 6 which is not set
      [Check 3] Call remove API for UID 6 which is not set
      [Check 4] Call get API for UID 6 which is removed
      [Check 5] Call get_info API for UID 6 which is removed
      [Check 6] Call remove API for UID 6 which is removed
      Set storage for UID 6
      [Check 7] Call get API for different UID 5
      [Check 8] Call get_info API for different UID 5
      [Check 9] Call remove API for different UID 5

      TEST RESULT: PASSED

      ******************************************

      [...]

      TEST: 417 | DESCRIPTION: Storage asset capacity modification check
      [Info] Executing tests from non-secure

      [Info] Executing PS tests
      Test Case skipped as Optional PS APIs are not supported.

      TEST RESULT: SKIPPED (Skip Code=0x0000002B)

      ******************************************

      ************ Storage Suite Report **********
      TOTAL TESTS     : 17
      TOTAL PASSED    : 11
      TOTAL SIM ERROR : 0
      TOTAL FAILED    : 0
      TOTAL SKIPPED   : 6
      ******************************************

      Entering standby..
