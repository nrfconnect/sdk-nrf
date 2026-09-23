.. _bt_sap_readme:

Secure Application Pairing (SAP)
################################

.. contents::
   :local:
   :depth: 2

The Secure Application Pairing (SAP) library authenticates Bluetooth® Low Energy (LE) application peers by using provisioned P-256 device credentials and a certificate authority (CA) public key.
After authentication, it derives an application transport key that protects application traffic with Advanced Encryption Standard in Galois/Counter Mode (AES-GCM).

The library owns SAP contexts, sessions, authentication state, and key material.
Applications provide a :c:struct:`bt_sap_cb` callback table for SAP authentication transport, secure frames, payload delivery, and session state events.

Design and threat model
***********************

SAP is an application authorization layer for products that need a peer identity separate from the Bluetooth bond database.
It is intended for devices that are provisioned with SAP credentials during manufacturing.
Before exposing application functionality, these devices must authorize a peer by CA, device identifier, group identifier, and allowed SAP role.

SAP does not replace Bluetooth SMP.
It adds application-owned credentials and policy checks on top of the Bluetooth transport.
With these checks, an application can reject a bonded but unauthorized peer or accept only peers from a specific product group.
The application does not have to rely on the local bond database as the only authorization source.

The library relies on the following assumptions:

* The product provisions the device private keys and the CA public key.
* PSA Crypto provides the following requested operations:

  * P-256
  * ECDSA-SHA256
  * ECDH
  * HKDF-SHA256
  * AES-GCM

* Applications pass received SAP frames to the matching session and do not reuse a session after a disconnect event.
* Each traffic direction uses a unique AES-GCM nonce base and the library-owned counter for that session.

SAP authenticates the compact peer certificate with the configured CA public key.
It also signs a transcript that includes both roles, both nonces, both certificate bodies, and both ECDH public keys.
After authentication, the derived AES-GCM key protects secure application traffic.
If a session enters the failed state, applications must reconnect or allocate a new session before sending more protected application payloads.

Credential handling
*******************

.. caution::
   Do not use sample credentials in production applications.
   Provision production devices with product-specific private keys and certificates.

The sample accepts raw credential files at build time for demonstration and automated test coverage.
In product designs, import or provision private keys through the platform's secure credential storage mechanism, such as PSA persistent keys or hardware-backed storage when available on the target.

Threading
*********

SAP public APIs serialize access to context and session state internally.
The library delivers session state events synchronously through the :c:struct:`bt_sap_cb` callback table in the API call or transport receive path that produced the event.
Event and payload callbacks receive an immutable :c:struct:`bt_sap_event` snapshot with the session identifier, peer identifiers, and the session ``user_data`` pointer captured before the callback was queued.
Callback implementations must return quickly, must not store event or data pointers beyond the callback lifetime, and should offload long-running application work.

Frame size requirements
***********************

SAP does not fragment authentication or secure frames.
A Generic Attribute Profile (GATT) transport must send each serialized SAP frame in one characteristic value.
Before starting SAP, applications must negotiate an Attribute Protocol (ATT) maximum transmission unit (MTU) that is at least the value of the :c:macro:`SAP_REQUIRED_ATT_MTU` macro.
If the negotiated MTU cannot carry the value of the :c:macro:`SAP_MAX_FRAME_LEN` macro, SAP send and receive APIs fail with ``-EMSGSIZE`` and the session does not continue.

Dependencies
************

The library depends on the following components:

* The :ref:`Zephyr Bluetooth Host <zephyr:bluetooth_api>` with connection and Security Manager Protocol support.
* The :ref:`PSA Crypto API <ug_psa_certified_api_overview_crypto>` with support for the following cryptographic primitives:

  * P-256
  * ECDSA-SHA256
  * ECDH
  * HKDF-SHA256
  * HMAC-SHA256
  * AES
  * AES-GCM

Configuration
*************

Enable the library with the :kconfig:option:`CONFIG_BT_SAP` Kconfig option.

The main configuration options are the following:

* :kconfig:option:`CONFIG_BT_SAP_MAX_CONTEXTS` - Maximum initialized SAP contexts
* :kconfig:option:`CONFIG_BT_SAP_MAX_PEERS` - Maximum peer sessions per context
* :kconfig:option:`CONFIG_BT_SAP_MAX_APP_PAYLOAD_SIZE` - Maximum protected payload size
* :kconfig:option:`CONFIG_BT_SAP_UNSAFE_PROTOCOL_TRACE` - Test-only hexadecimal dumps of SAP frames

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
