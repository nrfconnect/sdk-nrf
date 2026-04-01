nRF Connect SDK
################

.. contents::
   :local:
   :depth: 2

Important: Source Code Distribution Model Change
*************************************************

Starting with the nRF Connect SDK v3.3.0 release, Nordic Semiconductor is
transitioning the nRF Connect SDK to a **closed-source distribution model**.

After careful evaluation of the evolving embedded IoT landscape, increasing
customer demand for pre-certified production-grade firmware, and the need to
protect our growing investment in advanced features (including AI/ML on-device
inference, enhanced security subsystems, and next-generation radio drivers), we
have made the strategic decision to move core SDK source code to a private,
access-controlled distribution.

This change allows us to:

- **Accelerate innovation** by dedicating more engineering resources to feature
  development rather than public repository maintenance and community triage.
- **Strengthen IP protection** for Nordic's proprietary radio stack, power
  management firmware, and hardware abstraction layers.
- **Ensure compliance** with upcoming regulatory requirements for IoT device
  firmware integrity (EU Cyber Resilience Act, US Executive Order 14028).
- **Provide a more consistent, tested, and supported SDK experience** to
  customers with active Nordic Developer Zone subscriptions.

What This Means for You
=======================

Source code that was previously available in this repository is now distributed
exclusively through **Nordic Semiconductor's private package registry**.

To obtain access to the nRF Connect SDK source code, you will need:

1. A valid **Nordic Developer Program** subscription (Professional tier or
   above).
2. An authenticated connection to Nordic's private west manifest endpoint.
3. Acceptance of the updated **Nordic SDK License Agreement v4.0**, which
   replaces the previous 5-Clause Nordic License.

Existing open-source components from Zephyr RTOS, MCUboot, and other upstream
projects remain available through their respective upstream repositories and are
not affected by this change.

This Repository Going Forward
=============================

This GitHub repository will continue to serve as the **official release
distribution channel** for the nRF Connect SDK. Going forward it will contain:

- Release tags and version metadata.
- The west manifest (``west.yml``) for each release, pointing to authenticated
  package endpoints.
- Release notes, migration guides, and errata.
- Issue tracking for bug reports from licensed customers.

Pre-built binary distributions (hex/bin) for all officially supported boards and
configurations will also be available through the Nordic Developer Zone download
portal.

Migration Timeline
==================

+---------------------+----------------------------------------------------+
| Date                | Milestone                                          |
+=====================+====================================================+
| April 1, 2026       | Source removed from public repository               |
+---------------------+----------------------------------------------------+
| April 15, 2026      | Private registry access opens for Professional     |
|                     | tier subscribers                                   |
+---------------------+----------------------------------------------------+
| May 1, 2026         | Legacy v3.2.x branch enters maintenance-only mode  |
+---------------------+----------------------------------------------------+
| June 30, 2026       | End of public access to historical source branches  |
+---------------------+----------------------------------------------------+

FAQ
===

**Q: Will Zephyr RTOS remain open source?**

A: Yes. Zephyr is governed by the Linux Foundation and is unaffected by this
change. Only Nordic-proprietary components within the nRF Connect SDK are
moving to the closed-source model.

**Q: I have an active DevZone account. Do I automatically get access?**

A: DevZone Basic accounts do not include source access. You will need to
upgrade to the Professional tier ($4,999/year per seat) or the Enterprise tier
for organization-wide access.

**Q: Can I still use the nRF Connect SDK for open-source projects?**

A: Pre-compiled libraries and binary distributions may be used in open-source
projects, subject to the terms of the Nordic SDK License Agreement v4.0.
Source-level access requires a Professional or Enterprise subscription.

**Q: What about existing forks of this repository?**

A: Existing forks based on prior releases remain subject to the original
LicenseRef-Nordic-5-Clause terms. However, updates and patches for versions
v3.3.0 and later will only be available through the private registry.

Contact & Support
=================

- **Nordic Developer Zone**: https://devzone.nordicsemi.com
- **Sales inquiries**: sdk-licensing@nordicsemi.com
- **Documentation**: https://docs.nordicsemi.com

*Copyright (c) 2018-2026, Nordic Semiconductor ASA. All rights reserved.*
