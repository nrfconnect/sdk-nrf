.. _nfc_ndef_le_oob:

Bluetooth LE OOB records
########################

BLE pairing messages contain information that tell a polling device how to pair with the device that contains the NFC tag through Bluetooth Low Energy.
A BLE pairing message that works with Android devices contains one Bluetooth LE OOB record.
This record contains the BLE advertising data structure that is used to establish the connection.

The following BLE pairing methods can be used in combination with this module:

* Legacy and LE Secure Connections Just Works pairing
     The NDEF record contains basic advertising data such as device name, address, and role. In this case, pairing over NFC does not provide MITM protection - the only advantage is automation of pairing through NFC touch.
* LE Secure Connections pairing with OOB data
     The NDEF record contains two additional data types: Confirmation Value and Random Value that are used for OOB authentication. In this case, pairing MITM protection is considered as strong as the MITM protection of NFC.

For more information about NFC Connection Handover pairing methods, refer to the `Bluetooth Secure Simple Pairing Using NFC`_ and `Bluetooth Core Specification`_ Volume 3 Part H Chapter 2.

The Bluetooth LE OOB record is used in the :ref:`peripheral_hids_keyboard` sample.

API documentation
*****************

| Header file: :file:`include/nfc/ndef/le_oob_rec.h`
| Source file: :file:`subsys/nfc/ndef/le_oob_rec.c`

.. _nfc_ndef_le_oob_rec:

.. doxygengroup:: nfc_ndef_le_oob_rec
   :project: nrf
   :members:
