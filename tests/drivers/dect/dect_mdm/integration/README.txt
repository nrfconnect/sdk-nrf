DECT NR+ Stack Integration Tests
================================

Unity-based integration tests for the DECT NR+ stack in NCS. Tests run from
net_mgmt() and conn_mgr() APIs down to dect_mdm driver for nRF91x.
The driver uses a mock libmodem backend.

Architecture - What Is Tested vs Mocked
=======================================

  +-------------------------------------------+
  |        Unity Test Framework               |
  |  +-------------------------------------+  |
  |  |   Real conn_mgr() calls             |  |  <- tests call
  |  |   (conn_mgr_if_connect, etc.)       |  |
  |  +--------------+----------------------+  |
  |  +--------------+----------------------+  |
  |  |   Real net_mgmt() calls             |  |  <- tests call
  |  |   (NET_REQUEST_DECT_*, etc.)        |  |
  |  +-------------+-----------------------+  |
  +----------------+--------------------------+
                   |
  +----------------+--------------------------+
  |                v                          |  <- Real
  |       DECT Connection Manager             |
  |                |                          |
  |       DECT Management API                 |
  |    (dect_net_l2_mgmt.h)                   |
  |                |                          |
  |       DECT L2 Networking                  |
  |   (subsys/net/l2_dect/)                   |
  |                |                          |
  |       DECT MAC Driver                     |
  |    (drivers/dect/dect_mdm/)               |
  |                |                          |
  +----------------+--------------------------+
                   |
  +----------------+--------------------------+
  |                v                          |  <- Mock
  |    Mock nrf_modem_dect_mac.h              |
  |  (src/mocks/mock_nrf_modem_dect_mac.c)    |
  +-------------------------------------------+

Tested:
- Full stack: conn_mgr, net_mgmt, DECT management API, L2 DECT,
  DECT Connection Manager, DECT MDM driver.

Mocked:
- nrf_modem_dect.h: all modem MAC calls are faked with callback simulation.

Building and running
====================

Twister
-------

  cd nrf/tests/drivers/dect/dect_mdm/integration
  west twister -T . -p native_sim -s unity.dect_integration_test -O twister-out

To inspect test runtime logs (including the TEST CASE RESULTS summary), find
handler.log in Twister output:

  cd nrf/tests/drivers/dect/dect_mdm/integration
  find twister-out -type f -name handler.log

Direct build and run
--------------------

  cd nrf/tests/drivers/dect/dect_mdm/integration
  west build -p -b native_sim . && timeout 15s ./build/integration/zephyr/zephyr.exe

Requirements:
- NCS 3.1+
- native_sim board (no hardware)
- Tests exit after completion

Measuring coverage
==================

Twister
-------

  cd nrf/tests/drivers/dect/dect_mdm/integration
  west twister -T . -p native_sim -s unity.dect_integration_test \
    -C --coverage-tool lcov --coverage-formats html,lcov \
    --coverage-basedir "$(west topdir)" -O twister-out-cov
  xdg-open twister-out-cov/coverage/index.html   # or open the path in a browser

This runs the test and generates reports directly via Twister. The HTML report is
written to twister-out-cov/coverage/index.html and LCOV data to
twister-out-cov/coverage.info.

Use -O for the full Twister output tree (build/run logs, including
handler.log). The -o option only changes report file locations.

Limit Twister coverage to DECT NR+ files
----------------------------------------

Twister generates full coverage by default. To limit coverage to DECT NR+ files,
post-filter coverage.info with lcov and regenerate HTML:

  cd nrf/tests/drivers/dect/dect_mdm/integration

  lcov --extract twister-out-cov/coverage.info \
    "*/nrf/drivers/dect/dect_mdm/*" \
    "*/nrf/subsys/net/l2_dect/*" \
    "*/nrf/subsys/net/lib/dect/utils/*" \
    --output-file twister-out-cov/dect_coverage.info \
    --rc lcov_branch_coverage=1

  # Optional cleanup pass:
  # remove any remaining test/generated files if they still appear.
  # This is a safety net after --extract, not required in all runs.
  # If nothing matches these patterns, lcov may return a non-zero exit code.
  lcov --remove twister-out-cov/dect_coverage.info \
    "*/tests/*" "*/generated/*" \
    --output-file twister-out-cov/dect_coverage.info \
    --rc lcov_branch_coverage=1

  genhtml twister-out-cov/dect_coverage.info \
    --output-directory twister-out-cov/dect_coverage_html \
    --branch-coverage --legend \
    --title "DECT NR+ Stack Coverage Report"

  xdg-open twister-out-cov/dect_coverage_html/index.html

Adding a new test case (including libmodem callback)
====================================================

Init flow (mock <-> driver):
- The driver calls mock functions.
- The mock invokes stored callbacks asynchronously.
- The driver's callback handlers update driver state (semaphores, events).
- The mock never touches driver internals.

  Driver Init            Mock Response    Driver Callback      Driver State
  -----------           -------------    ---------------      ------------
  systemmode_set()   ->  SUCCESS      ->   systemmode_cb    ->   sem_give()
  configure()        ->  SUCCESS      ->   configure_cb     ->   sem_give(), activation
  func_mode_set()    ->  SUCCESS      ->   func_mode_cb     ->   NET_EVENT_DECT_ACTIVATE_DONE

Principles for mocks:
- Use the real function signatures and callback parameter types from
  nrf_modem_dect.h (no redefinitions).
- Add a call counter (e.g. mock_nrf_modem_dect_*_call_count) for assertions.
- Simulate success by calling simulate_async_callback() with the appropriate
  op/ntf callback and cb_params; callbacks are run after a short timer delay
  (async).
- Do not access driver internals; the driver's own callbacks handle semaphores
  and state.

Steps to add a test:

1. Respect run order.
   Tests share state and run sequentially. Configure and activation failure
   tests must run before the successful stack initialization test; add new
   tests in a logical place (e.g. failure paths first, then success, then
   feature tests).

2. Add the test in src/test_dect_integration.c:

     void test_my_new_feature(void)
     {
         /* Use net_mgmt() / DECT API as in other tests */
         TEST_ASSERT_EQUAL(expected, actual);
     }

3. Register it in src/main.c:
   - Declare: extern void test_my_new_feature(void);
   - Add an entry to test_results[] with the next index.
   - Call: RUN_TEST_AND_TRACK(test_my_new_feature, <index>);

4. If the test needs a new libmodem operation/callback:
   - In src/mocks/mock_nrf_modem_dect_mac.c: implement the nrf_modem_dect_*
     function; increment the corresponding mock_*_call_count; when the
     operation should succeed, call
     simulate_async_callback((void (*)(void *))mock_op_callbacks.<callback>, &cb_params);
     (or mock_ntf_callbacks.<callback> for notification callbacks).
   - Use the same struct types for cb_params as in nrf_modem_dect.h.
   - If the callback parameter struct is larger than 64 bytes, add a size
     branch in simulate_async_callback() (see capability_ntf) so params are
     copied correctly.

No changes to the real driver are required; the mock is the boundary.
