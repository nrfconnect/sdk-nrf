.. _app_boards:
.. _app_boards_names:
.. _programming_board_names:

Board support
#############

.. contents::
   :local:
   :depth: 2

The |NCS| provides board definitions for all Nordic Semiconductor devices and follows Zephyr's :ref:`zephyr:hw_support_hierarchy`.

The following tables list all boards and corresponding board targets for Nordic Semiconductor's hardware platforms.
You can select the board targets for these boards when :ref:`building`.

Board terminology in the |NCS|
******************************

The board targets follow Zephyr's :ref:`zephyr:board_terminology` scheme and are used mostly when :ref:`building`.
For example, the board target ``nrf54l15dk/nrf54l15/cpuapp`` can be read as made of the following elements:

+-------------+----------------+-------------------------+---------------------------------+--------------------------------------------------+
| nrf54l15dk  |                |        /nrf54l15        |             /cpuapp             |                                                  |
+=============+================+=========================+=================================+==================================================+
| Board name  | Board revision | Board qualifier for SoC | Board qualifier for CPU cluster | Board qualifier for variant (empty in this case) |
+-------------+----------------+-------------------------+---------------------------------+--------------------------------------------------+

While the board name is always present, other elements, such as the board revision or the board qualifiers, are optional:

* :ref:`Board revision <zephyr:glossary>` - You can use the *board_name@board_revision* board target to get extra devicetree overlays with new features available for a specific board version.
  The board version is printed on the label of your DK, just below the PCA number.
  For example, :ref:`building <building>` for the ``nRF9160dk@1.0.0/nrf9160`` board target adds the external flash on the nRF9160 DK that was available since :ref:`board version v0.14.0 <nrf9160_board_revisions>`.

* :term:`System on Chip (SoC)` - When you choose a board target with this qualifier, you build for this specific SoC on this board.
  For example, :ref:`building <building>` for the ``thingy53/nrf5340/cpuapp`` board target builds the application core firmware for the nRF5340 SoC on the Nordic Thingy:53 prototyping platform.

* :ref:`CPU cluster <zephyr:glossary>` - You can use this board qualifier to build for a group of one or more CPU cores, all executing the same image within the same address space and in a symmetrical (SMP) configuration.
  The CPU cluster board qualifier varies depending on the SoC and SoC Series.
  See the following examples:

  * :ref:`Building <building>` for the ``thingy53/nrf5340/cpunet`` board target builds the network core firmware for Nordic Thingy:53.
  * Building for ``nrf54h20dk/nrf54h20/cpuapp`` builds the application core firmware for the nRF54H20 DK.
  * Building for ``nrf54h20dk/nrf54h20/cpurad`` builds the radio core firmware for the nRF54H20 DK.

  Check the Product Specification of the given SoC for more information about the available CPU clusters.

* :ref:`Variant <zephyr:glossary>` - You can use this board qualifier to build for a particular type or configuration of a build for a combination of SoC and CPU cluster.
  In the |NCS|, board variants are used for the following purposes:

  * Changing the default memory map - This applies to the entry for the nRF52840 Dongle with the ``*/bare`` variant (``nrf52840dongle/nrf52840/bare``).
    When you select this target, the firmware does not account for the onboard USB bootloader.
    This corresponds to using :zephyr:board:`flashing option 3 with an external debug probe <nrf52840dongle>`.

  * Indicating the use of Cortex-M Security Extensions (CMSE), also known as security by separation:

    * Variants without ``*/ns`` (for example, ``cpuapp``) - When you select this target, you build the application core firmware as a single execution environment without CMSE.
      See :ref:`Trusted Firmware-M (TF-M) <ug_tfm>` for more information.

    * Variants with ``*/ns`` (for example, ``cpuapp/ns``) - Recommended for enhanced security.
      When you select this target, you build the application with CMSE enabled, using security by separation.

      The application core firmware is placed in Non-Secure Processing Environment (NSPE) and uses Secure Processing Environment (SPE) for security features.
      By default, the build system automatically includes :ref:`Trusted Firmware-M (TF-M) <ug_tfm>` in SPE and merges it with NSPE.

      Read more about separation of processing environments on the :ref:`ug_tfm_security_by_separation` page.

.. note::
    This board name scheme was introduced in the |NCS| before the v2.7.0 release following changes in Zephyr v3.6.0.
    Read :ref:`hwmv1_to_v2_migration`, Zephyr's :ref:`zephyr:hw_model_v2`, and refer to the `conversion example Pull Request`_ in Zephyr upstream if you have to port a board to the new model.

.. _app_boards_names_zephyr:

Boards included in sdk-zephyr
*****************************

The following boards are defined in the :file:`zephyr/boards/nordic/` folder.
Also see the :ref:`zephyr:boards` section in the Zephyr documentation.

.. note::
    |thingy52_not_supported_note|

.. _table:

+-------------------+------------+--------------------------------------------------------+--------------------------------------------------------------------------+
| Hardware platform | PCA number |                 Board name                             |                             Board targets                                |
+===================+============+========================================================+==========================================================================+
| nRF9161 DK        | PCA10153   | :zephyr:board:`nrf9161dk <nrf9161dk>`                  | ``nrf9161dk/nrf9161``                                                    |
|                   |            |                                                        |                                                                          |
|                   |            |                                                        | ``nrf9161dk/nrf9161/ns`` (:ref:`TF-M <app_boards_spe_nspe>`)             |
+-------------------+------------+--------------------------------------------------------+--------------------------------------------------------------------------+
| nRF9160 DK        | PCA10090   | :ref:`nrf9160dk <zephyr:nrf9160dk_nrf9160>`            | ``nrf9160dk/nrf9160``                                                    |
|                   |            |                                                        |                                                                          |
|                   |            |                                                        | ``nrf9160dk/nrf9160/ns`` (:ref:`TF-M <app_boards_spe_nspe>`)             |
|                   |            +--------------------------------------------------------+--------------------------------------------------------------------------+
|                   |            | :ref:`nrf9160dk <zephyr:nrf9160dk_nrf52840>`           | ``nrf9160dk/nrf52840``                                                   |
+-------------------+------------+--------------------------------------------------------+--------------------------------------------------------------------------+
| nRF9151 DK        | PCA10171   | :zephyr:board:`nrf9151dk <nrf9151dk>`                  | ``nrf9151dk/nrf9151``                                                    |
|                   |            |                                                        |                                                                          |
|                   |            |                                                        | ``nrf9151dk/nrf9151/ns`` (:ref:`TF-M <app_boards_spe_nspe>`)             |
+-------------------+------------+--------------------------------------------------------+--------------------------------------------------------------------------+
| nRF9131 EK        | PCA10165   | :zephyr:board:`nrf9131ek <nrf9131ek>`                  | ``nrf9131ek/nrf9131``                                                    |
|                   |            |                                                        |                                                                          |
|                   |            |                                                        | ``nrf9131ek/nrf9131/ns`` (:ref:`TF-M <app_boards_spe_nspe>`)             |
+-------------------+------------+--------------------------------------------------------+--------------------------------------------------------------------------+
| nRF54H20 DK       | PCA10175   | :zephyr:board:`nrf54h20dk <nrf54h20dk>`                | ``nrf54h20dk/nrf54h20/cpuapp``                                           |
|                   |            |                                                        |                                                                          |
|                   |            |                                                        | ``nrf54h20dk/nrf54h20/cpurad``                                           |
|                   |            |                                                        |                                                                          |
|                   |            |                                                        | ``nrf54h20dk/nrf54h20/cpuppr``                                           |
+-------------------+------------+--------------------------------------------------------+--------------------------------------------------------------------------+
| nRF54LV10 DK      | PCA10188   | ``nrf54lv10dk``                                        | ``nrf54lv10dk/nrf54lv10a/cpuapp``                                        |
|                   |            |                                                        |                                                                          |
|                   |            |                                                        | ``nrf54lv10dk/nrf54lv10a/cpuapp/ns`` (:ref:`TF-M <app_boards_spe_nspe>`) |
|                   |            |                                                        |                                                                          |
|                   |            |                                                        | ``nrf54lv10dk/nrf54lv10a/cpuflpr``                                       |
|                   |            |                                                        |                                                                          |
|                   |            |                                                        | ``nrf54lv10dk/nrf54lv10a/cpuflpr/xip``                                   |
+-------------------+------------+--------------------------------------------------------+--------------------------------------------------------------------------+
| nRF54LM20 DK      | PCA10184   | :zephyr:board:`nrf54lm20dk <nrf54lm20dk>`              | ``nrf54lm20dk/nrf54lm20a/cpuapp``                                        |
|                   |            |                                                        |                                                                          |
|                   |            |                                                        | ``nrf54lm20dk/nrf54lm20a/cpuapp/ns`` (:ref:`TF-M <app_boards_spe_nspe>`) |
|                   |            |                                                        |                                                                          |
|                   |            |                                                        | ``nrf54lm20dk/nrf54lm20a/cpuflpr``                                       |
|                   |            |                                                        |                                                                          |
|                   |            |                                                        | ``nrf54lm20dk/nrf54lm20a/cpuflpr/xip``                                   |
+-------------------+------------+--------------------------------------------------------+--------------------------------------------------------------------------+
| nRF54L15 DK       | PCA10156   | :zephyr:board:`nrf54l15dk <nrf54l15dk>`                | ``nrf54l15dk/nrf54l15/cpuapp``                                           |
|                   |            |                                                        |                                                                          |
|                   |            |                                                        | ``nrf54l15dk/nrf54l15/cpuapp/ns`` (:ref:`TF-M <app_boards_spe_nspe>`)    |
|                   |            |                                                        |                                                                          |
|                   |            |                                                        | ``nrf54l15dk/nrf54l15/cpuflpr``                                          |
|                   |            |                                                        |                                                                          |
|                   |            |                                                        | ``nrf54l15dk/nrf54l15/cpuflpr/xip``                                      |
+-------------------+------------+--------------------------------------------------------+--------------------------------------------------------------------------+
| nRF54L10 emulated | PCA10156   | :ref:`nrf54l15dk/nrf54l10 <zephyr:nrf54l15dk_nrf54l10>`| ``nrf54l15dk/nrf54l10/cpuapp``                                           |
| on the nRF54L15 DK|            |                                                        |                                                                          |
|                   |            |                                                        | ``nrf54l15dk/nrf54l10/cpuapp/ns`` (:ref:`TF-M <app_boards_spe_nspe>`)    |
+-------------------+------------+--------------------------------------------------------+--------------------------------------------------------------------------+
| nRF54L05 emulated | PCA10156   | :ref:`nrf54l15dk/nrf54l05 <zephyr:nrf54l15dk_nrf54l05>`| ``nrf54l15dk/nrf54l05/cpuapp``                                           |
| on the nRF54L15 DK|            |                                                        |                                                                          |
+-------------------+------------+--------------------------------------------------------+--------------------------------------------------------------------------+
| nRF5340 DK        | PCA10095   | :zephyr:board:`nrf5340dk <nrf5340dk>`                  | ``nrf5340dk/nrf5340/cpunet``                                             |
|                   |            |                                                        |                                                                          |
|                   |            |                                                        | ``nrf5340dk/nrf5340/cpuapp``                                             |
|                   |            |                                                        |                                                                          |
|                   |            |                                                        | ``nrf5340dk/nrf5340/cpuapp/ns`` (:ref:`TF-M <app_boards_spe_nspe>`)      |
+-------------------+------------+--------------------------------------------------------+--------------------------------------------------------------------------+
| nRF5340 Audio     | PCA10121   | :zephyr:board:`nrf5340_audio_dk <nrf5340_audio_dk>`    | ``nrf5340_audio_dk/nrf5340/cpuapp``                                      |
+-------------------+------------+--------------------------------------------------------+--------------------------------------------------------------------------+
| Thingy:53         | PCA20053   | :zephyr:board:`thingy53 <thingy53>`                    | ``thingy53/nrf5340/cpunet``                                              |
|                   |            |                                                        |                                                                          |
|                   |            |                                                        | ``thingy53/nrf5340/cpuapp``                                              |
|                   |            |                                                        |                                                                          |
|                   |            |                                                        | ``thingy53/nrf5340/cpuapp/ns`` (:ref:`TF-M <app_boards_spe_nspe>`)       |
+-------------------+------------+--------------------------------------------------------+--------------------------------------------------------------------------+
| nRF52840 DK       | PCA10056   | :zephyr:board:`nrf52840dk <nrf52840dk>`                | ``nrf52840dk/nrf52840``                                                  |
|                   |            +--------------------------------------------------------+--------------------------------------------------------------------------+
|                   |            | :ref:`nrf52840dk <zephyr:nrf52840dk_nrf52811>`         | ``nrf52840dk/nrf52811``                                                  |
+-------------------+------------+--------------------------------------------------------+--------------------------------------------------------------------------+
| nRF52840 Dongle   | PCA10059   | :zephyr:board:`nrf52840dongle <nrf52840dongle>`        | ``nrf52840dongle/nrf52840``                                              |
|                   |            |                                                        |                                                                          |
|                   |            |                                                        | ``nrf52840dongle/nrf52840/bare``                                         |
+-------------------+------------+--------------------------------------------------------+--------------------------------------------------------------------------+
| nRF52833 DK       | PCA10100   | :zephyr:board:`nrf52833dk <nrf52833dk>`                | ``nrf52833dk/nrf52833``                                                  |
|                   |            |                                                        |                                                                          |
|                   |            |                                                        | ``nrf52833dk/nrf52820``                                                  |
+-------------------+------------+--------------------------------------------------------+--------------------------------------------------------------------------+
| nRF52 DK          | PCA10040   | :zephyr:board:`nrf52dk <nrf52dk>`                      | ``nrf52dk/nrf52832``                                                     |
| (nRF53832)        |            +--------------------------------------------------------+--------------------------------------------------------------------------+
|                   |            | :ref:`nrf52dk <zephyr:nrf52dk_nrf52810>`               | ``nrf52dk/nrf52810``                                                     |
|                   |            +--------------------------------------------------------+--------------------------------------------------------------------------+
|                   |            | :ref:`nrf52dk <zephyr:nrf52dk_nrf52805>`               | ``nrf52dk/nrf52805``                                                     |
+-------------------+------------+--------------------------------------------------------+--------------------------------------------------------------------------+
| nRF21540 DK       | PCA10112   | :zephyr:board:`nrf21540dk <nrf21540dk>`                | ``nrf21540dk/nrf52840``                                                  |
+-------------------+------------+--------------------------------------------------------+--------------------------------------------------------------------------+
| nRF7002 DK        | PCA10143   | :zephyr:board:`nrf7002dk <nrf7002dk>`                  | ``nrf7002dk/nrf5340/cpunet``                                             |
|                   |            |                                                        |                                                                          |
|                   |            |                                                        | ``nrf7002dk/nrf5340/cpuapp``                                             |
|                   |            |                                                        |                                                                          |
|                   |            |                                                        | ``nrf7002dk/nrf5340/cpuapp/ns`` (:ref:`TF-M <app_boards_spe_nspe>`)      |
+-------------------+------------+--------------------------------------------------------+--------------------------------------------------------------------------+

.. note::
   In |NCS| releases before v1.6.1:

   * The board target ``nrf9160dk/nrf9160/ns`` was named ``nrf9160dk_nrf9160ns``.
   * The board target ``nrf5340dk/nrf5340/cpuapp/ns`` was named ``nrf5340dk_nrf5340_cpuappns``.

.. _app_boards_names_nrf:

Boards included in sdk-nrf
**************************

The following boards are defined in the :file:`nrf/boards/nordic/` folder.

+-------------------+------------+----------------------------------------------------------+---------------------------------------------------------------------------+
| Hardware platform | PCA number | Board name                                               | Board targets                                                             |
+===================+============+==========================================================+===========================================================================+
| nRF Desktop       | PCA20041   | :ref:`nrf52840gmouse <nrf_desktop>`                      | ``nrf52840gmouse/nrf52840``                                               |
| Gaming Mouse      |            |                                                          |                                                                           |
+-------------------+------------+----------------------------------------------------------+---------------------------------------------------------------------------+
| nRF Desktop       | PCA20044   | :ref:`nrf52dmouse <nrf_desktop>`                         | ``nrf52dmouse/nrf52832``                                                  |
| Mouse             |            |                                                          |                                                                           |
+-------------------+------------+----------------------------------------------------------+---------------------------------------------------------------------------+
| nRF Desktop       | PCA20037   | :ref:`nrf52kbd <nrf_desktop>`                            | ``nrf52kbd/nrf52832``                                                     |
| Keyboard          |            |                                                          |                                                                           |
+-------------------+------------+----------------------------------------------------------+---------------------------------------------------------------------------+
| nRF Desktop       | PCA10111   | :ref:`nrf52833dongle <nrf_desktop>`                      | ``nrf52833dongle/nrf52833``                                               |
| Dongle            |            |                                                          |                                                                           |
+-------------------+------------+----------------------------------------------------------+---------------------------------------------------------------------------+
| nRF Desktop       | PCA10114   | :ref:`nrf52820dongle <nrf_desktop>`                      | ``nrf52820dongle/nrf52820``                                               |
| Dongle            |            |                                                          |                                                                           |
+-------------------+------------+----------------------------------------------------------+---------------------------------------------------------------------------+
| Thingy:91         | PCA20035   | :ref:`thingy91 <ug_thingy91>`                            | ``thingy91/nrf9160``                                                      |
|                   |            |                                                          |                                                                           |
|                   |            |                                                          | ``thingy91/nrf9160/ns`` (:ref:`TF-M <app_boards_spe_nspe>`)               |
|                   |            +----------------------------------------------------------+---------------------------------------------------------------------------+
|                   |            | :ref:`thingy91 <ug_thingy91>`                            | ``thingy91/nrf52840``                                                     |
+-------------------+------------+----------------------------------------------------------+---------------------------------------------------------------------------+
| Thingy:91 X       | PCA20065   | :ref:`thingy91x <ug_thingy91x>`                          | ``thingy91x/nrf9151``                                                     |
|                   |            |                                                          |                                                                           |
|                   |            |                                                          | ``thingy91x/nrf9151/ns`` (:ref:`TF-M <app_boards_spe_nspe>`)              |
|                   |            +----------------------------------------------------------+---------------------------------------------------------------------------+
|                   |            | :ref:`thingy91x <ug_thingy91x>`                          | ``thingy91x/nrf5340/cpuapp``                                              |
|                   |            |                                                          |                                                                           |
|                   |            |                                                          | ``thingy91x/nrf5340/cpuapp/ns`` (:ref:`TF-M <app_boards_spe_nspe>`)       |
|                   |            |                                                          |                                                                           |
|                   |            |                                                          | ``thingy91x/nrf5340/cpunet``                                              |
+-------------------+------------+----------------------------------------------------------+---------------------------------------------------------------------------+

.. _shield_names_nrf:

Shields included in sdk-nrf
***************************

The following shields are defined in the :file:`nrf/boards/shields` folder.

+-----------------------------------------------------+------------+---------------------------------------------+------------------------------------------------------------------------------+
| Hardware platform                                   | PCA number | Board name                                  | Board targets                                                                |
+=====================================================+============+=============================================+==============================================================================+
| nRF7002 :term:`Evaluation Kit (EK)`                 | PCA63556   | :ref:`nrf7002ek <ug_nrf7002ek_gs>`          | ``nrf7002ek``                                                                |
+-----------------------------------------------------+------------+---------------------------------------------+------------------------------------------------------------------------------+
| nRF7002 EK with emulated support for the nRF7001 IC | PCA63556   | :ref:`nrf7002ek_nrf7001 <ug_nrf7002ek_gs>`  | ``nrf7002ek_nrf7001``                                                        |
+-----------------------------------------------------+------------+---------------------------------------------+------------------------------------------------------------------------------+
| nRF7002 EK with emulated support for the nRF7000 IC | PCA63556   | :ref:`nrf7002ek_nrf7000 <ug_nrf7002ek_gs>`  | ``nrf7002ek_nrf7000``                                                        |
+-----------------------------------------------------+------------+---------------------------------------------+------------------------------------------------------------------------------+
| nRF7002 :term:`Expansion Board (EB)` (Deprecated)   | PCA63561   | :ref:`nrf7002eb <ug_nrf7002eb_gs>`          | ``nrf7002eb``, ``nrf7002eb_interposer_p1`` (nRF54 Series)                    |
+-----------------------------------------------------+------------+---------------------------------------------+------------------------------------------------------------------------------+
| nRF7002-EB II                                       | PCA63571   | :ref:`nrf7002eb2 <ug_nrf7002eb2_gs>`        | ``nrf7002eb2`` (nRF54 Series, supersedes ``nrf7002eb`` for nRF54 Series DKs) |
+-----------------------------------------------------+------------+---------------------------------------------+------------------------------------------------------------------------------+
| nRF21540 EK                                         | PCA63550   | :ref:`nrf21540ek <ug_radio_fem_nrf21540ek>` | ``nrf21540ek``                                                               |
+-----------------------------------------------------+------------+---------------------------------------------+------------------------------------------------------------------------------+
