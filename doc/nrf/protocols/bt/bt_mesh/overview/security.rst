.. _ug_bt_mesh_overview_security:

Bluetooth Mesh network security
###############################

.. contents::
   :local:
   :depth: 2

Devices in a mesh network are often moving around, and dropping in and out of the network as needed.
With this dynamic change of network topology and its distributed nature over a shared medium, security awareness is important.
Bluetooth Mesh employs several security measures to prevent third-party interference and monitoring:

* Authentication
* Message encryption using security keys
* Use of privacy key protecting message metadata/public header fields
* Replay protection

.. _ug_bt_mesh_overview_security_authentication:

Authentication
**************

Device authentication is part of the provisioning process and lets the user confirm that the device being added to the network is indeed the device they think it is.

The Bluetooth Mesh protocol specification defines a range of out-of-band authentication methods, such as:

* Blinking of lights
* Output and input of passphrases
* Static authentication against a pre-shared key

To secure the provisioning procedure, Elliptic Curve Diffie-Helman (ECDH) public key cryptography is used.
After a device has been provisioned, it is part of the network and all its messages are considered authenticated.

.. _ug_bt_mesh_overview_security_encryption:

Message encryption
******************

Bluetooth Mesh features two levels of AES-CCM encryption with 128-bit keys for all messages going across the network:

Network encryption
	The lowest layer that protects all messages in a mesh network from being readable by devices that are not part of the network.

	The encryption is done with a network encryption key (NetKey), and any network may consist of up to 4096 different subnets, each with their own network key.
	All devices sharing a network key are considered part of the network, and may send and relay messages across it.
	By using multiple network keys, a network administrator may effectively divide their network into multiple subnets, because a mesh relay only forwards messages that are encrypted with a known network key.

Transport encryption
	The second encryption layer that limits which devices can do what *within a network* by encrypting the application payload with an application key (AppKey) or device key (DevKey).

	As an example, consider a mesh network deployed in a hotel.
	In this example it is desirable to limit some features that are to be controlled by the staff (like configuration of key cards or access to storage areas), and some features to be available to guests (like controlling room lighting or air conditioning).
	For this, we can have one application key for the guests and one for the staff, allowing the messages to be relayed across the same network, while preventing the guests and the staff from reading each other's messages.

While application keys are used to separate access rights to different applications in the network, the device keys are used to manage devices in the network.

Every device has a unique device key, which is only known to the provisioner and the device itself.
The device key is used when configuring a device with new encryption keys (network or application keys) or addresses, in addition to setting other device-specific parameters.

The device key can also be used to evict malicious devices from a network by transferring new keys to all the other devices in the network (using their individual device keys when transferring the keys).
This process is called the *Key Refresh Procedure*.
Refreshing keys in this way is a good protection against trash-can attacks from discarded devices.

Bluetooth Mesh networking ensures that devices can be disposed of securely through a procedure for removing a node from a network.
For more information, see :ref:`ug_bt_mesh_node_removal`.

Each encryption layer contains a message integrity check value that validates that the content of the message was encrypted with the indicated encryption keys.

.. _ug_bt_mesh_overview_security_privacy:

Privacy key
***********

All mesh message payloads are fully encrypted.
Message metadata, like source address and message sequence number, is obfuscated with the privacy key derived from the network key.
It provides limited privacy even for public header fields.

Obfuscation with the privacy key ensures that casual, passive eavesdropping cannot be used to track nodes and the people using them.
It also makes attacks based upon traffic analysis difficult.

.. _ug_bt_mesh_overview_security_replay_protection:

Replay protection
*****************

To guard against malicious devices replaying previous messages, every device keeps a running sequence number used for outbound messages.
Each mesh message is sent with a unique pair of sequence number and source address.
When receiving a message, the receiving device stores the sequence number and makes sure that it is more recent than the last sequence number it received from the same source address.
