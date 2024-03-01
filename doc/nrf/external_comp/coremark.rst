.. _ug_coremark:

CoreMark integration
####################

.. contents::
   :local:
   :depth: 2

CoreMark® is a CPU benchmark provided by Embedded Microprocessor Benchmark Consortium (EEMBC) to verify SoCs performance.
You can use it to perform series of tests in each benchmark iteration.
The benchmark score is calculated based on the number of iterations per second the :term:`Device Under Test (DUT)` can perform.
Integrating CoreMark into the |NCS| allows users to easily verify SoCs performance.

Integration prerequisites
*************************

Before you start the |NCS| integration with CoreMark, make sure that the following prerequisites are completed:

* :ref:`Installation of the nRF Connect SDK <installation>`.
* Setup of the supported :term:`Development Kit (DK)`.

Solution architecture
*********************

Adding the CoreMark repository to the |NCS| manifest automatically includes it in the repository as the |NCS| module.
You can find it under the :file:`modules/benchmark/coremark` path.
CoreMark needs a porting layer to run correctly on a SoC.
You can find the files of porting layer for devices supported in the |NCS| under the :file:`nrf/modules/coremark` path.

CoreMark benchmark consists of continuously repeated iterations.
Each iteration, in turn, consists of the following workloads:

* Linked list
* Matrix multiply
* State machine

For detailed description of each of those workloads, see the `CoreMark GitHub`_ repository.

Integration steps
*****************

To integrate CoreMark into the |NCS|, configure CoreMark as described in the following section.

Configuring CoreMark
====================

To include CoreMark in your build, complete the following steps:

#. Add the following Kconfig options in your :file:`prj.conf` file:

   .. code-block:: console

      CONFIG_COREMARK=y

#. Choose any of the following memory methods for dealing with the CoreMark data.
   Make sure that you allocate enough memory, depending on the method you choose.

   * Memory method stack:

     .. code-block:: console

        COREMARK_MEMORY_METHOD_STACK=y
        CONFIG_MAIN_STACK_SIZE=4096

   * Memory method heap:

     .. code-block:: console

        CONFIG_COREMARK_MEMORY_METHOD_MALLOC=y
        CONFIG_HEAP_MEM_POOL_SIZE=4096

   * Memory method static:

     .. code-block:: console

        CONFIG_COREMARK_MEMORY_METHOD_STATIC=y

To see an example, refer to the :ref:`coremark_configuration` section of the :ref:`coremark_sample` documentation.

Submitting score
****************

You can see the submitted scores in the official `CoreMark <CoreMark®_>`_ database.
If you want to submit your own score, follow steps in the `CoreMark GitHub`_ repository.

Applications and samples
************************

The following sample demonstrates the CoreMark integration in the |NCS|:

* :ref:`coremark_sample` - This sample is used to obtain the CoreMark score for boards supported in the |NCS|.
