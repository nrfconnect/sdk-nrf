DECT NR+ RPC Unit Tests
=======================

Ztests on native_sim for the DECT NR+ RPC library in nrf/subsys/net/dect/rpc/. Three test
apps (client/, client_hooks/, server/); client and server never run in the same process.
client/prj.conf and server/prj.conf follow samples/dect/dect_rpc (mock UART and dummy shell).

Architecture
============

  +------------------+     +------------------+
  |  Client ztest    |     |  Server ztest     |
  |  shell / API     |     |  mock RX cmd      |
  |  mock RX evt     |     |  fixture L2/HAL   |
  +--------+---------+     +---------+---------+
           |                         |
           v                         v
  +--------+-----------------------------------+
  |  subsys/net/dect/rpc (client|server|common) |
  +--------+-----------------------------------+
           |
           v
  +--------+---------+
  | mock_dect_rpc_tr |  (common/mock_dect_rpc_transport.c; not UART nRF RPC)
  +------------------+

Tested: RPC library under subsys/net/dect/rpc/; server also real l2_dect + dect_mdm_mock.c.
Mocked: UART transport; client has no modem/L2; server has no libmodem MAC.
Suites/helpers: tests/.../rpc/common/, client/src/, server/src/ (test_rpc_env.h, expect/receive).

Twister (native_sim, four scenarios)
====================================

  net.dect.rpc.client              client/
  net.dect.rpc.client.auto_sync    client_hooks/     CONFIG_DECT_NR_RPC_AUTO_SYNC
  net.dect.rpc.server              server/           default prj.conf (shell on)
  net.dect.rpc.server.no_shell     server/           EXTRA_CONF_FILE=shell_disabled.conf

Both server scenarios share server/src/; default server prj.conf tracks samples/dect/dect_rpc
server.conf. With shell on, link-state suites use MOCK_DECT_RPC_EXPECT_ANY_SEND pads
(rpc_test_link_state_expect.h).


Building and running
====================

Twister
-------

  cd nrf/tests/subsys/net/dect/rpc

Run all DECT NR+ RPC ztests (four scenarios on native_sim):

  west twister -T . -p native_sim -O twister-out

Same set, explicit scenario names:

  west twister -T . -p native_sim \
    -s net.dect.rpc.client \
    -s net.dect.rpc.client.auto_sync \
    -s net.dect.rpc.server \
    -s net.dect.rpc.server.no_shell \
    -O twister-out-all

Individual scenarios:

  west twister -T . -p native_sim -s net.dect.rpc.client -O twister-out-client

  west twister -T . -p native_sim -s net.dect.rpc.client.auto_sync -O twister-out-hooks

  west twister -T . -p native_sim -s net.dect.rpc.server -O twister-out-server

  west twister -T . -p native_sim \
    -s net.dect.rpc.server.no_shell \
    -O twister-out-server-no-shell

Use platform native_sim (32-bit). native_sim/native/64 is not supported here (nRF RPC OS
assert on 64-bit atomics).

Direct build and run
--------------------

  cd nrf/tests/subsys/net/dect/rpc/client   # or server/ or client_hooks/
  west build -p -b native_sim .
  ./build/<client|server|client_hooks>/zephyr/zephyr.exe

Requirements: NCS, native_sim, CONFIG_NRF_RPC_RESPONSE_TIMEOUT set in prj.conf (avoid hang).

Measuring DECT NR+ RPC code coverage
====================================

  cd nrf/tests/subsys/net/dect/rpc

  west twister -T . -p native_sim \
    -C --coverage-tool lcov --coverage-formats html,lcov \
    --gcov-tool gcov --coverage-basedir "$(west topdir)" \
    -O twister-out-cov

  lcov --extract twister-out-cov/coverage.info "*/nrf/subsys/net/dect/rpc/*" \
    --output-file twister-out-cov/dect_rpc_only.info --rc lcov_branch_coverage=1

  genhtml twister-out-cov/dect_rpc_only.info \
    --output-directory twister-out-cov/dect_rpc_only_html \
    --branch-coverage --legend --prefix "$(west topdir)"

Report: twister-out-cov/dect_rpc_only_html/index.html

Adding a new test case
======================

1. Add *_suite.c under client/src or server/src (CMake GLOB). AUTO_SYNC-only hooks go in
   client_hooks/src/ and must be listed in client_hooks/CMakeLists.txt.
2. Use common/test_rpc_env.h; peer packets with mock_dect_rpc_tr_receive(), sends with
   mock_dect_rpc_tr_expect_add() (see link_state_suite.c: no helpers/loops — RPC_CMD/RPC_EVT
   compound literals).
3. Client: expect RPC_EVT_ACK after each handled RPC_EVT (events_suite.c).
4. Server: shared server_dect_fixture.c (one dect0 per binary); tear down associations if
   needed (if_send_suite.c).
5. Shell tests: fixed SHELL_LINE lines (shell_l2_suite.c) or build server.no_shell.

See client/src/, server/src/, and suite comments for individual tests.
