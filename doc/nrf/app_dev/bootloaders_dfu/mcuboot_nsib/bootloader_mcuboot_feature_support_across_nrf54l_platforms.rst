.. _mcuboot_on_nrf54l_series:

MCUboot feature coverage across nRF54L platform series
######################################################

The following table summarizes MCUboot feature coverage across nRF54L SoC series.

Columns describe features as independent: a row in table does not imply that every combination is supported.

Column "Bootloader modes" lists modes additionally supported to swap-move, which is default for all platforms.
Crypto support is selected by build system scripts and is not configurable by user.

.. |yes|     replace:: ✓
.. |default| replace:: ✓ (default)
.. |exp|     replace:: ✓ (experimental)
.. |no|      replace:: –

.. list-table::
   :header-rows: 1
   :widths: 28 18 18 18 18 18

   * - Feature
     - nRF54L15, nRF54L10, nRF54L05
     - nRF54LM20A/B
     - nRF54LV10A
     - nRF54LC10A
     - nRF54LS05A/B

   * - **Signature algorithms**
     -
     -
     -
     -
     -
   * - Ed25519 Pure over Image SHA/Pure over image
     - |default|/|yes|
     - |default|/|yes|
     - |default|/|yes|
     - |default|/|yes|
     - |no|/|no|
   * - ECDSA P-256
     - |exp|
     - |exp|
     - |exp|
     - |exp|
     - |default|
   * - RSA
     - |exp|
     - |exp|
     - |exp|
     - |exp|
     - |exp|

   * - **Bootloader modes**
     -
     -
     -
     -
     -
   * - Direct-XIP
     - |yes|
     - |yes|
     - |yes|
     - |yes|
     - |yes|
   * - Direct-XIP with revert
     - |yes|
     - |yes|
     - |yes|
     - |yes|
     - |yes|
   * - Overwrite-only
     - |yes|
     - |yes|
     - |yes|
     - |yes|
     - |yes|
   * - Firmware loader (single slot)
     - |yes|
     - |yes|
     - |yes|
     - |yes|
     - |yes|
   * - USB
     - |no|
     - |yes|
     - |no|
     - |no|
     - |no|

   * - **On-startup boot mode request methods**
     -
     -
     -
     -
     -
   * - Retention
     - |yes|
     - |yes|
     - |yes|
     - |yes|
     - |no|
   * - Reset pin
     - |yes|
     - |yes|
     - |yes|
     - |yes|
     - |yes|
   * - GPIO trigger
     - |yes|
     - |yes|
     - |yes|
     - |yes|
     - |yes|

   * - **Other features**
     -
     -
     -
     -
     -
   * - KMU
     - |yes|/Ed25519/with key revocation
     - |yes|/Ed25519/with key revocation
     - |yes|/Ed25519/with key revocation
     - |yes|/Ed25519/with key revocation
     - |no|
   * - Compression
     - |yes|
     - |yes|
     - |yes|
     - |yes|
     - |yes|
   * - Encryption: ECIES-X25519/AES
     - |exp|
     - |exp|
     - |exp|
     - |exp|
     - |no|
   * - Encryption: ECIES-P256/AES
     - |no|
     - |no|
     - |no|
     - |no|
     - |exp|
