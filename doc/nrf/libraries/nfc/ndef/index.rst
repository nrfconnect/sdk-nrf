.. _lib_nfc_ndef:

NFC Data Exchange Format (NDEF)
###############################

NFC communication uses NFC Data Exchange Format (NDEF) messages to exchange data.

See the :ref:`ug_nfc` user guide for more information about :ref:`ug_nfc_ndef`.

The |NCS| provides modules for generating messages and records to make it easy to generate such messages.
If you want to implement your own polling device, you can use the supplied parser libraries.

.. note::
   You need additional hardware for reading NFC tags.

The NFC NDEF libraries are used in the following samples:

* :ref:`record_text`
* :ref:`writable_ndef_msg`
* :ref:`nfc_tag_reader`
* :ref:`nfc_tnep_tag`
* :ref:`nfc_tnep_poller`
* :ref:`record_launch_app`

.. toctree::
   :maxdepth: 1
   :glob:
   :caption: Subpages:

   *
