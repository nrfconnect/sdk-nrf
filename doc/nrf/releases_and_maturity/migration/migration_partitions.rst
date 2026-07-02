.. _migration_partitions:

Migrating partition configuration from Partition Manager to devicetree (DTS)
############################################################################

.. contents::
   :local:
   :depth: 2

Partition Manager is a component in the |NCS| that handles flash memory partitioning at build time.
The |NCS| is phasing out Partition Manager in favor of Zephyr's default devicetree-based flash partitioning.
Use the following migration procedure for existing projects that still rely on Partition Manager except for the nrf91 Series devices.

Partition Manager deprecation timeline
***************************************

Partition Manager is phased out across three major |NCS| releases:

* In |NCS| v3.3, Partition Manager stays enabled by default for all builds to avoid breaking existing applications.
  However, the following changes have been applied to Partition Manager and devicetree partitioning:

   * The build issues a warning when Partition Manager is enabled.
   * MCUboot and TF-M are updated to use devicetree partitions instead of Partition Manager.
   * Built-in samples and applications are updated incrementally on the v3.3 branch to use devicetree-based partitioning.
   * A Partition Manager-to-DTS helper script helps you migrate from Partition Manager to devicetree partitions.
     The script generates devicetree overlay files based on existing Partition Manager configurations.
     You can find the script in the :file:`scripts/pm_to_dts.py` file in your |NCS| installation.
   * Partition Manager is deprecated but remains available.

* In the next |NCS| major release after v3.3, Partition Manager is disabled by default for all builds.

* In the |NCS| major release after that, Partition Manager is removed from the codebase.

Determine whether the migration is needed for your project
**********************************************************

To determine whether the migration is needed, check the following for your project:

   * The project does not disable Partition Manager (setting :kconfig:option:`SB_CONFIG_PARTITION_MANAGER` to ``n``) in the build configuration.
   * The project uses a :file:`pm_static.yml` file or similar ``pm_static*`` files.

If either (or both) of the above conditions are true, the migration is needed.
If not, go to the :ref:`migration_partitions_post_migration` section.

Migrate your project to DTS-partitioning
****************************************

To migrate your project to DTS-partitioning, follow these steps:

1. Identify each board target and each build configuration that can change the partition layout.

   Run the following steps for each combination.

   If your project supports only one board and one configuration, run the procedure once.

#. Configure a build without compiling sources.

   *Configuring a build* means generating build system files only, without compiling sources.
   Use the ``--cmake-only`` option.

   Run the following command from the workspace root:

   .. code-block:: none

      west build -p -b <board_target> <app_or_test_path> --cmake-only

   This generates a :file:`build/` directory with devicetree and CMake context.
   No object files or binaries are compiled at this stage.

#. Run the Partition Manager-to-DTS helper script on the build directory.
   Replace ``<path-to-nrf/scripts/pm_to_dts.py>`` with the path to the :file:`scripts/pm_to_dts.py` file in your |NCS| installation.

   .. code-block:: none

      python <path-to-nrf/scripts/pm_to_dts.py> build

   This generates a :file:`<board_target>.overlay` file and, if applicable, a :file:`<board>_<soc>_cpunet.overlay` file.
   The script will also print the generated partition nodes to the console.

#. Compare the generated partitions with the default board devicetree partitions.
   Compare the content of the partition nodes, including the following:

   * Node labels
   * Addresses and offsets
   * Sizes
   * Presence or absence of nodes

   If the generated partition nodes match the default board DTS partitioning, migration for this board/configuration is complete:

   * Do not replace any dts node in the default Board DTS file.
   * Remove corresponding :file:`pm_static*` files, if present.
   * Continue with the next board/configuration from the first step.

#. If partitions differ, change the partitions in the Board DTS file by the generated ones in the overlay file(s).

#. Manually review and fix generated output.

   The helper script is not guaranteed to produce correct output for every board/configuration.
   Manually review and fix generated overlays as needed.

   Typical fixes include the following:

   * Correcting partition labels
   * Correcting sizes and offsets
   * Adding missing partition nodes
   * Removing incorrect extra nodes

   Ensure partition ordering and boundaries match the intended flash layout for the board/configuration.

#. Build and run with Partition Manager disabled.

   Reconfigure and build with Partition Manager disabled:

   .. code-block:: none

      west build -p \
        -b <board_target> \
        <app_or_test_path> \
        -- \
        -DSB_CONFIG_PARTITION_MANAGER=n

   Then flash and verify the result:

   .. code-block:: none

      west flash

   Confirm that the app or test runs as expected with devicetree partitions only.

#. Remove migrated ``pm_static*`` files.

   Delete ``pm_static*`` files.
   If migration is done in phases, remove only files that have already been ported.
   Delete the generated overlay files.

.. _migration_partitions_post_migration:

Post-migration tasks
********************

Disable Partition Manager by default in your board sysbuild Kconfig file :file:`Kconfig.sysbuild`.

For example:

.. code-block:: kconfig

   config PARTITION_MANAGER
      default n
