.. _hpf_assembly_management:

Assembly management
###################

Assembly Management is a critical component in the development process, particularly when meeting the modularity and testability requirements defined in the event handling.

Overview
********

During the compilation process, Hard Real-Time (HRT) functions are compiled into assembly code.
This assembly code is then compared to a previously verified version that has undergone hardware characteristic tests.
If the differences between the current and previous versions are within acceptable limits, the new code is considered to maintain timing accuracy.

The application can be built in two modes: user mode and developer mode.
In user mode, the application uses a pre-generated assembly file to construct the HRT component, while in developer mode, the application employs a newly generated assembly file.
This approach not only facilitates the integration of updates but also allows for a thorough review of changes made by developers and modifications introduced by the toolchain.

See following flow of the build-time assembly management process:

.. IMG TBA

Implementation
**************

Assembly management is integrated into the build process through CMake scripts.
To use this feature, you must include the command ``sdp_assembly_install(app "${CMAKE_SOURCE_DIR}/file.c")`` in your application's :file:`CMakeLists.txt` file, where ``file.c`` value is the C file containing only Hard Real-Time (HRT) functions.

You can manage assembly files with the following functions:

* ``sdp_assembly_generate``
* ``sdp_assembly_check``
* ``sdp_assembly_prepare_install``
* ``sdp_assembly_target_sources``

sdp_assembly_generate
=====================

The :c:func:`sdp_assembly_generate` function creates a build target that generates a temporary assembly file.
Tis file is named :file:`file-soc-temp.s` and is located in the Fast Lightweight Peripheral Processor (FLPR) application's build directory when ``ninja build`` is invoked.
``soc`` represents the System on Chip (SoC) name for the current build, such as ``nrf54l15``.

sdp_assembly_check
==================

The :c:func:`sdp_assembly_check` function establishes a build target that is triggered when executing ``ninja build``, following the :c:func:`sdk_assembly_generate` function.
It verifies whether the :file:`file-soc-temp.s` file in the build directory matches the :file:`file-soc.s` file in the source directory.
If discrepancies are found, a CMake error (in user mode) or warning (in developer mode) is generated.
In user mode, you should assess whether the differences between :file:`file-soc-temp.s` and :file:`file-soc.s` are minimal enough to be considered timing accurate.
If they are, you must execute the ``ninja asm_install`` command in the FLPR application's build directory.
If not, you need to adjust the code or re-run hardware characterization tests.

In developer mode, :file:`file-soc-temp.s` is included in the build, but it is not automatically replaced in the source directory.
To update the source directory, invoke the ``ninja asm_install`` command.

sdp_assembly_prepare_install
============================

The :c:func:`sdp_assembly_prepare_install` function creates a build target that replaces the :file:`file-soc.s` file in the source directory with :file:`file-soc-temp.s` from the build directory.
It is activated when executing the ``ninja asm_install`` command in the FLPR application's build directory.

sdp_assembly_target_sources
===========================

The :c:func:`sdp_assembly_target_sources` function includes the :fil:`file-soc.s` from source directory in the target sources.
