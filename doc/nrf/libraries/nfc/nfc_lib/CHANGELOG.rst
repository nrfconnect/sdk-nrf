.. _nfc_changelog:

Changelog
#########

.. contents::
   :local:
   :depth: 2

All notable changes to this project are documented in this file.

nRF Connect SDK v2.3.0
**********************

Added
=====

* A callback requested by the :c:func:`nfc_platform_cb_request()` function to allow execution of platform-dependent code before executing the user callback.
* A header file :file:`nfc_platform.h` for platform layer.

Modified
========

* The :c:func:`nfc_platform_setup()` function now provides a pointer to the callback resolution function that is used to execute the user-defined NFC callback.
* The :c:func:`nfc_t2t_setup()` function was updated to allow coexistence of the Type 2 Tag and the Type 4 Tag libraries.

nRF Connect SDK v2.1.0
**********************

Added
=====

* Support for modifying the Frame Delay Min parameter in the :ref:`type_2_tag` and :ref:`type_4_tag` libraries.

nRF Connect SDK v2.0.0
**********************

Bug fixes
=========

* Fixed the Type 2 Tag initialization, where the return value of the :c:func:`nfc_platform_nfcid1_default_bytes_get()` function was not converted to the local NFC error code resulting in incorrect NFCID1 values.

nRF Connect SDK v1.9.0
**********************

Added
=====

* A dedicated header file :file:`nrf_nfc_errno.h` with error codes.
* A source file that translates nrfx error codes to NFC library error codes.
* Definitions for returned error codes in the API documentation.

Modified
========

* The library returns specific error codes from the :file:`nrf_nfc_errno.h` file instead of Zephyr error codes.
* Removed unit test dependencies from the header files :file:`nfc_t2t_lib.h` and :file:`nrf_t4t_lib.h`.

Bug fixes
=========

* Fixed the R(NAK) frame handling after sending the R(ACK) frame.
  The Tag now responds with the last transmitted R(ACK) frame.
* Fixed the race condition that occurred when the S(WTX) frame was scheduled and the library tried to send a data chunk.
* Fixed a possible memory overwrite in the :ref:`type_4_tag` library when the reader device sends an incorrect APDU Update command.

nRF Connect SDK v1.8.0
**********************

No changes in this release.

nRF Connect SDK v1.7.0
**********************

Modified
========

* Debug info is removed from the NFC T2T and T4T libraries.
* Fixed duplicated initial Frame Waiting Time (FWT) value setting.
* Modified the :c:func:`nfc_t2t_done()` and the :c:func:`nfc_t4t_done()` functions to uninitialize the NFCT driver to achieve symmetry in the library behaviour since the :c:func:`nfc_txt_setup()` function initializes the NFCT driver.
* Fixed FSDI value setting for RFU value cases.

nRF Connect SDK v1.6.0
**********************

No changes in this release.

nRF Connect SDK v1.5.0
**********************

No changes in this release.

nRF Connect SDK v1.4.0
**********************

Added
=====

* Added support for non-secure firmware image to the NFC T2T and NFC T4T libraries.
  A non-secure image can only be run on nRF devices that support Trusted Execution (for example, nRF5340).
* Added Frame Waiting Time (FWT) management logic to the NFC T4T library.
  FWT is changed now according to the ISO-DEP timing requirements (for example, WTX frame).

NFC 0.4.0
*********

Added Type 2 Tag and Type 4 Tag NFC libraries for the nRF5340 device.
Both libraries are compiled for the Cortex-M33 CPU in hard and soft float variants.

Added
=====

* Added the NFC T2T library for the nRF5340 device: ``libnfct2t_nrf53.a``.
* Added the NFC T4T library for the nRF5340 device: ``libnfct4t_nrf53.a``.
* Added a feature to the NFC T4T library that allows the user to change the NDEF
  buffer when the NFC stack is already initialized and running.

NFC 0.3.0
*********

Fixed minor bugs in the NFC libraries.

Modified
========

* Modified the NFC platform module (``nfc_platform_zephyr.c``) to use
  the Zephyr Clock Controller driver.
* Fixed the behavior of the Type 2 Tag library (``libnfct2t_nrf52.a``) when
  receiving an unsupported WRITE command.


NFC 0.2.0
*********

The NFC libraries now use the NFCT driver instead of the NFC HAL modules. The
NFCT driver is a part of nrfx repository.

Added
=====

* Added the NFC platform module to abstract runtime environment specific
  implementation:

  * ``nfc_platform_zephyr.c``

Removed
=======

* Removed the NFC HAL modules (replaced by the NFCT driver):

  * ``hal_nfc_t2t.c``
  * ``hal_nfc_t4t.c``


NFC 0.1.0
*********

Initial release.

Added
=====

* Added the following NFC libraries in both soft-float and hard-float builds:

  * ``libnfct2t_nrf52.a``
  * ``libnfct4t_nrf52.a``
