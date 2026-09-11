.. _dect_tether_ipv6_ubuntu:

Ubuntu usage
#############

.. contents::
   :local:
   :depth: 2

Baseline firmware is usually enough on Ubuntu.

Reset the Ethernet interface
=============================

Run after reflashing the tether gateway or changing its Ethernet MAC. Replace ``eth0`` with your tether interface.

.. code-block:: bash

   sudo dhclient -6 -r eth0
   sudo dhclient -6 eth0

See the status
================

Replace ``eth0`` with your tether interface.

.. code-block:: bash

   ip -6 route show default
   ip -6 route show dev eth0
   ip -6 addr show dev eth0
   ip -6 neigh show dev eth0

Example default route (``proto ra`` = Router Advertisement):

.. code-block:: text

   default via fe80::f4ce:36ff:fe04:73b1 dev eth0 proto ra metric 1024 expires 1747sec

Live GW valid / GONE monitor
===============================

Same idea as the Windows PowerShell loop: one line per second, colored green for valid and red for GONE. Replace ``eth0`` with your tether interface. Stop with Ctrl+C.

.. code-block:: bash

   IF=eth0
   while true; do
     line=$(ip -6 route show default dev "$IF" 2>/dev/null | head -1)
     if [ -n "$line" ]; then
       via=$(echo "$line" | sed -n 's/.* via \([^ ]*\).*/\1/p')
       metric=$(echo "$line" | sed -n 's/.* metric \([0-9]*\).*/\1/p')
       expires=$(echo "$line" | sed -n 's/.* expires \([^ ]*\).*/\1/p')
       proto=$(echo "$line" | sed -n 's/.* proto \([^ ]*\).*/\1/p')
       printf '\033[32m%s  GW valid  expires=%s  via=%s  metric=%s  proto=%s\033[0m\n' \
         "$(date +%H:%M:%S)" "${expires:-n/a}" "$via" "${metric:-?}" "${proto:-?}"
     else
       printf '\033[31m%s  GW GONE\033[0m\n' "$(date +%H:%M:%S)"
     fi
     sleep 1
   done

* ``expires=…sec`` — kernel countdown until the RA route expires; should jump back when a new RA arrives (~60 s on baseline).
* ``GW GONE`` — rare on Ubuntu with baseline firmware; investigate if the tether gateway stopped sending RAs or the link is down.

Extra: capture Router Advertisements
=======================================

.. code-block:: bash

   sudo tcpdump -ni eth0 'icmp6 and ip6[40] == 134'
