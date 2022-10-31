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
It is used to exchange keys required for establishing a safe communication between nodes.
It also involves node authentication, which verifies that both nodes that initiate communication trust each other.

The Matter protocol uses elliptic curve cryptography as the principal mean of both public and private key protection and for providing digital signatures.
The elliptic curve cryptography is based on the NIST P-256 curve (secp256r1).

The following session establishment methods are available:

* Password-Authenticated Session Establishment (PASE)
* Certificate-Authenticated Session Establishment (CASE)

Password-Authenticated Session Establishment (PASE)
===================================================

When using PASE, both nodes share the same secret.
This process takes place when commissioning the device.

PASE uses the `SPAKE2+`_ algorithm to ensure a safe exchange of keys over non-secure channel.
With the SPAKE2+ algorithm, only one of the communicating parties actively uses the password during the execution of the protocol.
This is a reinforced version of the Password Authenticated Key Exchange (PAKE) protocol, where both parties are involved in creating a shared key and both actively use the password.

Certificate-Authenticated Session Establishment (CASE)
======================================================

When using CASE, both nodes own operational certificates that chain back to the same root of trust.
This process takes place while establishing the secured communication between nodes that are already commissioned.

CASE uses the `SIGMA`_ algorithm to ensure a safe exchange of keys over non-secure channel.

Root of trust is a concept within Matter that is centered around a certification authority (CA), identified by Root Public Key (Root PK).
The CA is a device tasked with issuing and assigning Node Operational Credentials (NOCs).
NOCs are used to identify a node within a fabric and are signed by the Root Private Key.
NOCs are installed during the :ref:`ug_matter_network_topologies_commissioning` by the commissioner together with Trusted Root CA Certificates.

Message confidentiality
***********************

After exchanging the keys and establishing secure channel, the commonly available AES modes of operation are used to provide shared key cryptographic operations.
