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
- Sink uplink (one of):
    mock_ppp_net_if.c   — PPP/cellular path (default prj.conf)
    mock_eth_net_if.c   — fake Ethernet device (eth_sink.conf)

Sink uplink variants
====================

Two Twister scenarios share the same Unity sources; only uplink mock + overlay differ.

  Scenario                             | Config          | Mock
  -------------------------------------|-----------------|---------------
  unity.dect_integration_test          | prj.conf (PPP)  | mock_ppp_net_if
  unity.dect_integration_test.eth_sink | eth_sink.conf   | mock_eth_net_if


Building and running
====================

Twister
-------

Default (PPP / cellular mock uplink):

  cd nrf/tests/drivers/dect/dect_mdm/integration
  west twister -T . -p native_sim -s unity.dect_integration_test -O twister-out

Ethernet sink variant:

  cd nrf/tests/drivers/dect/dect_mdm/integration
  west twister -T . -p native_sim -s unity.dect_integration_test.eth_sink \
    -O twister-out-eth

Run both:

  west twister -T . -p native_sim \
    -s unity.dect_integration_test \
    -s unity.dect_integration_test.eth_sink \
    -O twister-out-all

Handler logs (TEST CASE RESULTS summary), e.g. after the runs above:

  cd nrf/tests/drivers/dect/dect_mdm/integration
  find twister-out -type f -name handler.log
  find twister-out-eth -type f -name handler.log
  find twister-out-all -type f -name handler.log

Direct build and run
--------------------

Default build:

  cd nrf/tests/drivers/dect/dect_mdm/integration
  west build -p -b native_sim . && timeout 60s ./build/integration/zephyr/zephyr.exe

Ethernet sink build:

  cd nrf/tests/drivers/dect/dect_mdm/integration
  west build -p -b native_sim . -- \
    -DEXTRA_CONF_FILE=eth_sink.conf
  timeout 60s ./build/integration/zephyr/zephyr.exe

Requirements:
- NCS 3.1+
- native_sim board (no hardware)
- Tests exit after completion (eth_sink run is longer; allow ~60s timeout)

Measuring DECT NR+ stack code coverage
=====================================

  cd nrf/tests/drivers/dect/dect_mdm/integration

  west twister -T . -p native_sim \
    -s unity.dect_integration_test \
    -s unity.dect_integration_test.eth_sink \
    -C --coverage-tool lcov --coverage-formats html,lcov \
    --gcov-tool gcov --coverage-basedir "$(west topdir)" \
    -O twister-out-cov-all

  lcov --extract twister-out-cov-all/coverage.info \
    "*/nrf/drivers/dect/dect_mdm/*" \
    "*/nrf/subsys/net/l2_dect/*" \
    "*/nrf/subsys/net/lib/dect/utils/*" \
    --output-file twister-out-cov-all/dect_coverage.info \
    --rc lcov_branch_coverage=1

  genhtml twister-out-cov-all/dect_coverage.info \
    --output-directory twister-out-cov-all/dect_coverage_html \
    --branch-coverage --legend --prefix "$(west topdir)"

Reports: twister-out-cov-all/coverage/index.html (full),
         twister-out-cov-all/dect_coverage_html/index.html (DECT only).
Requires lcov/gcov. CONFIG_COVERAGE in prj.conf stays off; Twister sets -C.

Adding a new test case (including libmodem callback)
====================================================

Mocks use real nrf_modem_dect.h types; success via simulate_async_callback().
Do not touch driver internals.

1. Add test in test_dect_integration.c (or test_dect_eth_integration.c for
   eth_sink; guard CONFIG_NET_L2_ETHERNET && !CONFIG_MODEM_CELLULAR).
2. Register in main.c (extern, test_results[], RUN_TEST_AND_TRACK). Respect
   run order — tests share state.
3. New libmodem op: mock_nrf_modem_dect_mac.c + call counter +
   simulate_async_callback() (extend if params > 256 B).
4. Eth sink: test_eth_sink_bring_connected(), dect_test_mock_eth_* helpers.

No driver changes required.
