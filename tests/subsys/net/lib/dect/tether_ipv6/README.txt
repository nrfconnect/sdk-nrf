DECT IPv6 Tether Integration Tests
==================================

Ztest integration tests for subsys/net/lib/dect/tether_ipv6/ on native_sim.
No hardware. Same mock libmodem as tests/drivers/dect/dect_mdm/integration.

Scope (what is real vs not)
===========================

Real (library under test + DECT hooks it uses):
- dect_tether_ipv6 (RA, DHCPv6 server, fwd, uplink)
- net_l2_dect + net_l2_dect_mgmt, dect_mdm driver
- net_mgmt() scan / associate / release (via test_dect_utils.c)
- NET_EVENT_DECT_ASSOCIATION_CHANGED -> uplink RA/DHCP/routing hooks

Not real / simulated:
- libmodem (mock_nrf_modem_dect_mac.c); bearer disabled (CONFIG_NET_L2_DECT_BR=n)
- Beacons, scan completion, GUA prefix: injected mock callbacks, not over-the-air
- Tethered host: fake eth (tether_test_eth) + synthetic RS/DHCP (tether_host_sim)

This is not a full modem-to-air DECT stack test; it validates the tether library
and its integration with L2/mgmt/uplink state on the host Ethernet leg.

Test cases
==========

  test_bootstrap_init              dect_tether_ipv6_init(), initial RA TX
  test_rs_to_ra                    RS -> RA (lifetime, M, RDNSS, PIO)
  test_dhcp_solicit_request_bind   Solicit/Request -> lease bound
  test_uplink_loss_ra_withdraw     parent release -> RA lifetime 0
  test_renew_address_change_revoke GUA prefix change -> Renew revoke IAADDR

Fixture: mock modem activate -> host eth up -> dect_tether_ipv6_init() (as in sample)
-> scan/associate -> mock GUA prefix (tether_net_fixture.c).

Building and running
====================

  cd nrf/tests/subsys/net/lib/dect/tether_ipv6
  west twister -T . -p native_sim -s net.dect.tether_ipv6.integration -O twister-out

Direct build:

  west build -p -b native_sim . -- -DEXTRA_CONF_FILE=tether_ipv6_test.conf
  timeout 60s ./build/tether_ipv6/zephyr/zephyr.exe

Configs: prj.conf (base), tether_ipv6_test.conf (Twister overlay, fast CI).

Shared with DECT integration tests: mock_nrf_modem_dect_mac.c, test_dect_utils.c

Measuring dect_tether_ipv6 library code coverage
=================================================

  cd nrf/tests/subsys/net/lib/dect/tether_ipv6

  west twister -T . -p native_sim \
    -s net.dect.tether_ipv6.integration \
    -C --coverage-tool lcov --coverage-formats html,lcov \
    --gcov-tool gcov --coverage-basedir "$(west topdir)" \
    -O twister-out-cov

  lcov --extract twister-out-cov/coverage.info \
    "*/nrf/subsys/net/lib/dect/tether_ipv6/*" \
    --output-file twister-out-cov/tether_coverage.info \
    --rc lcov_branch_coverage=1

  genhtml twister-out-cov/tether_coverage.info \
    --output-directory twister-out-cov/tether_coverage_html \
    --branch-coverage --legend --prefix "$(west topdir)"

Reports: twister-out-cov/coverage/index.html (full),
         twister-out-cov/tether_coverage_html/index.html (library only).
Requires lcov/gcov. CONFIG_COVERAGE in prj.conf stays off; Twister sets -C.

Adding tests
============

Add ZTEST() in src/main.c; extend tether_host_sim / tether_test_eth if needed.
RS inject = L2; DHCP inject = L3 (htons ports). Reset TX capture between steps.
