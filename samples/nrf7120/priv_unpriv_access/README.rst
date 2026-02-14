
   :name: priv_unpriv_access

   Access memory and peripherals from both privileged and unprivileged mode.

Overview
********
A simple user application that demonstrates access to privileged and unprivileged
memory and peripherals from both privileged(kernel thread) and unprivileged(user thread) mode.

This application can be built into cases of:
1. Privileged mode access privileged memory and peripherals. (No error)
2. Privileged mode access unprivileged memory and peripherals. (No error)
3. Unprivileged mode access unprivileged memory and peripherals. (No error)
4. Unprivileged mode access privileged memory and peripherals. (Expected error)

Sample Output
=============
.. code-block:: text

    START - test_case_priv_access_priv_perip
    Current Privilege in access_gpio = 0x0 .
    App thread: accessed GPIO pins successfully!
     PASS - test_case_priv_access_priv_perip in 0.010 seconds
    ===================================================================
    START - test_case_unpriv_access_priv_mem
    We should expect fault in this test case...
    Current Privilege in access_memory = 0x1 .
    E: ***** MPU FAULT *****
    E:   Data Access Violation
    E:   MMFAR Address: 0x20001000
    E: r0/a1:  0x0000002b  r1/a2:  0x000112aa  r2/a3:  0x000000aa
    E: r3/a4:  0x20001000 r12/ip:  0x0000eaa1 r14/lr:  0x00000de9
    E:  xpsr:  0x21000000
    E: Faulting instruction address (r15/pc): 0x00000dee
    E: >>> ZEPHYR FATAL ERROR 0: CPU exception on CPU 0
    E: Current thread: 0x20002320 (unknown)

    [FATAL] reason=0, thread=0x20002320 ()
    Fault expected: true , Fault reason allow: 0
    [FATAL] Expected fault in user thread – marking as handled
     PASS - test_case_unpriv_access_priv_mem in 2.005 seconds
    ===================================================================
    START - test_case_unpriv_access_priv_perip
    We should expect fault in this test case...
    Current Privilege in access_gpio = 0x1 .
    E: thread 0x200023e8 (5) does not have permission on gpio driver 0xfd88
    E: permission bitmap
    E: 00 00                   |..
    E: syscall z_vrfy_gpio_port_set_bits_raw failed check: access denied
    E: r0/a1:  0x00000000  r1/a2:  0x00000000  r2/a3:  0x00000000
    E: r3/a4:  0x00000000 r12/ip:  0x00000000 r14/lr:  0x00000000
    E:  xpsr:  0x00000000
    E: Faulting instruction address (r15/pc): 0x00000000
    E: >>> ZEPHYR FATAL ERROR 3: Kernel oops on CPU 0
    E: Current thread: 0x200023e8 (unknown)

    [FATAL] reason=3, thread=0x200023e8 ()
    Fault expected: true , Fault reason allow: 3
    [FATAL] Expected fault in user thread – marking as handled
