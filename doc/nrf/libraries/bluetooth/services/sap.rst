.. _bt_sap_readme:

Secure Application Pairing (SAP)
################################

.. contents::
   :local:
   :depth: 2

The Secure Application Pairing library authenticates application peers using provisioned P-256 device credentials and a certificate authority public key.
After authentication, it derives an application AES-GCM transport key to protect application traffic.

The library owns SAP contexts, sessions, authentication state, and key material.
Applications provide a :c:struct:`bt_sap_cb` callback table for SAP authentication transport, secure frames, payload delivery, and session state events.

Design and threat model
***********************

SAP is an application authorization layer for products that need a peer identity separate from the Bluetooth® bond database.
It is intended for devices that are provisioned with SAP credentials during manufacturing and that must authorize a peer by certificate authority, device identifier, group identifier, and allowed SAP role before exposing application functionality.

SAP does not replace Bluetooth SMP.
It adds application-owned credentials and policy checks on top of the Bluetooth transport.
This lets an application reject a bonded but unauthorized peer or accept only peers from a specific product group without relying on the local bond database as the only authorization source.

The library assumes:

* Device private keys and the CA public key are provisioned by the product.
* PSA Crypto provides the requested P-256, ECDSA-SHA256, ECDH, HKDF-SHA256, and AES-GCM operations.
* Applications pass received SAP frames to the matching session and do not reuse a session after a disconnect event.
* Each traffic direction uses a unique AES-GCM nonce base and the library-owned counter for that session.

SAP authenticates the compact peer certificate with the configured CA public key and signs a transcript that includes both roles, both nonces, both certificate bodies, and both ECDH public keys.
After authentication, secure application traffic is protected by the derived AES-GCM key.
If a session enters the failed state, applications must reconnect or allocate a new session before sending more protected application payloads.

Credential handling
*******************

.. caution::
   Do not use sample credentials in production applications.
   Provision production devices with product-specific private keys and certificates.

The sample accepts raw credential files at build time for demonstration and automated test coverage, but product designs should import or provision private keys through the platform's secure credential storage mechanism, such as PSA persistent keys or hardware-backed storage when available on the target.

Threading
*********

SAP public APIs serialize access to context and session state internally.
Session state events are delivered synchronously through the :c:struct:`bt_sap_cb` callback table in the API call or transport receive path that produced the event.
Event and payload callbacks receive an immutable :c:struct:`bt_sap_event` snapshot with the session identifier, peer identifiers, and the session ``user_data`` pointer captured before the callback was queued.
Callback implementations must return quickly, must not store event or data pointers beyond the callback lifetime, and should offload long-running application work.

Transport MTU
*************

SAP does not fragment authentication or secure frames.
A GATT transport must send each serialized SAP frame in one characteristic value.
Before starting SAP, applications must negotiate an ATT MTU that is at least the value of the :c:macro:`SAP_REQUIRED_ATT_MTU` macro.
If the negotiated MTU cannot carry :c:macro:`SAP_MAX_FRAME_LEN`, SAP send and receive APIs fail with ``-EMSGSIZE`` and the session does not continue.

Dependencies
************

The library depends on the following components:

* The :ref:`Zephyr Bluetooth Host <zephyr:bluetooth_api>` with connection and Security Manager Protocol support.
* The :ref:`PSA Crypto API <ug_psa_certified_api_overview_crypto>` with support for P-256, ECDSA-SHA256, ECDH, HKDF-SHA256, HMAC-SHA256, AES, and AES-GCM.

Configuration
*************

Enable the library with the :kconfig:option:`CONFIG_BT_SAP` Kconfig option.

The main configuration options are the following:

* :kconfig:option:`CONFIG_BT_SAP_MAX_CONTEXTS` - Maximum initialized SAP contexts
* :kconfig:option:`CONFIG_BT_SAP_MAX_PEERS` - Maximum peer sessions per context
* :kconfig:option:`CONFIG_BT_SAP_MAX_APP_PAYLOAD_SIZE` - Maximum protected payload size
* :kconfig:option:`CONFIG_BT_SAP_UNSAFE_PROTOCOL_TRACE` - Test-only hexdumps of SAP frames

Events
******

The library invokes the :c:struct:`bt_sap_cb` callbacks in the following scenarios:

* Authentication succeeds.
* Authentication fails.
* A protected payload is received.
* The session disconnects.

These callbacks are the SAP service event API and are available whenever :kconfig:option:`CONFIG_BT_SAP` is enabled.

API documentation
*****************

| Header file: :file:`include/bluetooth/services/sap.h`
| Source files: :file:`subsys/bluetooth/services/sap/`

.. doxygengroup:: bt_sap
.. doxygengroup:: bt_sap_protocol
