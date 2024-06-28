.. _ug_nrf54h20_suit_customize_dfu:

How to customize the SUIT DFU process
#####################################

.. contents::
   :local:
   :depth: 2

Nordic Semiconductor provides a Software Update Internet of Things (SUIT) sample (:ref:`ug_nrf54h20_suit_intro`) which uses predefined configurations in the build system.
The specified Kconfig options in the sample can be used to customize the DFU process and integrate the DFU solution with external build systems.
This guide provides a comprehensive set of instructions for modifying values in the :ref:`SUIT manifest <ug_nrf54h20_suit_manifest_overview>`.

Overview of the SUIT DFU procedure
**********************************

The SUIT DFU protocol is essential for secure, reliable firmware updates in IoT devices, particularly for the nRF54H20 SoC.
It consists of two main concepts: the SUIT envelope and the SUIT manifest.

* The SUIT envelope acts as a secure container for transporting firmware updates, encapsulating the firmware binary and its manifest.
* The SUIT manifest is a structured file with metadata essential for the update process, including firmware version, size, and hash for integrity verification.

Default manifest templates are provided by Nordic Semiconductor and used by default during application build.
These templates are suitable for simple cases and form the basis for generating SUIT envelopes and manifests tailored to specific application requirements.
Customization of these templates is crucial for specific use cases and security requirements.

For detailed guidance on customizing these templates and creating your own templates, refer to the :ref:`ug_suit_use_own_manifest` section.

.. figure:: images/nrf54h20_suit_dfu_overview.png
   :alt: Overview of the SUIT DFU procedure

   Overview of the SUIT DFU procedure

Requirements
************

For this user guide, the following development kit is required:

+------------------------+----------+--------------------------------+-------------------------------+
| **Hardware platforms** | **PCA**  | **Board name**                 | **Board target**              |
+========================+==========+================================+===============================+
| nRF54H20 DK            | PCA10175 | ``nrf54h20dk``                 | ``nrf54h20dk/nrf54h20/cpuapp``|
+------------------------+----------+--------------------------------+-------------------------------+

Software requirements
---------------------

This guide requires the following software:

* Toolchain Manager - For installing the full |NCS| toolchain.
* Microsoft's |VSC| - The recommended IDE for |NCS|.
* |nRFVSC| - An add-on for |VSC| for developing |NCS| applications.
* nRF Command Line Tools - Mandatory tools for working with |NCS|.
* **suit-generator** - A Python package by Nordic Semiconductor for generating SUIT envelopes and manifests.

Download instructions are in the README file in the `suit-generator`_ repository.

.. _ug_suit_build_system_config:

Build system configuration
**************************

By default the build system generates SUIT envelopes using predefined manifest templates provided by Nordic Semiconductor.
These templates can be found in :file:`modules/lib/suit-generator/ncs`, and are suitable for standard development needs.

Three manifests are used in the most common case:

* Root manifest - :file:`root_with_binary_nordic_top.yaml.jinja2`
* Application domain manifest - :file:`app_envelope.yaml.jinja2`
* Radio domain manifest - :file:`rad_envelope.yaml.jinja2`

.. note::
   The radio domain manifest template is available only for the Bluetooth® Low Energy version of the :ref:`nrf54h_suit_sample`, not the UART version.

The process of building a SUIT envelope (which contains the manifests) can be summarized with the following diagram:

.. figure:: images/nrf54h20_suit_generator_workflow.png
   :alt: Modifying manifest templates workflow

   Modifying manifest templates workflow

Manifest templates (``.jinja2``) files are rendered to YAML files.
These YAML files are used as a representation of the output binary SUIT envelopes.
The provided manifest templates (``.jinja2``) files contain variables (represented as components), such as memory ranges, names, and paths to binaries.
The component values are filled out automatically by the build system during the manifest rendering.

Variables in the provided templates, like memory ranges and paths to binaries, are filled out by the build system.

.. _ug_suit_use_own_manifest:

How to use your own manifest
----------------------------

Changing the source of manifest templates allows you to create manifest files on your own, without relying on the templates provided by Nordic Semiconductor.

The source of the manifest templates can be configured by setting the following Kconfig options:

* ``SB_CONFIG_SUIT_ENVELOPE_ROOT_TEMPLATE``

* :kconfig:option:`CONFIG_SUIT_ENVELOPE_TEMPLATE` - For each of the images (application and radio core images)

One example is demonstrated with the following case:

.. tabs::

    .. group-tab:: Windows

        The provided user-defined manifest templates are stored in ``C:\my_default_templates``.

    .. group-tab:: Linux

        The provided user-defined manifest templates are stored in ``/home/my_user/my_default_templates``.

The following files are used to create the SUIT envelope:

* Root envelope - :file:`root.yaml.jinja2`
* Application domain - :file:`app.yaml.jinja2`
* Radio domain - :file:`radio.yaml.jinja2`

To build the described example with the provided user-defined manifest templates:

.. tabs::

    .. group-tab:: Windows

        Run the following command:

        .. code-block:: console

            west build -d C:/ncs-lcs/work_dir/build/ -b nrf54h20dk/nrf54h20/cpuapp -p -- -DSB_CONFIG_SUIT_ENVELOPE_ROOT_TEMPLATE=\"c:/my_default_templates/root.yaml.jinja2\" -DCONFIG_SUIT_ENVELOPE_TEMPLATE=\"c:/my_default_templates/app.yaml.jinja2\" -Dhci_ipc_CONFIG_SUIT_ENVELOPE_TEMPLATE=\"c:/my_default_templates/radio.yaml.jinja2\"

    .. group-tab:: Linux

        Run the following command:

        .. code-block:: console

            west build -b nrf54h20dk/nrf54h20/cpuapp -p -- -DSB_CONFIG_SUIT_ENVELOPE_ROOT_TEMPLATE=\"/home/my_user/my_default_templates/root.yaml.jinja2\" -DCONFIG_SUIT_ENVELOPE_TEMPLATE=\"/home/my_user/my_default_templates/app.yaml.jinja2\" -Dhci_ipc_CONFIG_SUIT_ENVELOPE_TEMPLATE=\"/home/my_user/my_default_templates/radio.yaml.jinja2\"

For more information about the Kconfig options used in this example, see the `SUIT Kconfig options`_.

.. _ug_suit_customize_uuids:

Customize Vendor and Class UUIDs
--------------------------------

Customizing UUIDs used for class and vendor IDs enhances security and is recommended for specific use cases.
Values for ``class-identifier`` and ``vendor-identifier`` in the manifest are created based on the ``CONFIG_SUIT_MPI_<MANIFEST_ROLE>_VENDOR_NAME`` and ``CONFIG_SUIT_MPI_<MANIFEST_ROLE>_CLASS_NAME`` Kconfig options.
Specifically, in the basic case:

* :kconfig:option:`CONFIG_SUIT_MPI_ROOT_VENDOR_NAME`
* :kconfig:option:`CONFIG_SUIT_MPI_ROOT_CLASS_NAME`
* :kconfig:option:`CONFIG_SUIT_MPI_APP_LOCAL_1_VENDOR_NAME`
* :kconfig:option:`CONFIG_SUIT_MPI_APP_LOCAL_1_CLASS_NAME`
* :kconfig:option:`CONFIG_SUIT_MPI_RAD_LOCAL_1_VENDOR_NAME`
* :kconfig:option:`CONFIG_SUIT_MPI_RAD_LOCAL_1_CLASS_NAME`

These Kconfigs are used during Manifest Provisioning Information (MPI) generation.
After the MPI has been flashed, it is read by the Secure Domain Firmware, which can then use it to verify if the UUIDs in a manifest are correct,

As as an example, after adding the following lines to the :file:`prj.conf` file:

.. code-block::

   CONFIG_SUIT_MPI_APP_LOCAL_1_VENDOR_NAME="ACME Corp"
   CONFIG_SUIT_MPI_APP_LOCAL_1_CLASS_NAME="Light bulb"

You will find the following lines in the generated manifest .yaml file :file:`build/DFU/application.yaml`

.. code-block::

  - suit-directive-override-parameters:
      suit-parameter-vendor-identifier:
         RFC4122_UUID: ACME Corp              # Changed vendor-identifier value
      suit-parameter-class-identifier:
         RFC4122_UUID:                        # Changed class-identifier values
           namespace: ACME Corp
           name: Light bulb

.. _ug_suit_var_methods_in_manifest:

Variables and methods available in the manifest templates
**********************************************************

The manifest templates have access to the following:

* Devicetree values (`edtlib`_ object)
* Target names
* Paths to binary artifacts
* Application version

Some of these values are stored in the Python dictionaries that are named after the target name.
(Therefore, Python is used within the ``.jinja2`` files to fill in the necessary values in the manifest(s).)
For example, for the :ref:`nrf54h_suit_sample` there will be two variables available: ``application`` and ``radio``.
The target names (the names of these variables) can be changed using the :kconfig:option:`CONFIG_SUIT_ENVELOPE_TARGET` Kconfig option for a given image.
Each variable is a Python dictionary type (``dict``) containing the following keys:

* ``name`` - Name of the target
* ``dt`` -  Devicetree representation (`edtlib`_ object)
* ``binary`` - Path to the binary, which holds the firmware for the target

Additionally, the Python dictionary holds a variable called ``version`` that holds the application version.
With the Python dictionary you are able to, for example:

* Extract the CPU ID by using ``application['dt'].label2node['cpu'].unit_addr``
* Obtain the partition address with ``application['dt'].chosen_nodes['zephyr,code-partition']``
* Obtain the size of partition with ``application['dt'].chosen_nodes['zephyr,code-partition'].regs[0].size``
* Get the pair of URI name and the binary path by using ``'#{{ application['name'] }}': {{ application['binary'] }}``
* Get the application version with ``suit-manifest-sequence-number: {{ sysbuild['config']['SB_CONFIG_SUIT_ENVELOPE_SEQUENCE_NUM'] }}``

Additionally, the **get_absolute_address** method is available to recalculate the absolute address of the partition.
With these variables and methods, you can define templates which will next be filled out by the build system and use them to prepare the output binary SUIT envelope.
The examples below demonstrate the use of these variables and methods.

.. _ug_suit_suit_set_comp_def_mem_range:

Set component definition and memory ranges
------------------------------------------

In :file:`modules/lib/suit-generator/ncs/app_envelope.yaml.jinja2`
, the component definition and memory ranges are filled out by using the ``edtlib`` (devicetree values) object like so:

.. code-block::

    suit-components:
    - - MEM
    - ``{{ application['dt'].label2node['cpu'].unit_addr }}``
    - ``{{ get_absolute_address(application['dt'].chosen_nodes['zephyr,code-partition']) }}``
    - ``{{ application['dt'].chosen_nodes['zephyr,code-partition'].regs[0].size }}``

.. note::
   See the :ref:`ug_suit_dfu_component_def` page for a full list and table of the available customizable components.

Set integrated payload
----------------------

In :file:`modules/lib/suit-generator/ncs/app_envelope.yaml.jinja2`
, the integrated payload definition is done using the target name and binary location:

.. code-block::

    suit-integrated-payloads:
    ``'#{{ application['name'] }}': {{ application['binary'] }}``

.. _ug_suit_root_manifest_temp:

Root manifest template
----------------------

The file :file:`modules/lib/suit-generator/ncs/root_with_binary_nordic_top.yaml.jinja2` contains content that is dynamically created, depending on how many targets are built.
The following example only shows a selected portion of the root manifest file.
For more information, see the file available in the sample and `Jinja documentation`_:

.. code-block::

   {%- set component_index = 0 %}                                                  # Initialize the `component_index variable`.
                                                                                   # This variable will be used to assign component indexes dynamically depending on
                                                                                   # How many cores have been built.


   {%- set component_list = [] %}                                                  # Initialize the `component_list variable`.
                                                                                   # This variable will be used to execute `suit-directive-set-component-index` over
                                                                                   # all components, except the first one with index 0.

   SUIT_Envelope_Tagged:
      suit-authentication-wrapper:
         SuitDigest:
           suit-digest-algorithm-id: cose-alg-sha-256
      suit-manifest:
         suit-manifest-version: 1
         suit-manifest-sequence-number: {{ sysbuild['config']['SB_CONFIG_SUIT_ENVELOPE_SEQUENCE_NUM'] }}                              # Assign value defined in the `CONFIG_APP_VERSION` Kconfig option.
         suit-common:
            suit-components:
            - - CAND_MFST
            - 0
   {%- if radio is defined %}                                         # Add section below only, in case the radio core has been already been built.
      {%- set component_index = component_index + 1 %}                             # Increment `component_index`.
      {%- set radio_component_index = component_index %}              # Store the current component index for further use.
      {{- component_list.append( radio_component_index ) or ""}}      # Append the current component index to the common list.
        - - INSTLD_MFST
          - RFC4122_UUID:
              namespace: nordicsemi.com
              name: nRF54H20_sample_rad
   {%- endif %}
   {%- if application is defined %}
   {%- set component_index = component_index + 1 %}
   {%- set app_component_index = component_index %}
   {{- component_list.append( app_component_index ) or ""}}
       - - INSTLD_MFST
         - RFC4122_UUID:
             namespace: nordicsemi.com
             name: nRF54H20_sample_app
   {%- endif %}

.. _ug_suit_ref_for_edit_manifest:

Reference for editing manifest values
-------------------------------------

Some entries in the YAML file will filled in automatically, (upon first build of the sample) by the build system in the final binary DFU envelope.

+---------------------------------------------------------+------------------------------+--------------------------------------------------------+
| Operation                                               | YAML entry                   | Value in the output binary envelope                    |
+=========================================================+==============================+========================================================+
| UUID calculation                                        | RFC4122_UUID:                | ``3f6a3a4dcdfa58c5accef9f584c41124``                   |
|                                                         |    namespace:                |                                                        |
|                                                         |      nordicsemi.com          |                                                        |
|                                                         |    name:                     |                                                        |
|                                                         |      nRF54H20_sample_root    |                                                        |
+---------------------------------------------------------+------------------------------+--------------------------------------------------------+
| Digest calculation for provided file                    | suit-digest-bytes:           | ``<digest value created for application.bin content>`` |
|                                                         |    file: application.bin     |                                                        |
+---------------------------------------------------------+------------------------------+--------------------------------------------------------+
| Image size calculation for provided file                | suit-parameter-image-size:   | ``<size calculated for application.bin content>``      |
|                                                         |    file: application.bin     |                                                        |
+---------------------------------------------------------+------------------------------+--------------------------------------------------------+
| Attaching data to the envelope as an integrated payload | suit-integrated-payloads:    | ``<application.bin binary content>``                   |
|                                                         |    '#application':           |                                                        |
|                                                         |       application.bin        |                                                        |
+---------------------------------------------------------+------------------------------+--------------------------------------------------------+

For more information, see the example YAML files available in :file:`modules/lib/suit-generator/examples/input_files`
.

.. _ug_suit_using_manifest_gen:

How to use the manifest generator
**********************************

The **suit-generator** tool is used by the build system to create and parse SUIT envelopes.
This Python-powered tool can be used as a command-line application, a Python module, or a script.

To use **suit_generator** from the command line:

.. code-block::

   pip install <workspace>/modules/lib/suit-generator
   suit-generator --help
   suit-generator create --input-file input.yaml --output-file envelope.suit
   suit-generator parse --input-file envelope.suit

As a Python module:

.. code-block:: python

   from suit_generator import envelope
   envelope = SuitEnvelope()
   envelope.load('input.yaml')
   envelope.dump('output.suit')

Executing the Python script from the command line:

.. code-block::

   python <workspace>/modules/lib/suit-generator/cli.py create --input-file input.yaml --output-file envelope.suit

.. _ug_suit_edit_build_artifacts:

Edit build artifacts
--------------------

The :ref:`nrf54h_suit_sample` :file:`/build/DFU` directory contains several artifacts related to the SUIT process:

* :file:`./build/DFU/radio.yaml`
* :file:`./build/DFU/application.yaml`
* :file:`./build/DFU/root.yaml`
* :file:`./build/DFU/radio.suit`
* :file:`./build/DFU/application.suit`
* :file:`./build/DFU/root.suit`

These files can be used with the **suit-generator** for various purposes, such as recreating SUIT files, restoring YAML files from a binary SUIT envelope, debugging a SUIT envelope, and converting between different SUIT-related file types.

.. note::
    You must build the sample at least once to make these artifacts available.

Recreate SUIT files
-------------------

To recreate SUIT files:

.. code-block::

   suit-generator create --input-file ./build/DFU/root.yaml --output-file my_new_root.suit

Restore YAML files from a binary SUIT envelope
----------------------------------------------

To restore a YAML file from a binary SUIT envelope:

.. code-block::

   suit-generator parse --input-file ./build/DFU/root.suit --output-file my_new_root.yaml

Debug a SUIT envelope
---------------------

To debug a SUIT envelope, by printing their parsed content to the ``stdout``, run the following:

.. code-block::

   suit-generator parse --input-file ./build/DFU/root.suit

.. note::
   The previous command can be extended by parsing the dependent manifests by calling:

   .. code-block::

      suit-generator parse --input-file ./build/DFU/root.suit --parse-hierarchy

Convert between file types
--------------------------

All mentioned artifacts can be converted back-and-forth, remembering that calculated and resolved YAML entries like UUIDs or files will be presented as a RAW value in the form of HEX strings.

For example, if you have an input entry like the following:

.. code-block::

   suit-parameter-class-identifier:
      RFC4122_UUID:
         namespace: nordicsemi.com
         name: nRF54H20_sample_app

This entry will be presented, after parsing, as the following:

.. code-block::

   suit-parameter-class-identifier:
      raw: 08c1b59955e85fbc9e767bc29ce1b04d

Reference for suit-generator
****************************

Find more information about the **suit-generator** in :file:`modules/lib/suit-generator/README.md` and its documentation.

To build the **suit-generator** documentation:

.. code-block::

   cd <workspace>/modules/lib/suit-generator
   pip install ./
   pip install -r doc/requirements-doc.txt
   sphinx-build -b html doc/source/ doc/build/html
