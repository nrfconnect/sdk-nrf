#######################
HPF Assembly Management
#######################

Assembly Management is required to achieve following requirements defined in event handling:

- Modularity
- Testability


Rationale
=========

During a build, Hard Real-Time (HRT) functions will be compiled to assembly code.
This code will be checked against previous version of the assembly code that was verified with HW characteristic tests.
If the difference is none, or deemed small enough, new code is considered timing accurate as well.
Application can be build in two modes: user or developer mode. In user mode, application will use pre-generated assembly file to build HRT part. In developer mode application will use newly-generated assembly file.
By storing assembly files, it is possible to review changes introduced both by developer and toolchain.

Implementation
==============

Assembly management is implemented in build-time CMake scripts.
In order to use it, application's CMakeLists.txt has to include ``sdp_assembly_install(app "${CMAKE_SOURCE_DIR}/file.c)``.
Where ``file.c`` is C file containing only HRT functions.

This allows following CMake functions to be used:

- ``sdp_assembly_generate``
- ``sdp_assembly_check``
- ``sdp_assembly_prepare_install``
- ``sdp_assembly_target_sources``

sdp_assembly_generate
---------------------

This function adds a build target that generates a temporary ``file-soc-temp.s`` file in the FLPR application's build directory when ``ninja build`` is invoked.
``soc`` is the SoC name for current build, such as ``nrf54l15``.

sdp_assembly_check
------------------

This function adds a build target that is called when ``ninja build`` is invoked, after one from ``sdk_assembly_generate``.
It checks whether ``file-soc-temp.s`` file in build directory is identical to ``file-soc.s`` file in source directory (where ``file.c`` is located).
If there is a difference between those files, a CMake error (in user mode) or warning (in developer mode) is generated.
In user mode, developer should verify if changes between ``file-soc-temp.s`` and ``file-soc.s`` can be deemed small enough to be considered timing accurate.
If yes, ``ninja asm_install`` should be called in the FLPR application's build directory.
Otherwise, code needs to be adjusted or HW characterisation tests need to be rerun.
In developer mode, ``file-soc-temps.s`` is included in the build. However, the file is not replaced in the source directory, to do that ``ninja asm_install`` should be called in the FLPR application's build directory.

sdp_assembly_prepare_install
----------------------------

This function adds a build target that replaces ``file-soc.s`` in source directory with ``file-soc-temp.s`` from build directory.
It is invoked when developer calls ``ninja asm_install`` in the FLPR application's build directory.

sdp_assembly_target_sources
---------------------------

This function adds ``file-soc.s`` from source directory to target sources.

Flow
====

Flow of build-time assembly management is presented below:

.. raw:: html
    :file: ./hpf_asm.drawio.html
