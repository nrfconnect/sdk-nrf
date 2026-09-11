.. _dect_tether_ipv6_windows:

Windows 11 usage
#################

.. contents::
   :local:
   :depth: 2

Find the tether adapter and gateway address
=============================================

Run in elevated PowerShell. Replace ``"Ethernet"`` with the alias from ``Get-NetAdapter`` if your tether NIC has another name.

.. code-block:: powershell

   Get-NetAdapter | Format-Table Name, ifIndex, Status, LinkSpeed, InterfaceDescription

The tether gateway's link-local address (``fe80::...``) is in the device log / ``net iface`` on the nRF, or from a Wireshark RA Source on Ethernet. Example used below: ``fe80::f4ce:36ff:fe04:73b1``.

Reset the Ethernet interface
=============================

Run after reflashing the tether gateway, changing its Ethernet MAC, or whenever the tether link looks stale. Elevated PowerShell or Command Prompt required.

.. code-block:: powershell

   ipconfig /release6
   ipconfig /renew6
   Restart-NetAdapter -Name "Ethernet" -Confirm:$false

See the status
===============

.. code-block:: powershell

   Get-NetRoute -InterfaceAlias "Ethernet" -AddressFamily IPv6 -DestinationPrefix "::/0" |
     Format-Table ifIndex, NextHop, RouteMetric, ValidLifetime
   Get-NetNeighbor -InterfaceAlias "Ethernet" -AddressFamily IPv6 |
     Format-Table IPAddress, LinkLayerAddress, State
   Get-NetIPAddress -InterfaceAlias "Ethernet" -AddressFamily IPv6 |
     Format-Table IPAddress, PrefixOrigin, SuffixOrigin

A working tether shows a ``::/0`` route with the tether gateway's ``fe80::`` address as ``NextHop``, that same address ``Reachable`` or ``Permanent`` as a neighbor, and an address with ``PrefixOrigin: Dhcp``.

Or use plain ``ipconfig /all`` (Command Prompt) and look at the tether adapter's block. Working tether -- ``Default Gateway`` filled in with the tether gateway's link-local address:

.. code-block:: text

   Ethernet adapter Ethernet:

      IPv6 Address. . . . . . . . . . . : 2001:db8:1:1::100(Preferred)
      Link-local IPv6 Address . . . . . : fe80::1234:5678:9abc:def0%17(Preferred)
      Default Gateway . . . . . . . . . : fe80::f4ce:36ff:fe04:73b1%17
      DHCPv6 IAID . . . . . . . . . . . : 123456789
      DHCPv6 Client DUID. . . . . . . . : 00-01-00-01-11-22-33-44-aa-bb-cc-dd-ee-ff
      DNS Servers . . . . . . . . . . . : 2001:4860:4860::8888

``GW GONE`` looks the same except ``Default Gateway`` is empty (addresses can still be present from a still-valid DHCPv6 lease):

.. code-block:: text

   Default Gateway . . . . . . . . . :

Live GW valid / GONE monitor
=============================

Paste into elevated PowerShell to watch the ``::/0`` route on the tether continuously instead of re-running the status check by hand. Stop with Ctrl+C.

* ``GW valid`` — route present, with its remaining lifetime and metric.
* ``GW GONE`` — no ``::/0`` on that interface. Expected on baseline firmware once Neighbor Unreachability Detection expires the router between Router Advertisements; see *Set a default route* below for a workaround that does not require changing firmware.

.. code-block:: powershell

   while ($true) {
       $r = Get-NetRoute -InterfaceAlias "Ethernet" -DestinationPrefix "::/0" -ErrorAction SilentlyContinue
       if ($r) {
           $rl = $r.ValidLifetime
           if ($rl.TotalDays -gt 3650) { $rlStr = "static" } else { $rlStr = $rl.ToString() }
           Write-Host "$(Get-Date -Format HH:mm:ss)  GW valid  RL=$rlStr  NextHop=$($r.NextHop)  Metric=$($r.RouteMetric)" -ForegroundColor Green
       } else {
           Write-Host "$(Get-Date -Format HH:mm:ss)  GW GONE" -ForegroundColor Red
       }
       Start-Sleep 1
   }

Set a default route
====================

Use when the monitor above shows ``GW GONE`` (for example while testing baseline firmware, where the route can expire between Router Advertisements). Replace the example address with your tether gateway's link-local address.

.. code-block:: powershell

   netsh interface ipv6 add route ::/0 "Ethernet" fe80::f4ce:36ff:fe04:73b1 store=persistent

Delete it
=========

.. code-block:: powershell

   netsh interface ipv6 delete route ::/0 "Ethernet" fe80::f4ce:36ff:fe04:73b1 store=persistent
   netsh interface ipv6 delete route ::/0 "Ethernet" fe80::f4ce:36ff:fe04:73b1 store=active

Extra: add a static neighbor entry
=====================================

If ping to the tether gateway fails with ``General failure`` or the neighbor stays ``Stale`` in the status check, add a static (non-expiring) neighbor cache entry mapping the tether gateway's IPv6 address directly to its MAC address, bypassing normal Neighbor Discovery for it (replace both addresses with your tether gateway's link-local address and Ethernet MAC):

.. code-block:: powershell

   netsh interface ipv6 add neighbors "Ethernet" fe80::f4ce:36ff:fe04:73b1 f6-ce-36-04-73-b1 store=persistent

Remove it the same way, with ``delete neighbors`` in place of ``add neighbors``.
