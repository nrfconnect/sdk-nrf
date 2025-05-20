.. _ug_matter_overview_security:
.. _ug_matter_network_topologies_concepts_security:

Matter network security
#######################

.. contents::
   :local:
   :depth: 2

The Matter network security aims at authenticating only trustworthy devices to the Matter fabric and protecting the confidentiality of messages exchanged between the fabric nodes.

Session establishment
*********************

Session establishment is a process that serves two purposes.
It is used to exchange encryption keys required for establishing a secure communication between nodes.
It also involves mutual node authentication, which assures both nodes that they initiate communication with a trusted peer.

The Matter protocol uses elliptic curve cryptography as the principal means of key exchange schemes and for providing digital signatures.
The elliptic curve cryptography is based on the NIST P-256 curve (secp256r1).

The following session establishment methods are available:

* Passcode-Authenticated Session Establishment (PASE)
* Certificate-Authenticated Session Establishment (CASE)

Passcode-Authenticated Session Establishment (PASE)
===================================================

When using PASE, both nodes share the same secret in the form of 8-digit passcode.
The shared secret is used by the `SPAKE2+`_ algorithm to ensure a safe exchange of keys over non-secure channel.
This process takes place when commissioning the device.

On constrained devices, the computational cost of the SPAKE2+ algorithm can be reduced by using the passcode to calculate SPAKE2+ Verifier offline.
Thanks to the pre-calculated Verifier, a device does not need to use the passcode during the execution of the algorithm.

Certificate-Authenticated Session Establishment (CASE)
======================================================

When using CASE, both nodes own Node Operational Certificates that chain back to the same root of trust.
The NOCs are used by the `SIGMA`_ algorithm to ensure a mutual node authentication and a safe exchange of keys over non-secure channel.
This process takes place while establishing the secured communication between nodes that are already commissioned.

Root of trust is a concept within Matter that is centered around a certification authority (CA), identified by Root Public Key (Root PK).
The CA is a device tasked with issuing and assigning Node Operational Certificates (NOCs) or Intermediate Certificate Authority Certificates (ICACs).
NOCs are installed during the :ref:`ug_matter_network_topologies_commissioning` by the commissioner together with Trusted Root CA Certificates.

A node that is not synchronized to a reliable source of UTC time must fall back to using Last Known Good UTC Time when validating a peer node's certificate.
Using this fallback means that the node is unable to properly validate ``notBefore`` field of the certificate and this check must be skipped.

Message confidentiality
***********************

After exchanging the keys and establishing secure channel, the 128-bit AES-CCM algorithm is used to provide both confidentiality and integrity of exchanged messages.

A Matter message consists of the following elements:

* Message Header - Carries session and transport-related information.
* Protocol Header - Describes semantics of the message.
* Payload - Actual protocol-specific content of the message.

While the AES-CCM algorithm ensures the integrity of all three elements, only Protocol Header and Payload get encrypted.
This is because Message Header contains fields, such as Security Flags and Message Counter, which are used to calculate the AES-CCM Nonce that is necessary to decrypt the remaining part of a message.

Privacy Processing
==================

A Matter node may enable an optional privacy processing to additionally obfuscate the unencrypted Message Header in a transmitted message.
Privacy Processing uses the 128-bit AES-CTR mode to encrypt the obfuscated elements, such as Source Node ID, Destination Node ID, and Message Counter.
