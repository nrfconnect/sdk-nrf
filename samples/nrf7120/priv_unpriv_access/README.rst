.. _nrf7120_priv_unpriv_access:

nRF7120: Privileged and Unprivileged Access
###########################################

.. contents::
   :local:
   :depth: 2

The nRF7120 privileged and unprivileged access sample demonstrates access to memory and peripherals from both privileged and unprivileged mode.

Requirements
************

The sample supports the following development kit:

.. table-from-sample-yaml::

Overview
********
A simple user application that demonstrates access to privileged and unprivileged
memory and peripherals from both privileged(kernel thread) and unprivileged(user thread) mode.

This application can be built into testing cases of:
1. Privileged mode access privileged memory and peripherals. (No error)
2. Privileged mode access unprivileged memory and peripherals. (No error)
3. Unprivileged mode access unprivileged memory and peripherals. (No error)
4. Unprivileged mode access privileged memory and peripherals. (Expected error)

Building and running
********************

.. |sample path| replace:: :file:`samples/nrf7120/priv_unpriv_access`

.. include:: /includes/build_and_run.txt

Testing
=======

After programming the sample to your development kit, complete the following steps to test it:

|test_sample|

#. |connect_kit|
#. |connect_terminal|
#. Observe that the sample prints to the terminal.

Example output of the sample:

    .. code-block:: text

       *** Booting nRF Connect SDK v3.2.99-6c62ca92c0e9 ***
       *** Using Zephyr OS v4.3.99-7b28ebcbea99 ***
       Running TESTSUITE test_suite_priv_settings
       ===================================================================
       Test executed on nrf7120dk/nrf7120/cpuapp
       START - test_case_priv_access_priv_mem
       Current function access_memory is in Privileged mode.
       App thread: accessed privileged memory successfully!
        PASS - test_case_priv_access_priv_mem in 0.011 seconds
       ===================================================================
       START - test_case_priv_access_priv_perip
       Current function access_gpio is in Privileged mode.
       App thread: accessed GPIO pins successfully!
        PASS - test_case_priv_access_priv_perip in 0.011 seconds
       ===================================================================
       START - test_case_unpriv_access_priv_mem
       We should expect fault in this test case...
       Current function access_memory is in UnPrivileged mode.
       E: ***** MPU FAULT *****
       E:   Data Access Violation
       E:   MMFAR Address: 0x20001000
       E: r0/a1:  0x00000038  r1/a2:  0x0001163a  r2/a3:  0x000000aa
       E: r3/a4:  0x20001000 r12/ip:  0x0000ed53 r14/lr:  0x00000d7d
       E:  xpsr:  0x21000000
       E: Faulting instruction address (r15/pc): 0x00000d82
       E: >>> ZEPHYR FATAL ERROR 0: CPU exception on CPU 0
       E: Current thread: 0x20002320 (unknown)

       [FATAL] reason=0, thread=0x20002320 ()
       Fault expected: true , Fault reason allow: 0
       [FATAL] Expected fault in user thread – marking as handled
        PASS - test_case_unpriv_access_priv_mem in 2.005 seconds
       ===================================================================
