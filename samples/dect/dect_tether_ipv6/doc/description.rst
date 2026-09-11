.. _dect_tether_ipv6_description:

Sample description
###################

.. contents::
   :local:
   :depth: 2

This sample is always built as a PT (Portable Termination) device (``CONFIG_DECT_DEFAULT_DEV_TYPE_PT`` in :file:`prj.conf`).

The sample enables ``CONFIG_DECT_TETHER_IPV6_LIB`` for a tethered host on Ethernet:

* ICMPv6 Router Advertisements on ``eth0``: default router (``router_lifetime``), RDNSS (DNS), Managed (M) flag set (``CONFIG_DECT_TETHER_IPV6_RA_MANAGED=y``, required), and Prefix Information Options for ULA/GUA with L=0 and A=0 (prefix hint only—no SLAAC, not on-link).
* Minimal DHCPv6 server on UDP port 547: responds to Solicit, Request, Renew, and Information-request, offering ULA and GUA ``/128`` values read from ``dect0`` (same addresses the stack configured on the DECT interface).

The tether model is split: RA advertises this device as the IPv6 default gateway; M=1 tells the host to obtain those DECT /128 addresses using DHCPv6 IA_NA. Do not clear the Managed flag (``CONFIG_DECT_TETHER_IPV6_RA_MANAGED=n``): with PIO A=0 there is no SLAAC fallback, so hosts such as Windows typically end up with no addresses and no ``::/0``.

On the nRF9151 DK, console and shell use UART0 at 115200 baud (board default), like other Zephyr samples on that kit.

Requirements
************

The sample supports the following development kit:

.. table-from-sample-yaml::

For Ethernet builds, use an SPI Ethernet shield so ``eth0`` exists; merge the overlays described in :ref:`dect_tether_ipv6_building`.

.. include:: /includes/tfm.txt

Overview
********

Call ``dect_tether_ipv6_init()`` after ``nrf_modem_lib_init()`` (see :file:`src/main.c`).

The following abbreviations from the DECT NR+ MAC specification (`ETSI TS 103 636-4`_) are used where relevant:

* FT: Fixed Termination point
* PT: Portable Termination point

For the architecture diagram and system-level big picture, see :ref:`lib_dect_tethering`.

Tethered hosts (Windows and Ubuntu)
====================================

The same tether-gateway firmware behaves differently on Windows 11 and Ubuntu when using baseline overlays (``eth_common.conf`` only).

Ubuntu typically keeps an RA-learned ``::/0`` (``ip -6 route show default``) and refreshes the route expires timer when multicast RAs arrive (~every 60 s). DHCPv6 on the PC often completes in one round trip (Solicit → Reply when the client uses Rapid Commit).

Windows 11 may remove the RA-learned default route after NUD (~30 s with default ``REACHABLE_TIME``) even though the tether gateway still sends multicast RAs and DHCPv6 addresses may still work. For long sessions on Windows, add a persistent static ``::/0`` on the PC or append :file:`eth_max_connectivity.conf`.

Default route vs neighbor cache—these are independent on the host:

* ``::/0`` / ``GW valid``—from the RA router_lifetime (and Windows route policy).
* Neighbor ``fe80::…``—link-layer mapping; may show Reachable, Stale, or Permanent (if you pinned it with ``netsh``). A Reachable neighbor does not guarantee Windows will keep ``::/0``.

RA timing is controlled by Kconfig (:kconfig:option:`CONFIG_DECT_TETHER_IPV6_RA_ROUTER_LIFETIME` and related options in ``subsys/net/lib/dect/tether_ipv6/Kconfig``). See :ref:`dect_tether_ipv6_windows` and :ref:`dect_tether_ipv6_ubuntu` for OS-specific monitoring commands and troubleshooting.

.. _dect_tether_ipv6_building:

Building
********

.. |sample path| replace:: :file:`samples/dect/dect_tether_ipv6`

.. include:: /includes/build_and_run_ns.txt

See :ref:`cmake_options` for instructions on how to provide CMake options, for example to use a configuration overlay.

Configuration overlays (Ethernet)
==================================

Baseline Kconfig (no extra overlay) provides standard tethering: Managed RA, DHCPv6 /128 leases, 60 s multicast RAs, RS-triggered RAs, and Zephyr solicited NA to host NUD. Ubuntu tether tests usually need only baseline.

Optional files in this sample directory (append in this order after :file:`eth_common.conf` and shield-specific :file:`eth_*.conf`):

:file:`mdns-common.conf`
  Zephyr mDNS / DNS-SD stack (optional).

:file:`mdns.conf`
  Sample DNS-SD advertisement on ``dect0``.

:file:`eth_mdns.conf`
  mDNS-on-Ethernet tuning (scoped DNS, ``CONFIG_ZVFS_POLL_MAX``). After :file:`mdns-common.conf` and :file:`mdns.conf`.

:file:`mdns_forward.conf`
  eth0 ↔ dect0 mDNS tap. After :file:`eth_mdns.conf`.

:file:`eth_max_connectivity.conf`
  RFC-max REACHABLE_TIME (1 h), long router_lifetime, long DHCPv6 lifetimes. For long iperf / stability tests on Windows.

:file:`eth_ra_test_short_lifetime.conf`
  Lab helper: ``router_lifetime=120`` and 30 s periodic RAs.

:file:`eth_unsol_na.conf`
  Periodic unsolicited Neighbor Advertisements on ``eth0`` (optional ND-table aid).

:file:`dect_rx_pool.conf`
  Enables a DECT-private RX net_pkt / net_buf pool (see :ref:`dect_tether_ipv6_dect_rx_pool`) so eth0 RX bursts cannot starve dect0 RX.

:file:`dhcpv6-debug.conf`
  Logs each DHCPv6 datagram received on UDP 547 plus extra DHCPv6 debug output. Disable for production—Renew traffic can spam UART.

Both the Arceli and Seeed Studio W5500 shields are supported and equally recommended. For either one, the recommended default also appends :file:`dect_rx_pool.conf` and :file:`dlc_resilient.conf`, which keep the tether stable under sustained bidirectional load—use this combo unless you have a specific reason not to.

Recommended default—Arceli W5500 (from the sample directory):

.. code-block:: console

   cd nrf/samples/dect/dect_tether_ipv6
   west build -p -b nrf9151dk/nrf9151/ns -- -DSHIELD=arceli_eth_w5500 -DEXTRA_CONF_FILE="eth_common.conf;eth_w5500.conf;dect_rx_pool.conf;dlc_resilient.conf" -DEXTRA_DTC_OVERLAY_FILE=w5500-static-mac.overlay

Recommended default—Seeed Studio W5500 (from the sample directory):

.. code-block:: console

   cd nrf/samples/dect/dect_tether_ipv6
   west build -p -b nrf9151dk/nrf9151/ns -- -DSHIELD=seeed_w5500 -DEXTRA_CONF_FILE="eth_common.conf;eth_w5500.conf;eth_w5500_seeed.conf;dect_rx_pool.conf;dlc_resilient.conf" -DEXTRA_DTC_OVERLAY_FILE=w5500-seeed-static-mac.overlay

With mDNS responder on ``eth0`` and ``dect0`` added (Arceli W5500; append :file:`mdns-common.conf`, :file:`mdns.conf`, :file:`eth_mdns.conf` after the shield conf—see :ref:`dect_tether_ipv6_mdns` below for the full mDNS overlay chain):

.. code-block:: console

   cd nrf/samples/dect/dect_tether_ipv6
   west build -p -b nrf9151dk/nrf9151/ns -- -DSHIELD=arceli_eth_w5500 -DEXTRA_CONF_FILE="eth_common.conf;eth_w5500.conf;mdns-common.conf;mdns.conf;eth_mdns.conf" -DEXTRA_DTC_OVERLAY_FILE=w5500-static-mac.overlay

See :ref:`dect_tether_ipv6_arceli_w5500` and :ref:`dect_tether_ipv6_seeed_w5500` below for shield-specific notes (wiring, MAC address overlay, pin mapping).

.. _dect_tether_ipv6_arceli_w5500:

Ethernet with W5500 shield (Arceli)
====================================

Use this when the host leg should use Ethernet through the Zephyr :ref:`arceli_eth_w5500` shield on the nRF9151 DK.
Merge :file:`eth_common.conf` and :file:`eth_w5500.conf`; devicetree comes from :file:`w5500-static-mac.overlay` in this sample plus the Zephyr shield devicetree :file:`zephyr/boards/shields/arceli_eth_w5500/arceli_eth_w5500.overlay`.
For tethering, also append :file:`dect_rx_pool.conf` and :file:`dlc_resilient.conf` (recommended, see :ref:`dect_tether_ipv6_dect_rx_pool` and :ref:`dect_tether_ipv6_dlc_resilient`)—without the private DECT RX pool, sustained PC traffic can starve buffers and stall eth0 in poll mode.

.. note::
   Arduino D8 (reset) and D9 (interrupt) are shared with BUTTON1 and BUTTON2 on the nRF9151 DK.
   The Ethernet Kconfig overlays disable the DK library to avoid conflicts with the Arceli shield.

* From the sample directory (copy-paste each line):

  .. code-block:: console

     cd nrf/samples/dect/dect_tether_ipv6
     west build -p -b nrf9151dk/nrf9151/ns -- -DSHIELD=arceli_eth_w5500 -DEXTRA_CONF_FILE="eth_common.conf;eth_w5500.conf;dect_rx_pool.conf;dlc_resilient.conf" -DEXTRA_DTC_OVERLAY_FILE=w5500-static-mac.overlay

* Wiring as in the Arceli ETH W5500 shield overlay. Connect the RJ45 port to your LAN (router/switch) or directly to a PC as needed for your test.

Ethernet MAC: :file:`w5500-static-mac.overlay` sets a fixed locally administered ``local-mac-address``—edit that file so each board on the same LAN is unique.
For a random MAC each boot (``zephyr,random-mac-address``), use ``-DEXTRA_DTC_OVERLAY_FILE=w5500.overlay`` instead.

.. table:: nRF9151 DK + Arceli ETH W5500 (Arduino header).

   +-----------------------+---------------------------+
   | W5500 / shield signal | nRF9151 DK (Arduino/GPIO) |
   +=======================+===========================+
   | SCS                   | D10 (P0.10)               |
   +-----------------------+---------------------------+
   | MOSI                  | D11 (P0.11)               |
   +-----------------------+---------------------------+
   | MISO                  | D12 (P0.12)               |
   +-----------------------+---------------------------+
   | SCK/CLK               | D13 (P0.13)               |
   +-----------------------+---------------------------+
   | INT                   | D9 (P0.09)                |
   +-----------------------+---------------------------+
   | RESET                 | D8 (P0.08)                |
   +-----------------------+---------------------------+
   | 3.3V                  | Arduino 3.3V or DK VDD    |
   +-----------------------+---------------------------+
   | GND                   | GND                       |
   +-----------------------+---------------------------+

.. _dect_tether_ipv6_seeed_w5500:

Ethernet with W5500 shield (Seeed Studio board)
==================================================

Use this when the host leg should use Ethernet through the Zephyr :ref:`seeed_w5500` shield (Seeed Studio v1.1 shield was used) on the nRF9151 DK.
Merge :file:`eth_common.conf`, :file:`eth_w5500.conf`, and :file:`eth_w5500_seeed.conf`.
For tethering, also append :file:`dect_rx_pool.conf` and :file:`dlc_resilient.conf` (recommended, see :ref:`dect_tether_ipv6_dect_rx_pool` and :ref:`dect_tether_ipv6_dlc_resilient`)—without the private DECT RX pool, sustained PC traffic can starve buffers and stall eth0 in poll mode.
The Seeed shield (Rev 1.01) leaves the W5500 INTn disconnected, so the :file:`w5500-seeed*.overlay` files remove ``int-gpios`` for devicetree polling mode and :file:`eth_w5500_seeed.conf` sets a faster :kconfig:option:`CONFIG_ETH_W5500_POLL_PERIOD`.
Devicetree comes from the Zephyr shield devicetree :file:`zephyr/boards/shields/seeed_w5500/seeed_w5500.overlay`
plus a sample overlay: default :file:`w5500-seeed-static-mac.overlay` (fixed locally administered Ethernet MAC), or :file:`w5500-seeed.overlay` for ``zephyr,random-mac-address`` (new MAC each boot).

.. note::
   The sample :file:`w5500.overlay` is Arceli-specific (targets ``&eth_w5500_arceli_eth_w5500``).
   For ``seeed_w5500``, use :file:`w5500-seeed-static-mac.overlay` or :file:`w5500-seeed.overlay` (targets ``&eth_w5500``).

* From the sample directory (copy-paste each line):

  .. code-block:: console

     cd nrf/samples/dect/dect_tether_ipv6
     west build -p -b nrf9151dk/nrf9151/ns -- -DSHIELD=seeed_w5500 -DEXTRA_CONF_FILE="eth_common.conf;eth_w5500.conf;eth_w5500_seeed.conf;dect_rx_pool.conf;dlc_resilient.conf" -DEXTRA_DTC_OVERLAY_FILE=w5500-seeed-static-mac.overlay

  * Edit ``local-mac-address`` in :file:`w5500-seeed-static-mac.overlay` so each board on the same LAN has a unique MAC.

  * For a random Ethernet MAC each boot, use :file:`w5500-seeed.overlay` instead:

    .. code-block:: console

       west build -p -b nrf9151dk/nrf9151/ns -- -DSHIELD=seeed_w5500 -DEXTRA_CONF_FILE="eth_common.conf;eth_w5500.conf;eth_w5500_seeed.conf;dect_rx_pool.conf;dlc_resilient.conf" -DEXTRA_DTC_OVERLAY_FILE=w5500-seeed.overlay

.. _dect_tether_ipv6_dect_rx_pool:

DECT-private RX pool (:file:`dect_rx_pool.conf`)
====================================================

Under sustained bidirectional load (PC → DECT uplink + downstream return traffic), eth0 RX bursts allocate from the global Zephyr pools (``CONFIG_NET_PKT_RX_COUNT`` / ``CONFIG_NET_BUF_RX_COUNT``) and can starve dect0 RX, surfacing as ``RX packet allocation failed in ISR`` drops, followed by ``nrf_modem_dect_dlc_data_tx returned NRF_ENOMEM`` on the TX side and ``eth_w5500: TX semaphore timeout`` on the sink.

Append :file:`dect_rx_pool.conf` last in ``EXTRA_CONF_FILE`` to enable ``CONFIG_DECT_MDM_RX_PRIVATE_POOL``—a DECT-only ``net_pkt`` slab and ``net_buf`` pool isolated from the global RX pools:

.. code-block:: console

   cd nrf/samples/dect/dect_tether_ipv6
   west build -p -b nrf9151dk/nrf9151/ns -- -DSHIELD=seeed_w5500 -DEXTRA_CONF_FILE="eth_common.conf;eth_w5500.conf;eth_w5500_seeed.conf;dect_rx_pool.conf" -DEXTRA_DTC_OVERLAY_FILE=w5500-seeed-static-mac.overlay

Runtime inspection (with ``CONFIG_NET_BUF_POOL_USAGE=y`` and ``CONFIG_MEM_SLAB_TRACE_MAX_UTILIZATION=y`` already set by the overlay):

.. code-block:: console

   uart:~$ dect_mdm rx_pool
   DECT private RX pool:
   Address         Total   Free    MaxUsed Name
   0x...           35      35      8       dect_mdm_rx_pkts (slab)
   0x...           50      50      8       dect_mdm_rx_bufs (bufs, 128 B)

Bump ``PRIVATE_PKT_COUNT`` first if ``MaxUsed == Total`` under sustained traffic; raise ``PRIVATE_BUF_COUNT`` only if max-MTU downstream frames also become common (each consumes ``ceil(1500 / BUF_SIZE)`` fragments).

.. _dect_tether_ipv6_dlc_resilient:

Loss-resilient DLC profile (:file:`dlc_resilient.conf`)
==========================================================

Use :file:`dlc_resilient.conf` when the PT reconnects too eagerly under RF interference or link congestion: the modem's stock DLC defaults (60 s SDU lifetime, release on first discard) drop the association on a single discard-timer expiry. The overlay trades that for a more tolerant profile on the PT's uplink (see the file's comments for the exact values and rationale).

Append :file:`dlc_resilient.conf` last in ``EXTRA_CONF_FILE``, optionally combined with :file:`dect_rx_pool.conf`:

.. code-block:: console

   cd nrf/samples/dect/dect_tether_ipv6
   west build -p -b nrf9151dk/nrf9151/ns -- -DSHIELD=seeed_w5500 -DEXTRA_CONF_FILE="eth_common.conf;eth_w5500.conf;eth_w5500_seeed.conf;dect_rx_pool.conf;dlc_resilient.conf" -DEXTRA_DTC_OVERLAY_FILE=w5500-seeed-static-mac.overlay

Both knobs are also runtime-tunable using the DECT L2 shell (``dect sett --dlc_sdu_lifetime``, ``--dlc_discard_release_assoc_count``, ``--read``).

Valid ``--dlc_sdu_lifetime`` values are ``1..31`` (0.5 ms .. 60 s, see ``enum dect_dlc_sdu_lifetime`` / ``nrf_modem_dect_dlc_sdu_lifetime``) or ``255`` for ``INFINITY``. Use ``31`` (60 s) to revert to the default at runtime.

mDNS / DNS-SD
===============

To advertise ``_dect-nr._udp`` (same service name as ``dect_shell``), merge overlays in this order: :file:`mdns-common.conf`, :file:`mdns.conf`, and (for Ethernet) :file:`eth_mdns.conf`; add :file:`mdns_forward.conf` last for the eth0 ↔ dect0 tap.

* From the sample directory (copy-paste each line):

  DECT-only (append mDNS overlays):

  .. code-block:: console

     cd nrf/samples/dect/dect_tether_ipv6
     west build -p -b nrf9151dk/nrf9151/ns -- -DEXTRA_CONF_FILE="mdns-common.conf;mdns.conf"

  Arceli W5500—mDNS responder on ``eth0`` and ``dect0``:

  .. code-block:: console

     cd nrf/samples/dect/dect_tether_ipv6
     west build -p -b nrf9151dk/nrf9151/ns -- -DSHIELD=arceli_eth_w5500 -DEXTRA_CONF_FILE="eth_common.conf;eth_w5500.conf;mdns-common.conf;mdns.conf;eth_mdns.conf" -DEXTRA_DTC_OVERLAY_FILE=w5500-static-mac.overlay

  Arceli W5500—add :file:`mdns_forward.conf` for the eth0 ↔ dect0 mDNS tap in ``dect_tether_ipv6_lib`` (``CONFIG_DECT_TETHER_IPV6_MDNS_FORWARD``):

  .. code-block:: console

     cd nrf/samples/dect/dect_tether_ipv6
     west build -p -b nrf9151dk/nrf9151/ns -- -DSHIELD=arceli_eth_w5500 -DEXTRA_CONF_FILE="eth_common.conf;eth_w5500.conf;mdns-common.conf;mdns.conf;eth_mdns.conf;mdns_forward.conf" -DEXTRA_DTC_OVERLAY_FILE=w5500-static-mac.overlay

  Seeed W5500 uses the same mDNS conf chain; change ``SHIELD``, shield-specific conf, and ``EXTRA_DTC_OVERLAY_FILE`` as in the Ethernet sections above.

What this does:

* Zephyr mDNS responder listens on UDP 5353 on each IPv6-capable interface (``eth0`` when present, and ``dect0``).
* :file:`src/mdns_dns_sd_listen.c` binds UDP port ``CONFIG_DECT_TETHER_IPV6_SAMPLE_MDNS_DNS_SD_PORT`` (default 4700) on ``dect0`` so DNS-SD sees the advertised service port as in use, matching the ``dect_shell`` pattern.
* :file:`mdns_forward.conf` turns on ``CONFIG_DECT_TETHER_IPV6_MDNS_FORWARD`` (and ``CONFIG_NET_SOCKETS_PACKET``): an AF_PACKET tap in ``dect_tether_ipv6_lib`` (:file:`dect_tether_ipv6_mdns_fwd.c`) that forwards IPv6 mDNS between ``eth0`` and ``dect0`` without IID NAT. Queries from Ethernet are forwarded to DECT only when the IPv6 source is ULA or GUA; fe80:: sources are skipped. DECT toward Ethernet forwards multicast to ``ff02::fb`` and unicast mDNS whose destination is ULA or GUA (so unicast replies to a DHCPv6 host can return on the tether).

Hostname / instance label defaults to ``CONFIG_NET_HOSTNAME`` in :file:`mdns.conf` (``dect-tether-ipv6``). Change there if several devices share a LAN.

Dependencies
************

* :ref:`lib_dect_tethering`
* DECT NR+ :ref:`Connection Manager <zephyr:conn_mgr_overview>` and related APIs:

  * .. doxygengroup:: dect_net_l2_mgmt
  * .. doxygengroup:: dect_net_l2

This sample uses the following |NCS| libraries:

* :ref:`dk_buttons_and_leds_readme`

In addition, it uses the following secure firmware component:

* :ref:`Trusted Firmware-M <ug_tfm>`

.. _`ETSI TS 103 636-4`: https://www.etsi.org/deliver/etsi_ts/103600_103699/10363604/01.05.01_60/ts_10363604v010501p.pdf
