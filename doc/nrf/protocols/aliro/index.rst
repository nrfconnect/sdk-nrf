.. _ug_aliro:

Aliro
######

Aliro is a standardized access protocol defined by the Connectivity Standards Alliance (CSA).
It specifies how access readers communicate with user devices like smartphones and wearables to unlock points of entry.
The protocol supports NFC, Bluetooth® Low Energy (Bluetooth LE), and Bluetooth LE with Ultra-Wideband (UWB), and does not dictate how the reader connects to the rest of the ecosystem.

The nRF Door Lock and Access Control Add-on, used together with the |NCS|, provides a complete reference for building Aliro- and Matter-compatible locks and access control readers.
It integrates UWB and NFC support out of the box, while platform abstraction APIs allow for integration of hardware from any vendor.
The reference can support Aliro alone, Matter alone, or both, depending on your product requirements.

.. note::
    The Bluetooth LE-only mode defined by the Aliro standard as an optional feature is not supported.

To learn more about the solution, see the `nRF Door Lock and Access Control Add-on`_ documentation.
For source code, refer to the `ncs-door-lock-and-access-control`_ repository.
