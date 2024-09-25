.. _dfu_tools_mcumgr_cli:

Command-line tool
#################

.. contents::
   :local:
   :depth: 2

MCUmgr provides a :file:`mcumgr` command-line tool for managing remote devices.
The tool is written in the Go programming language.

.. note::
    This tool is provided for evaluation use only and is not recommended for use in a production environment.
    It has known issues and will not respect the MCUmgr protocol, for example, upon an error.
    In some circumstances, it will sit in an endless loop of sending the same command instead of aborting the task.
    Replacement for this tool is currently in development.
    Once released, support for the Go tool will be dropped.
    For production environment, use tools listed in the :ref:`zephyr:mcumgr_tools_libraries` section instead.

To install the tool, run the following command:

.. tabs::

   .. group-tab:: go version < 1.18

      .. code-block:: console

         go get github.com/apache/mynewt-mcumgr-cli/mcumgr

   .. group-tab:: go version >= 1.18

      .. code-block:: console

         go install github.com/apache/mynewt-mcumgr-cli/mcumgr@latest

Configuring the transport
*************************

There are two command-line options that are responsible for setting and configuring the transport layer when communicating with managed device:

* The ``--conntype`` option is used to choose the transport.
* The ``--connstring`` option is used to pass a comma-separated list of options in the ``key=value`` format, where each valid ``key`` depends on the particular ``conntype``.

Valid transports for ``--conntype`` are ``serial``, ``ble``, and ``udp``.
Each transport expects a different set of key or value options:

.. tabs::

   .. group-tab:: serial

      ``--connstring`` accepts the following ``key`` values:

      .. list-table::
         :width: 100%
         :widths: 10 60

         * - ``dev``
           - The device name for the OS ``mcumgr`` is running (for example, ``/dev/ttyUSB0``, ``/dev/tty.usbserial``, ``COM1``).
         * - ``baud``
           - The communication speed, which must match the baudrate of the server.
         * - ``mtu``
           - Maximum Transmission Unit, the maximum protocol packet size.

   .. group-tab:: ble

      ``--connstring`` accepts the following ``key`` values:

      .. list-table::
         :width: 100%
         :widths: 10 60

         * - ``ctlr_name``
           - An OS specific string for the controller name.
         * - ``own_addr_type``
           - Can be one of ``public``, ``random``, ``rpa_pub``, ``rpa_rnd``, where ``random`` is the default.
         * - ``peer_name``
           - The name the peer Bluetooth® LE device advertises, which should match the configuration specified with the :kconfig:option:`CONFIG_BT_DEVICE_NAME` Kconfig option.
         * - ``peer_id``
           - The peer Bluetooth LE device address or UUID.
             It is only required when ``peer_name`` was not given.
             The format depends on the OS where ``mcumgr`` is run (it is a 6 bytes hexadecimal string separated by colons on Linux, or a 128-bit UUID on macOS).
         * - ``conn_timeout``
           - A float number representing the connection timeout in seconds.

   .. group-tab:: udp

      ``--connstring`` takes the form ``[addr]:port`` where:

      .. list-table::
         :width: 100%
         :widths: 10 60

         * - ``addr``
           - Can be a DNS name (if it can be resolved to the device IP), IPv4 address.
             The application must be built with the :kconfig:option:`CONFIG_MCUMGR_TRANSPORT_UDP_IPV4` Kconfig option, or IPv6 address (:kconfig:option:`CONFIG_MCUMGR_TRANSPORT_UDP_IPV6`).
         * - ``port``
           - Any valid UDP port.

Saving the connection configuration
***********************************

You can manage the transport configuration with the ``conn`` sub-command and later use it with the ``--conn`` (or ``-c``) parameter instead of typing ``--conntype`` and ``--connstring``.
For example, a new config for a serial device that would require the ``mcumgr --conntype serial --connstring dev=/dev/ttyACM0,baud=115200,mtu=512`` command can be simplified as follows:

.. code-block:: console

   mcumgr conn add acm0 type="serial" connstring="dev=/dev/ttyACM0,baud=115200,mtu=512"

You can access this port by running:

.. code-block:: console

   mcumgr -c acm0

.. _dfu_tools_mcumgr_cli_options:

General options
***************

Some options work for every ``mcumgr`` command and might help with debugging and fixing communication issues.
Among them, the following options are included:

.. list-table::
   :width: 100%
   :widths: 10 60

   * - ``-l <log-level>``
     - Configures the log level, which ranges from ``critical``, ``error``, ``warn``, ``info`` or ``debug``, from least to most verbose.
       In case of communication issues, using ``-lDEBUG`` can be helpful to capture packet data for further analysis.
   * - ``-t <timeout>``
     - Adjusts the timeout period for waiting for a response, changing it from the default 10 seconds to a specified value.
       This is useful for commands that require longer processing times, such as erasing data before uploading an image, where increasing the timeout may be necessary.
   * - ``-r <tries>``
     - Modifies the number of retry attempts after a timeout, adjusting it from the default ``1`` to a specified value.

List of commands
****************

If you run ``mcumgr`` with no parameters, or use ``-h``, it will display the list of commands.
However, not all commands defined by ``mcumgr`` (and SMP protocol) are currently supported in Zephyr.
The ones that are supported are as follows:

.. list-table::
   :widths: 10 30
   :header-rows: 1

   * - Command
     - Description
   * - ``echo``
     - Sends data to a device and displays the echoed response.
       This command is part of the ``OS`` group, and you must enable it by setting the :kconfig:option:`CONFIG_MCUMGR_GRP_OS` Kconfig option.
       To enable the ``echo`` command specifically, enable the :kconfig:option:`CONFIG_MCUMGR_GRP_OS_ECHO` Kconfig option.
   * - ``fs``
     - Provides access to files on a device.
       For more details, see :ref:`dfu_tools_mcumgr_cli_fs_mgmt`.
   * - ``image``
     - Manages firmware images on a device.
       For additional information, see :ref:`dfu_tools_mcumgr_cli_image_mgmt`.
   * - ``reset``
     - Performs a soft reset of a device.
       This command is part of the ``OS`` group, which must be enabled by setting the :kconfig:option:`CONFIG_MCUMGR_GRP_OS` Kconfig option.
       The ``reset`` command is always enabled and the duration of the reset process can be configured with the :kconfig:option:`CONFIG_MCUMGR_GRP_OS_RESET_MS` Kconfig option (in ms).
   * - ``shell``
     - Executes a command in the remote shell.
       This option is disabled by default and can be enabled with the :kconfig:option:`CONFIG_MCUMGR_GRP_SHELL` Kconfig option.
       For more details, see :ref:`zephyr:shell_api`.
   * - ``stat``
     - Reads statistic data from a device.
       Fore more details, see :ref:`dfu_tools_mcumgr_cli_stats_mgmt`.
   * - ``taskstat``
     - Reads task statistics from a device.
       This command is part of the ``OS`` group.
       To use it, you must enable the :kconfig:option:`CONFIG_MCUMGR_GRP_OS` and :kconfig:option:`CONFIG_MCUMGR_GRP_OS_TASKSTAT` Kconfig options.
       Additionally, the :kconfig:option:`CONFIG_THREAD_MONITOR` Kconfig option must be enabled; otherwise, a ``-8`` (``MGMT_ERR_ENOTSUP``) will be returned.

``taskstat`` has a few options that might require adjustments.
To display task names, the :kconfig:option:`CONFIG_THREAD_NAME` Kconfig option must be enabled.
Otherwise, only the priority will be shown.
Additionally, due to the large size of  ``taskstat`` packets, it might be necessary to increase the :kconfig:option:`CONFIG_MCUMGR_TRANSPORT_NETBUF_SIZE` Kconfig option.

To display the correct stack size in the ``taskstat`` command, you must enable the :kconfig:option:`CONFIG_THREAD_STACK_INFO`.
To display the correct stack usage in the ``taskstat`` command, you must enable both the :kconfig:option:`CONFIG_THREAD_STACK_INFO` and :kconfig:option:`CONFIG_INIT_STACKS` Kconfig options.

.. _dfu_tools_mcumgr_cli_image_mgmt:

Image management
****************

The image management provided by ``mcumgr`` is based on the image format defined by MCUboot.
For more details, see `MCUboot design`_ and :ref:`zephyr:west-sign`.

To view a list of available images on a device, run the following command:

.. code-block:: console

  mcumgr <connection-options> image list

You should see the following output, where ``image`` represents the number of the image pair in a multi-image system, and ``slot`` indicates the slot number where the image is stored: ``0`` for the primary and ``1`` for the secondary slot.

.. code-block:: console

  $ mcumgr -c acm0 image list
  Images:
    image=0 slot=0
      version: 1.0.0
      bootable: true
      flags: active confirmed
      hash: 86dca73a3439112b310b5e033d811ec2df728d2264265f2046fced5a9ed00cc7
  Split status: N/A (0)

The image is ``active`` and ``confirmed``, which means it will be executed again upon next reset.
Additionally, the ``hash`` associated with this image is used by other commands to identify and interact with this specific image during operations.

You can erase the image manually with the command:

.. code-block:: console

  mcumgr <connection-options> image erase

The behavior of ``erase`` command is defined by the server (``MCUmgr`` on the device).
The current implementation is limited to erasing the image in the secondary partition.

.. note::
  ``mcumgr`` does not support HEX files.
  When uploading a new image, always use the BIN format.

To upload a new image, run the following command:

.. code-block:: console

  mcumgr <connection-options> image upload [-n] [-e] [-u] [-w] <signed-bin>

* ``-n`` - The option enables the uploading of a new image to a specific set of images within a multi-image system.
  It is currently supported by MCUboot only when the ``CONFIG\ _MCUBOOT_SERIAL`` option is enabled.
* ``-e`` - The option prevents a full erase of the partition before initiating a new upload.

    The ``-e`` option should always be included because the ``upload`` command automatically checks if an erase is required, adhering to the :kconfig:option:`CONFIG_IMG_ERASE_PROGRESSIVELY` Kconfig option.
    If the ``upload`` command times out while waiting for a response from the
    device, you can use the ``-t`` option to extend the wait time beyond the default 10 seconds.
    For more details, see :ref:`dfu_tools_mcumgr_cli_options`.

* ``-u`` - The option allows upgrading only to newer image version.
* ``-w`` - The option sets the maximum size for the window of outstanding chunks in transit, with a default value of 5.

    If the option is set to a value lower than the default, for example ``-w 1``, fewer chunks are transmitted on the window, which reduces the risk of errors.
    Conversely, setting a value higher than 5 increases the risk of errors and might negatively impact performance.

After completing an image upload, the output of a new ``image list`` should look as follows:

.. code-block:: console

  $ mcumgr -c acm0 image upload -e build/zephyr/zephyr.signed.bin
    35.69 KiB / 92.92 KiB [==========>---------------]  38.41% 2.97 KiB/s 00m19

You can list the images again to see the updated status:

.. code-block:: console

  $ mcumgr -c acm0 image list
  Images:
   image=0 slot=0
    version: 1.0.0
    bootable: true
    flags: active confirmed
    hash: 86dca73a3439112b310b5e033d811ec2df728d2264265f2046fced5a9ed00cc7
   image=0 slot=1
    version: 1.1.0
    bootable: true
    flags:
    hash: e8cf0dcef3ec8addee07e8c4d5dc89e64ba3fae46a2c5267fc4efbea4ca0e9f4
  Split status: N/A (0)

Verifying and testing the image
===============================

To test a new upgrade image, use the command:

.. code-block:: console

  mcumgr <connection-options> image test <hash>

This command initiates a ``test`` upgrade, indicating that after the next reboot, the bootloader will execute the upgrade and switch to the new image.
If no further image operations are performed on the newly running image, it will ``revert`` to the previously active image on the device during the subsequent reset.
When a ``test`` is requested, the ``flags`` will be updated to include ``pending`` to inform that a new image will be executed after a reset:

.. code-block:: console

  $ mcumgr -c acm0 image test e8cf0dcef3ec8addee07e8c4d5dc89e64ba3fae46a2c5267fc4efbea4ca0e9f4
  Images:
   image=0 slot=0
    version: 1.0.0
    bootable: true
    flags: active confirmed
    hash: 86dca73a3439112b310b5e033d811ec2df728d2264265f2046fced5a9ed00cc7
   image=0 slot=1
    version: 1.1.0
    bootable: true
    flags: pending
    hash: e8cf0dcef3ec8addee07e8c4d5dc89e64ba3fae46a2c5267fc4efbea4ca0e9f4
  Split status: N/A (0)

After a reset, you will see the following output:

.. code-block:: console

  $ mcumgr -c acm0 image list
  Images:
   image=0 slot=0
    version: 1.1.0
    bootable: true
    flags: active
    hash: e8cf0dcef3ec8addee07e8c4d5dc89e64ba3fae46a2c5267fc4efbea4ca0e9f4
   image=0 slot=1
    version: 1.0.0
    bootable: true
    flags: confirmed
    hash: 86dca73a3439112b310b5e033d811ec2df728d2264265f2046fced5a9ed00cc7
  Split status: N/A (0)

An upgrade will only proceed if the image is valid.
When an upgrade is requested, the first action MCUboot performs is image validation.
The process involves checking the SHA-256 hash and, depending on the configuration, verifying signature.
To ensure the validity of an image before uploading, run the command:

.. code-block:: console

  imgtool verify -k <your-signature-key> <your-image>``

In this command, the ``-k <your-signature-key>`` option is necessary for signature verification and can be omitted if signature validation is not configured.

The ``confirmed`` flag in the secondary slot indicates that after the next reset a revert upgrade will be performed to switch back to the original layout.
To prevent this revert and confirm that the current image is functioning correctly, use the ``confirm`` command.
If you want to confirm the currently running image, it should be executed without specifying a hash:

.. code-block:: console

  mcumgr <connection-options> image confirm ""

The confirm command can also be executed with a specific hash.
This method allows for a direct upgrade to the image in the secondary partition, bypassing the usual ``test``\``revert`` procedure:

.. code-block:: console

  mcumgr <connection-options> image confirm <hash>

.. note::

  You do not need to manage the entire ``test``/``revert`` cycle using only the ``mcumgr`` command-line tool.
  A more efficient approach is to perform a ``test`` and allow the new running image to self-confirm by calling the :c:func:`boot_write_img_confirmed` function after completing necessary checks.

Enabling the :kconfig:option:`CONFIG_MCUMGR_GRP_IMG_VERBOSE_ERR` Kconfig option improves error messaging when failures occur, although it does increase the application size.

.. _dfu_tools_mcumgr_cli_stats_mgmt:

Statistics management
*********************

Statistics are used for troubleshooting, maintenance, and monitoring usage.
They primarily consist of user-defined counters that are closely connected to ``mcumgr``.
These counters can be used to track various types of information for easy retrieval.
The available sub-commands are as follows:

.. code-block:: console

  mcumgr <connection-options> stat list
  mcumgr <connection-options> stat <section-name>

Statistics in ``mcumgr`` are organized into sections (also called groups), and each section can be individually queried.
You can define new statistics sections by using macros available in the :file:`zephyr/stats/stats.h` file.
Each section consists of multiple variables (or counters), which are all of the same size (16, 32 or 64 bits).

To create a new section named ``my_stats``, use the following:

.. code-block:: console

  STATS_SECT_START(my_stats)
    STATS_SECT_ENTRY(my_stat_counter1)
    STATS_SECT_ENTRY(my_stat_counter2)
    STATS_SECT_ENTRY(my_stat_counter3)
  STATS_SECT_END;
  STATS_SECT_DECL(my_stats) my_stats;

Each entry can be declared using one of the following macros: :c:macro:`STATS_SECT_ENTRY` (or the equivalent
:c:macro:`STATS_SECT_ENTRY32`), :c:macro:`STATS_SECT_ENTRY16`, or
:c:macro:`STATS_SECT_ENTRY64`.
All statistics in a section must be declared with the same size.
Whether the statistics counters are named depends on the configuration of the :kconfig:option:`CONFIG_STATS_NAMES` Kconfig option.
Adding names requires an additional step:

.. code-block:: console

  STATS_NAME_START(my_stats)
    STATS_NAME(my_stats, my_stat_counter1)
    STATS_NAME(my_stats, my_stat_counter2)
    STATS_NAME(my_stats, my_stat_counter3)
  STATS_NAME_END(my_stats);

The :kconfig:option:`CONFIG_MCUMGR_GRP_STAT_MAX_NAME_LEN` Kconfig option sets the maximum length of a section name that can can be accepted as parameter for showing the section data.
This may need adjustment for particularly long section names.

.. note::

  Disabling the :kconfig:option:`CONFIG_STATS_NAMES` Kconfig option will free resources.
  When this option is disabled, the ``STATS_NAME*`` macros produce no output, so adding them in the code does not increase the binary size.

The final steps to use a statistics section is to initialize and register it:

.. code-block:: console

  rc = STATS_INIT_AND_REG(my_stats, STATS_SIZE_32, "my_stats");
  assert (rc == 0);

Once initialized, you can manipulate the statistics counters in your running code.
A counter can be incremented by 1 using :c:macro:`STATS_INC`, by N using :c:macro:`STATS_INCN`, or reset with :c:macro:`STATS_CLEAR`.

For example, you can increment three counters by ``1``, ``2`` and ``3`` respectively every second.
Check the list of statistics, by running the following command:

.. code-block:: console

  $ mcumgr --conn acm0 stat list
  stat groups:
    my_stats

Get the current value of the counters in ``my_stats``:

.. code-block:: console

  $ mcumgr --conn acm0 stat my_stats
  stat group: my_stats
        13 my_stat_counter1
        26 my_stat_counter2
        39 my_stat_counter3
  $ mcumgr --conn acm0 stat my_stats
  stat group: my_stats
        16 my_stat_counter1
        32 my_stat_counter2
        48 my_stat_counter3

If the :kconfig:option:`CONFIG_STATS_NAMES` Kconfig option is disabled, the output will look as follows:

.. code-block:: console

  $ mcumgr --conn acm0 stat my_stats
  stat group: my_stats
         8 s0
        16 s1
        24 s2

.. _dfu_tools_mcumgr_cli_fs_mgmt:

Filesystem management
*********************

The filesystem module is disabled by default due to security concerns.
Without access control, enabling this module would allow all files within the filesystem to be accessible, including sensitive data such as secrets.
To enable it, you must set the :kconfig:option:`CONFIG_MCUMGR_GRP_FS` Kconfig option.
Once enabled, the following sub-commands are available:

.. code-block:: console

  mcumgr <connection-options> fs download <remote-file> <local-file>
  mcumgr <connection-options> fs upload <local-file> <remote-file>

To use the ``fs`` command, you must enable the :kconfig:option:`CONFIG_FILE_SYSTEM` Kconfig option.
Additionally, a specific filesystem must be enabled and properly mounted by the running application.
For example, to use littleFS, enable the :kconfig:option:`CONFIG_FILE_SYSTEM_LITTLEFS` Kconfig option, define a storage partition using :ref:`zephyr:flash_map_api`,
and mount the filesystem at startup using the :c:func:`fs_mount` function.

To upload a new file to a littleFS storage, mounted under ``/lfs``, use the following command:

.. code-block:: console

  $ mcumgr -c acm0 fs upload foo.txt /lfs/foo.txt
  25
  Done

where ``25`` is the size of the file.

To download a file, first you must use the ``fs`` command with the :kconfig:option:`CONFIG_FILE_SYSTEM_SHELL` Kconfig option enabled.
This allows operations via remote shell.
Create a new file on the remote system:

.. code-block:: console

  uart:~$ fs write /lfs/bar.txt 41 42 43 44 31 32 33 34 0a
  uart:~$ fs read /lfs/bar.txt
  File size: 9
  00000000  41 42 43 44 31 32 33 34 0A                       ABCD1234.

Now, you can download the file to your local system:

.. code-block:: console

  $ mcumgr -c acm0 fs download /lfs/bar.txt bar.txt
  0
  9
  Done
  $ cat bar.txt
  ABCD1234

where ``0`` is the return code, and ``9`` is the size of the file.

.. note::

  Using these commands might exhaust the system workqueue if its size is not large enough.
  You might need to increase the stack size using the :kconfig:option:`CONFIG_SYSTEM_WORKQUEUE_STACK_SIZE` Kconfig option.

  You can adjust the size of the stack-allocated buffer, which is used to store the blocks while transferring a file, with the :kconfig:option:`CONFIG_MCUMGR_GRP_FS_DL_CHUNK_SIZE` Kconfig option.
  This adjustment saves RAM resources.

  The :kconfig:option:`CONFIG_MCUMGR_GRP_FS_PATH_LEN` Kconfig option sets the maximum path length accepted for a file name.
  It might require adjustments for longer file names.

.. note::

  To enhance security within the filesystem management group, user applications can register callbacks for MCUmgr hooks.
  These callbacks are invoked during upload and download operations, allowing the application to control whether access to a file should be allowed or denied.
  For more details, refer to the :ref:`zephyr:mcumgr_callbacks` section.
