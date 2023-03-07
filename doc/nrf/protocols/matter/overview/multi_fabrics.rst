.. _ug_matter_overview_multi_fabrics:

Matter multiple fabrics feature
###############################

.. contents::
   :local:
   :depth: 2

A Matter device can join multiple fabrics and operate in them at the same time.
Because each Matter fabric has an administrator, this allows the device to be administered from multiple fabrics using multiple administrator devices, unrelated by any root of trust.

This Matter feature is key for achieving Matter's smart home interoperability.

This page contains conceptual information about the multiple fabrics feature in Matter.
For an example of how this feature works with multiple ecosystems, see :ref:`ug_matter_gs_ecosystem_compatibility_testing`.

Matter node in multiple fabrics
*******************************

A Matter fabric can be made of Matter devices (nodes) that belong to different IPv6 networks.
What brings them together in a fabric is the same :ref:`root of trust <ug_matter_network_topologies_concepts_security>`, configuration state, and a unique 64-bit Fabric ID.
These common elements are assigned during :ref:`ug_matter_network_topologies_commissioning` and are specific to a given Matter fabric.
The root of trust is the owner of Trusted Root CA Certificate to which each node's Node Operational Certificate chains back.

For a Matter device to join multiple fabrics, the following conditions must be met:

* The Matter node must support the Administrator Commissioning Cluster, which allows opening or closing the commissioning window on a Matter device.
* The administrator of the fabric in which the node is already present must open the commissioning window that allows the commissioner of another fabric to join the node to its fabric.

The Matter node receives and stores the identifiers of the new Matter fabric.

Multiple administrators of a node
*********************************

Each commissioner of a node is automatically granted administrator privileges on the node.
This implies that the commissioner is permitted to modify fabric and network configuration of the node, and configure the node's Access Control List to grant different level of privileges to other nodes on the same fabric.

Additionally, by the means of the multiple fabrics feature a single node may be administered by unrelated administrators that do not share a common root of trust.
Consequently, those administrators may be hosted by different ecosystem vendors.

.. note::
   Access Control Lists are based on the Node Operational Certificates.
   Using them alone already allows the commissioner to establish multiple administrators on a single fabric.
