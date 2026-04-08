.. _ug_nrf54h20_mcuboot_requests:

Configuring bootloader requests on the nRF54H20 SoC
###################################################

.. contents::
   :local:
   :depth: 2

If you must pass information from the application to the bootloader to affect its behavior during the next boot, you can use a retained memory area that survives resets and is accessible to both the application and the bootloader after reboot.
If retained memory is not available, use the *bootloader requests* module.
This module provides a common API on the nRF54H20 SoC that abstracts the underlying storage.

Overview
********

The bootloader requests module provides APIs for the following:

* Confirm an image after a successful update.

  This functionality marks an updated image as valid from the bootloader context, allowing you to set the whole application partition as non-updatable before executing.
  In such case, the application simply requests the bootloader to confirm the image during the next boot instead of modifying the active image partition directly.
  The bootloader handles these requests before entering the main boot logic, ensuring that the image selection and update logic acts as if the image was confirmed by the application itself.
  The bootloader always erases the confirm requests after processing them, ensuring that the image is confirmed only once.
  The number of erase cycles on the NVM storage while processing confirm requests is comparable to the number of image updates, thus there is no need for special care when using this functionality.

* Request a specific slot preference for the next boot (primary or secondary).

  This functionality allows the application to influence the bootloader image selection logic during the next boot.
  By default, the bootloader prefers the slot with the highest version number.
  By using this request, the application can override this behavior and force the bootloader to select a specific slot for the next boot.
  This is especially useful in combination with Direct-XIP mode, where there are two copies of the application image present in the flash memory.
  This module uses the :kconfig:option:`CONFIG_FIND_NEXT_SLOT_HOOKS` option to hook into the slot selection logic.
  Depending on the value of :kconfig:option:`CONFIG_NRF_MCUBOOT_BOOT_REQUEST_PREFERENCE_KEEP`, the bootloader either keeps the requested slot preference for subsequent boots or erases it after processing.

  If the application sets a slot preference to the same value as it is currently requested, the bootloader ignores the request, thus avoiding unnecessary erase cycles on the NVM storage.
  Despite this optimization, the application should avoid sending frequent slot preference requests to minimize wear on the NVM storage.

  .. caution::

     The default dependency resolution logic assumes that the slot with the highest version number is checked first.
     If the application requests a slot preference that contradicts this assumption, it must use this functionality in combination with manifest-based dependency management (see: :ref:`ug_nrf54h20_mcuboot_manifest`) or by merging all images (see :ref:`ug_nrf54h20_partitioning_merged`).

* Request to enter a special mode, such as recovery firmware or a firmware loader.

  This functionality support devices that do not have a retained memory area.
  In such cases, the application can request the bootloader to enter a specific mode during the next boot through NVM storage.
  The bootloader always erases the boot mode requests after processing them, ensuring that the special mode is entered only once.
  The application must ensure that those requests are not sent frequently, as each request causes an erase cycle on the NVM storage.

Memory backends
***************

This module supports two memory backends:

1. A retained RAM area (default, if available).

   This backend is a thin wrapper around the regular retention driver and provides the same functionality as the default mechanism used in MCUboot.

   .. note::
      When using this backend, configure the retention driver to use a checksum to ensure data integrity.
      See :ref:`zephyr:retention_api` for details.

#. A dedicated NVM area (usually a single erase block).

   In addition to passing data, the NVM backend can preserve the slot preference across power cycles.
   It achieves this by extending the NVM area with a second copy of the data, which allows recovery after a power loss during an update operation.
   The backend verifies the data using a CRC32 checksum to ensure the integrity and validity of the returned values.

You can select the memory backend by setting the ``nrf,bootloader-request`` chosen property in the devicetree.

.. code-block:: dts

   / {
           chosen {
                   nrf,bootloader-request = &boot_request;
           };
   };

If the chosen node points to an area compatible with the ``zephyr,retention`` driver, the retention backend is used.
Otherwise, the area is treated as a NVM storage, implementing the Zephyr flash API.

You can also select the backend manually using :kconfig:option:`CONFIG_NRF_MCUBOOT_BOOT_REQUEST_IMPL_RETENTION` or :kconfig:option:`CONFIG_NRF_MCUBOOT_BOOT_REQUEST_IMPL_FLASH`.

If you want to preserve the slot preference across power cycles, the application must use the NVM backend and you must define the ``nrf,bootloader-request-backup`` chosen property accordingly.

.. code-block:: dts

   / {
           chosen {
                   nrf,bootloader-request-backup = &boot_request_backup;
           };
   };


Memory layout
*************

The memory managed by the bootloader requests module is divided into five sections, one for each request type:

* A section holding the intended boot mode for the next boot.
* A section holding the slot preference for image 0 (application core).
* A section holding the confirm request for image 0 (application core).
* A section holding the slot preference for image 1 (radio core).
* A section holding the confirm request for image 1 (radio core).

Each section has a fixed size of a single byte.
Additionally, there are two extra sections:

* A 2-byte prefix at the beginning of the memory area, used to identify valid data.
* A 4-byte checksum at the end of the memory area, used to validate the integrity of the data.

In total, the memory area managed by the bootloader requests module has a minimum size of 11 bytes.

There is no easy way to change the number of sections, as they must be kept in sync between the application and the bootloader.
By default, the MCUboot is immutable on the nRF54H20 SoC, thus the number of sections as well as their purpose and location is fixed.

The following are devicetree definitions for the retention backend and the NVM backend:

* Retention backend devicetree definition:

  .. code-block:: dts

     retainedmem {
             compatible = "zephyr,retained-ram";
             status = "okay";
             #address-cells = <1>;
             #size-cells = <1>;

             boot_request: boot_request@0 {
                     compatible = "zephyr,retention";
                     status = "okay";
                     reg = <0x0 0x10>;
                     prefix = [0B 01];
                     checksum = <4>;
             };
     };

* NVM backend devicetree definition:

  .. code-block:: dts

     partitions {
             boot_request: partition@1ad000 {
                     reg = <0x1ad000 16>;
             };

             boot_request_backup: partition@1ad010 {
                     reg = <0x1ad010 16>;
             };
     };

When you use either backend, the memory layout is as follows:

+-----------------+-------------+------------------------+-------------------------+-------------------------+-------------------------+-------------------------+-------------+-------------+
|                 | Prefix      | Boot mode              | Image 0 slot preference | Image 0 confirm request | Image 1 slot preference | Image 1 confirm request | Unused      | Checksum    |
+=================+=============+========================+=========================+=========================+=========================+=========================+=============+=============+
| Address offsets | 0x00 - 0x01 | 0x02                   | 0x03                    | 0x04                    | 0x05                    | 0x06                    | 0x07 - 0x0B | 0x0C - 0x0F |
+-----------------+-------------+------------------------+-------------------------+-------------------------+-------------------------+-------------------------+-------------+-------------+
| Values          | Fixed:      | 0x00 - regular boot    | 0x00 - no preference    | 0x00 - no request       | 0x00 - no preference    | 0x00 - no request       |             | Calculated  |
|                 |             |                        |                         |                         |                         |                         |             | from data   |
|                 | 0x0B, 0x01  | 0x01 - recovery mode   | 0x01 - prefer slot 0    | 0x01 - confirm slot 0   | 0x01 - prefer slot 0    | 0x01 - confirm slot 0   |             |             |
|                 |             |                        |                         |                         |                         |                         |             |             |
|                 |             | 0x02 - firmware loader | 0x02 - prefer slot 1    | 0x02 - confirm slot 1   | 0x02 - prefer slot 1    | 0x02 - confirm slot 1   |             |             |
+-----------------+-------------+------------------------+-------------------------+-------------------------+-------------------------+-------------------------+-------------+-------------+

Slot preference updates with power-loss protection
**************************************************

When you use the NVM backend, you can configure the bootloader requests module to protect slot preference updates against power loss.
This mechanism ensures that a power loss during an update operation cannot be used to manipulate the slot preference.

This mechanism maintains two identical copies of the boot requests data: a primary copy and a backup copy.
The bootloader creates the backup copy as part of bootloader request initialization.
The application can update only the primary copy.
Write access is restricted to the backup copy to reduce NVM wear in the backup area.

The bootloader always starts with the initialization procedure.
During initialization, it copies the primary copy to the backup copy if the primary copy is valid.

.. graphviz::
   :alt: Initialization procedure of bootloader requests module with NVM backend and backup copy
   :caption: Initialization procedure of bootloader requests module with NVM backend and backup copy
   :align: center
   :layout: dot

        digraph boot_validation_flow {
        graph [
                layout=dot,
                rankdir=TB,
                bgcolor="transparent",
                pad=0.35,
                margin=0.15,
                dpi=96,
                nodesep=0.55,
                ranksep=0.60,
                size="8.125,8.333",
                splines=polyline
        ];

        edge [
                color="#007C92",
                penwidth=1.2,
                arrowsize=0.8
        ];

        /* ── action / process nodes ── */
        node [
                shape=box,
                style="filled",
                fillcolor="#00A9CE",
                color="#00A9CE",
                fontcolor="white",
                fontname="Arial",
                fontsize=13,
                penwidth=1,
                margin="0.12,0.08"
        ];

        start            [label="boot_request_init", group=g_center];
        update_backup_a  [label="update_backup()",   group=g_left_out];
        update_backup_b  [label="update_backup()",   group=g_left_in];
        restore_backup   [label="restore_backup()",  group=g_right_in];
        clear_backup     [label="clear(backup)",     group=g_right_out];
        clear_primary    [label="clear(primary)",    group=g_right_out];
        stop_node        [label="stop",              group=g_center];

        /* ── decision diamonds ── */
        d_primary [
                label="valid(primary)?",
                shape=diamond, width=2.0, height=0.9, fixedsize=true,
                group=g_center
        ];
        d_backup_yes [
                label="valid(backup)?",
                shape=diamond, width=2.0, height=0.9, fixedsize=true,
                group=g_left
        ];
        d_backup_no [
                label="valid(backup)?",
                shape=diamond, width=2.0, height=0.9, fixedsize=true,
                group=g_right
        ];
        d_equal [
                label="!equal(primary,\nbackup)?",
                shape=diamond, width=2.0, height=0.9, fixedsize=true,
                group=g_left_in
        ];

        /* ── branch-label pseudo-nodes (plain text, no border) ── */
        node [
                shape=plain, style="", fillcolor=none,
                fontcolor="#004F5E", fontname="Arial", fontsize=11,
                fixedsize=false, width=0, height=0, margin="0.01,0.01"
        ];

        lbl_p_yes  [label="yes",  group=g_left];
        lbl_p_no   [label="no",   group=g_right];
        lbl_by_yes [label="yes",  group=g_left_in];
        lbl_by_no  [label="no",   group=g_left_out];
        lbl_bn_yes [label="yes",  group=g_right_in];
        lbl_bn_no  [label="no",   group=g_right_out];
        lbl_eq_yes [label="yes",  group=g_left_out];
        lbl_eq_no  [label="no",   group=g_left_in];

        /* invisible spacer to center d_primary between the two branches */
        node [shape=point, width=0, height=0, style=invis];
        spacer_lbl [group=g_center];

        /* ── edges ── */

        start -> d_primary [weight=10];

        /* valid(primary)? — symmetric fork */
        d_primary -> lbl_p_yes  [arrowhead=none, weight=5];
        lbl_p_yes -> d_backup_yes [weight=5];
        d_primary -> lbl_p_no   [arrowhead=none, weight=5];
        lbl_p_no  -> d_backup_no [weight=5];

        /* valid(backup)? – yes branch (left) */
        d_backup_yes -> lbl_by_yes [arrowhead=none];
        lbl_by_yes   -> d_equal;
        d_backup_yes -> lbl_by_no  [arrowhead=none];
        lbl_by_no    -> update_backup_a;

        /* !equal(primary, backup)? */
        d_equal -> lbl_eq_yes [arrowhead=none];
        lbl_eq_yes -> update_backup_b;
        d_equal -> lbl_eq_no  [arrowhead=none];
        lbl_eq_no -> stop_node;

        /* valid(backup)? – no branch (right) */
        d_backup_no -> lbl_bn_yes [arrowhead=none];
        lbl_bn_yes  -> restore_backup;
        d_backup_no -> lbl_bn_no  [arrowhead=none];
        lbl_bn_no   -> clear_backup;

        /* continuation */
        update_backup_a -> stop_node;
        update_backup_b -> stop_node;
        restore_backup  -> stop_node;
        clear_backup    -> clear_primary;
        clear_primary   -> stop_node;

        /* ── rank constraints for symmetry ── */
        { rank=same; lbl_p_yes; spacer_lbl; lbl_p_no; }
        { rank=same; d_backup_yes; d_backup_no; }
        { rank=same; lbl_by_no; lbl_by_yes; lbl_bn_yes; lbl_bn_no; }
        { rank=same; d_equal; restore_backup; }

        /* invisible ordering edges to enforce left→right symmetry */
        lbl_p_yes  -> spacer_lbl [style=invis];
        spacer_lbl -> lbl_p_no   [style=invis];

        lbl_by_no  -> lbl_by_yes -> lbl_bn_yes -> lbl_bn_no [style=invis];

        update_backup_a -> update_backup_b [style=invis, weight=1];
        update_backup_b -> restore_backup  [style=invis, weight=1];
        restore_backup  -> clear_backup    [style=invis, weight=1];
        }

..

When the bootloader processes a slot preference request, it first checks the validity of the primary copy.
If the primary copy is valid, it uses it as the source for the requested slot preference.
If the primary copy is invalid and the backup copy is valid, it uses the backup copy.
If neither copy is valid, it does not request a slot preference.

.. graphviz::
   :alt: Reading slot preference from bootloader requests module with NVM backend and backup copy
   :caption: Reading slot preference from bootloader requests module with NVM backend and backup copy
   :align: center
   :layout: dot

        digraph boot_request_get_preferred_slot {

        graph [
                layout=dot,
                rankdir=TB,
                bgcolor="transparent",
                pad=0.35,
                margin=0.15,
                dpi=96,
                nodesep=0.55,
                ranksep=0.60,
                size="8.125,8.333",
                splines=polyline
        ];

        edge [
                color="#007C92",
                penwidth=1.2,
                arrowsize=0.8
        ];

        /* ── Action / Process Nodes ── */
        node [
                shape=box,
                style="filled",
                fillcolor="#00A9CE",
                color="#00A9CE",
                fontcolor="white",
                fontname="Arial",
                fontsize=13,
                penwidth=1,
                margin="0.12,0.08"
        ];

        start        [label="boot_request_get_preferred_slot", group=g_center];
        read_primary [label="read(primary, slot_offset)",      group=g_left];
        read_backup  [label="read(backup, slot_offset)",       group=g_right];
        stop_node    [label="stop",                            group=g_center];
        end_node     [label="end",                             group=g_far_right];

        /* ── Decision Diamonds ── */
        d_primary [
                label="valid(primary)?",
                shape=diamond, width=2.0, height=0.9, fixedsize=true,
                group=g_center
        ];

        d_backup [
                label="valid(backup)?",
                shape=diamond, width=2.0, height=0.9, fixedsize=true,
                group=g_right
        ];

        /* ── Branch-Label Pseudo-Nodes ── */
        node [
                shape=plain, style="", fillcolor=none,
                fontcolor="#004F5E", fontname="Arial", fontsize=11,
                fixedsize=false, width=0, height=0, margin="0.01,0.01"
        ];

        lbl_prim_yes [label="yes", group=g_left];
        lbl_prim_no  [label="no",  group=g_right];
        lbl_back_yes [label="yes", group=g_right];
        lbl_back_no  [label="no",  group=g_far_right];

        /* ── Invisible Spacer Nodes ── */
        node [shape=point, width=0, height=0, style=invis];
        spacer_prim [group=g_center];

        /* ── Edges ── */

        /* Spine */
        start -> d_primary [weight=10];

        /* d_primary → yes (left) */
        d_primary -> lbl_prim_yes [arrowhead=none];
        lbl_prim_yes -> read_primary [weight=10];

        /* d_primary → no (right) */
        d_primary -> lbl_prim_no [arrowhead=none];
        lbl_prim_no -> d_backup [weight=10];

        /* d_backup → yes */
        d_backup -> lbl_back_yes [arrowhead=none];
        lbl_back_yes -> read_backup [weight=10];

        /* d_backup → no → end */
        d_backup -> lbl_back_no [arrowhead=none];
        lbl_back_no -> end_node;

        /* converge reads to stop */
        read_primary -> stop_node [weight=10];
        read_backup  -> stop_node;

        /* ── Rank Constraints ── */
        { rank=same; lbl_prim_yes; spacer_prim; lbl_prim_no; }
        { rank=same; read_primary; d_backup; }
        { rank=same; lbl_back_yes; lbl_back_no; }
        { rank=same; read_backup; end_node; }

        /* ── Invisible Ordering Edges ── */
        lbl_prim_yes -> spacer_prim -> lbl_prim_no [style=invis];
        lbl_back_yes -> lbl_back_no [style=invis];
        }

..

When the bootloader successfully processes boot mode or image confirm requests (that is, any requests other than slot preference requests), it erases the primary area, copies the slot preference values from the backup area to the primary area, and then updates the checksum of the primary area.
After the primary area becomes valid and contains the backed-up slot preference values, the bootloader copies the primary area to the backup area.
This ensures that both areas stay in sync after the bootloader processes requests.

.. graphviz::
   :alt: Boot requests cleaning procedure after processing requests with NVM backend and backup copy
   :caption: Boot requests cleaning procedure after processing requests with NVM backend and backup copy
   :align: center
   :layout: dot

        digraph boot_request_clear_flow {
        graph [
                layout=dot,
                rankdir=TB,
                bgcolor="transparent",
                pad=0.35,
                margin=0.15,
                dpi=96,
                nodesep=0.55,
                ranksep=0.60,
                size="8.125,8.333",
                splines=polyline,
                compound=true
        ];

        edge [
                color="#007C92",
                penwidth=1.2,
                arrowsize=0.8
        ];

        /* ── action / process nodes ── */
        node [
                shape=box,
                style="filled",
                fillcolor="#00A9CE",
                color="#00A9CE",
                fontcolor="white",
                fontname="Arial",
                fontsize=13,
                penwidth=1,
                margin="0.12,0.08"
        ];

        boot_request_clear  [label="boot_request_clear",     group=g_main];
        update_needed_false [label="update_needed = false",  group=g_main];
        erase_primary       [label="erase(primary)",         group=g_main];
        update_digest       [label="update_digest(primary)", group=g_main];
        update_backup       [label="update_backup()",        group=g_main];
        stop_node           [label="stop",                   group=g_main];

        /* ── decision diamonds ── */
        d_valid_backup [
                label="valid(backup)?",
                shape=diamond, width=2.0, height=0.9, fixedsize=true,
                group=g_main
        ];
        d_update_needed [
                label="update_needed?",
                shape=diamond, width=2.0, height=0.9, fixedsize=true,
                group=g_main
        ];

        /* ── loop 1: check equality across slots ── */
        subgraph cluster_loop1 {
                label="loop over slot_preferences";
                labeljust=l;
                style=dashed;
                color="#007C92";
                fontname="Arial";
                fontsize=11;
                fontcolor="#004F5E";
                margin=16;

                node [
                shape=box, style="filled", fillcolor="#00A9CE", color="#00A9CE",
                fontcolor="white", fontname="Arial", fontsize=13,
                penwidth=1, margin="0.12,0.08"
                ];
                update_needed_true [label="update_needed = true", group=g_side];

                d_equal [
                label="!equal(backup,\nprimary)?",
                shape=diamond, width=2.0, height=0.9, fixedsize=true,
                group=g_main
                ];

                node [
                shape=plain, style="", fillcolor=none,
                fontcolor="#004F5E", fontname="Arial", fontsize=11,
                fixedsize=false, width=0, height=0, margin="0.01,0.01"
                ];
                lbl_eq_yes [label="yes", group=g_side];
        }

        /* ── loop 2: write preferred slots ── */
        subgraph cluster_loop2 {
                label="loop over slot_preferences";
                labeljust=l;
                labelloc=t;
                style=dashed;
                color="#007C92";
                fontname="Arial";
                fontsize=11;
                fontcolor="#004F5E";
                margin=20;

                node [shape=box, fixedsize=true, style=invis, label=""];
                loop2_entry    [width=0.01, height=0.01, group=g_main];
                loop2_pad_left [width=2.5,  height=0.01, group=g_side];

                node [
                shape=box, style="filled", fillcolor="#00A9CE", color="#00A9CE",
                fontcolor="white", fontname="Arial", fontsize=13,
                penwidth=1, margin="0.12,0.08", fixedsize=false
                ];
                write_primary [label="write(primary,\nslot_offset,\npreferred_slot)", group=g_main];
        }

        /* ── branch-label pseudo-nodes (outside clusters) ── */
        node [
                shape=plain, style="", fillcolor=none,
                fontcolor="#004F5E", fontname="Arial", fontsize=11,
                fixedsize=false, width=0, height=0, margin="0.01,0.01"
        ];

        lbl_vb_yes [label="yes", group=g_main];
        lbl_vb_no  [label="no",  group=g_side];
        lbl_eq_no  [label="no",  group=g_main];
        lbl_un_yes [label="yes", group=g_main];
        lbl_un_no  [label="no",  group=g_side];

        /* ── edges ── */

        boot_request_clear -> d_valid_backup [weight=10];

        /* valid(backup)? — yes continues down spine, no bypasses to stop */
        d_valid_backup -> lbl_vb_yes [arrowhead=none, weight=10];
        lbl_vb_yes -> update_needed_false [weight=10];
        d_valid_backup -> lbl_vb_no  [arrowhead=none];
        lbl_vb_no -> stop_node;

        /* update_needed = false → loop 1 */
        update_needed_false -> d_equal [weight=10];

        /* !equal(...)? — no continues down spine, yes detours to set flag */
        d_equal -> lbl_eq_no  [arrowhead=none, weight=10];
        lbl_eq_no -> d_update_needed [weight=10];
        d_equal -> lbl_eq_yes [arrowhead=none];
        lbl_eq_yes -> update_needed_true;
        update_needed_true -> d_update_needed;

        /* update_needed? — yes continues down spine, no bypasses to stop */
        d_update_needed -> lbl_un_yes [arrowhead=none, weight=10];
        lbl_un_yes -> erase_primary [weight=10];
        d_update_needed -> lbl_un_no  [arrowhead=none];
        lbl_un_no -> stop_node;

        /* update sequence */
        erase_primary -> loop2_entry [arrowhead=none, weight=10];
        loop2_entry   -> write_primary [weight=10];
        loop2_pad_left -> write_primary [style=invis];
        write_primary -> update_digest [weight=10];
        update_digest -> update_backup [weight=10];
        update_backup -> stop_node     [weight=10];

        /* ── rank constraints ── */
        { rank=same; lbl_vb_yes; lbl_vb_no; }
        { rank=same; lbl_eq_yes; lbl_eq_no; }
        { rank=same; lbl_un_yes; lbl_un_no; }
        }

..

When the application creates a request, it first checks the validity of the primary copy.
If the primary copy is invalid and the backup copy is valid, it restores the backup copy to the primary copy before proceeding.
If neither copy is valid, it erases the primary area and processes the request as usual.
If the primary copy is valid and already matches the requested value, it ignores the request to avoid unnecessary erase cycles.
If the primary area initialization fails or the backup copy cannot be restored, the request fails with an error code.

.. graphviz::
   :alt: Setting slot preference using boot requests module with NVM backend and backup copy
   :caption: Setting slot preference using boot requests module with NVM backend and backup copy
   :align: center
   :layout: dot

        digraph boot_request_set_preferred_slot {

        graph [
                layout=dot,
                rankdir=TB,
                bgcolor="transparent",
                pad=0.35,
                margin=0.15,
                dpi=96,
                nodesep=0.55,
                ranksep=0.60,
                size="8.125,8.333",
                splines=polyline
        ];

        edge [
                color="#007C92",
                penwidth=1.2,
                arrowsize=0.8
        ];

        /* ── Action / Process Nodes ── */
        node [
                shape=box,
                style="filled",
                fillcolor="#00A9CE",
                color="#00A9CE",
                fontcolor="white",
                fontname="Arial",
                fontsize=13,
                penwidth=1,
                margin="0.12,0.08"
        ];

        start           [label="boot_request_set_preferred_slot", group=g_center];
        restore_backup  [label="restore_backup()",                group=g_bk_left];
        clear_primary   [label="clear(primary)",                  group=g_bk_right];
        write_primary   [label="write(primary,\nslot_offset,\npreferred_slot)", group=g_center];
        update_digest   [label="update_digest(primary)",          group=g_center];
        stop_node       [label="stop",                            group=g_center];
        end_node        [label="end",                             group=g_right];

        /* ── Decision Diamonds ── */
        node [
                shape=diamond,
                width=2.0,
                height=0.9,
                fixedsize=true
        ];

        d_primary  [label="valid(primary)?", group=g_center];
        d_backup   [label="valid(backup)?",  group=g_left];
        d_primary2 [label="valid(primary)?", group=g_center];

        /* ── Branch-Label Pseudo-Nodes ── */
        node [
                shape=plain, style="", fillcolor=none,
                fontcolor="#004F5E", fontname="Arial", fontsize=11,
                fixedsize=false, width=0, height=0, margin="0.01,0.01"
        ];

        lbl_p1_yes [label="yes", group=g_right];
        lbl_p1_no  [label="no",  group=g_left];
        lbl_bk_yes [label="yes", group=g_bk_left];
        lbl_bk_no  [label="no",  group=g_bk_right];
        lbl_p2_yes [label="yes", group=g_center];
        lbl_p2_no  [label="no",  group=g_right];

        /* ── Invisible Spacer Nodes ── */
        node [shape=point, width=0, height=0, style=invis];
        sp_p1 [group=g_center];
        sp_bk [group=g_left];

        /* ── Edges ── */

        /* Spine */
        start -> d_primary [weight=10];

        /* d_primary → yes (right — primary is valid, skip repair) */
        d_primary -> lbl_p1_yes [arrowhead=none];
        lbl_p1_yes -> d_primary2;

        /* d_primary → no (left — enter repair sub-flow) */
        d_primary -> lbl_p1_no  [arrowhead=none];
        lbl_p1_no -> d_backup   [weight=10];

        /* d_backup → yes → restore_backup → merge at d_primary2 */
        d_backup -> lbl_bk_yes  [arrowhead=none];
        lbl_bk_yes -> restore_backup [weight=10];
        restore_backup -> d_primary2;

        /* d_backup → no → clear_primary → merge at d_primary2 */
        d_backup -> lbl_bk_no   [arrowhead=none];
        lbl_bk_no -> clear_primary [weight=10];
        clear_primary -> d_primary2;

        /* d_primary2 → yes (continue down spine) */
        d_primary2 -> lbl_p2_yes [arrowhead=none, weight=10];
        lbl_p2_yes -> write_primary [weight=10];
        write_primary -> update_digest [weight=10];
        update_digest -> stop_node [weight=10];

        /* d_primary2 → no → end */
        d_primary2 -> lbl_p2_no  [arrowhead=none];
        lbl_p2_no -> end_node;

        /* ── Rank Constraints ── */
        { rank=same; lbl_p1_no; sp_p1; lbl_p1_yes; }
        { rank=same; lbl_bk_yes; sp_bk; lbl_bk_no; }
        { rank=same; restore_backup; clear_primary; }
        { rank=same; lbl_p2_yes; lbl_p2_no; }
        { rank=same; write_primary; end_node; }

        /* ── Invisible Ordering Edges ── */
        lbl_p1_no -> sp_p1 -> lbl_p1_yes [style=invis];
        lbl_bk_yes -> sp_bk -> lbl_bk_no [style=invis];
        restore_backup -> clear_primary [style=invis];
        lbl_p2_yes -> lbl_p2_no [style=invis];
        write_primary -> end_node [style=invis];
        }

..

This mechanism ensures that even in the event of a power loss during an update operation, the bootloader can always recover a valid slot preference from the backup copy, maintaining the integrity of the boot process.
