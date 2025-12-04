
   :name: priv_unpriv_access

   Access memory and peripherals from both privileged and unprivileged mode.

Overview
********
A simple user application that demonstrates access to privileged and unprivileged
memory and peripherals from both privileged and unprivileged mode.

This application can be built into cases of:
1. Only privileged mode access privileged memory and peripherals. (No error)
2. Only privileged mode access unprivileged memory and peripherals. (No error)
3. Only unprivileged mode access unprivileged memory and peripherals. (No error)
4. Only unprivileged mode access privileged memory and peripherals. (Expected error)

Sample Output
=============
.. code-block:: text

    *** Booting nRF Connect SDK v3.1.99-9378d32ffcb0 ***
    *** Using Zephyr OS v4.2.99-dc05376c170f ***
    Initial privilege mode in main thread = 0x2 .
    Firewall: configuring SPU/MPC...
    App thread: trying to access privileged GPIO pins...
    Current Privilege in user_func_gpio = 0x0 .
    App thread: accessed privileged GPIO pins successfully!
    Current Privilege in user_func_sram = 0x0 .
    user_entry: writing unpriv scratch...
    user_entry: writing priv scratch (should fault)...
    App thread: trying to access privileged GPIO pins...
    Current Privilege in user_func_gpio = 0x1 .
    ***** MPU FAULT *****
      Data Access Violation
      MMFAR Address: 0x500d8204
    r0/a1:  0x500d8200  r1/a2:  0x00000001  r2/a3:  0x80000000
    r3/a4:  0x00000002 r12/ip:  0x0000c88f r14/lr:  0x00000c61
     xpsr:  0xa1000000
    Faulting instruction address (r15/pc): 0x00000c66

    [FATAL] reason=19, thread=0x200007e8 (noname)
    [FATAL] Expected fault in user thread – marking as handled
    Current Privilege in user_func_sram = 0x1 .
    user_entry: writing unpriv scratch...
    ***** MPU FAULT *****
      Data Access Violation
      MMFAR Address: 0x2000063c
    r0/a1:  0x0000eac6  r1/a2:  0x00000000  r2/a3:  0x000000aa
    r3/a4:  0x2000063c r12/ip:  0x0000c88f r14/lr:  0x0000c885
     xpsr:  0x21000000
    Faulting instruction address (r15/pc): 0x00000bc4

    [FATAL] reason=19, thread=0x20000740 (noname)
    [FATAL] Expected fault in user thread – marking as handled
    End of main.
