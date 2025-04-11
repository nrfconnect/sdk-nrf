.. _sw-arch-coprocessor-examples-hpf-fault-handling:

##############
Fault handling
##############

Host fault
==========

When Host crashes and is reset, handling of VPR state depends on architecture:

* On Lumos platform, hardware ensures that when Application core (Host) is reset, HPF FW (FLPR) will be reset as well.
* On Haltium platform, Secure Domain Firmware will reset all resources assigned to HPF FW's (FLPR's) owner (Host).

This means that there is no risk of two cores getting out of sync.

HPF FW fault
============

When HPF FW crashes, this must be signalled to Host. As there is no HW resource that could handle it, solution needs to be done in SW.

There are two possible solutions:

#. Create a hardware watchdog timer

   * Use TIMER peripheral as a watchdog. TIMER is set up by the Host and should be cleared periodically by HPF FW. If HPF FW fails to clear the TIMER, the Host receives an IRQ signalling an HPF FW error.
   * Use WDT peripheral with STOP functionality. HPF FW has to feed the watchdog. If it fails to do so, the Host is configured to receive an interrupt and has to invoke TASK_STOP. This solution assumes host is always able to invoke TASK_STOP within 2 32kHz cycles. Otherwise, the WDT triggers a reset, which may be an unwanted side effect.

#. Implement a trap handler that will signal the condition to Host (for example, using VEVIF IRQ and a predefined location in memory to store error code).

#. Create a flag in shared memory, which is periodically updated by HPF FW and checked by Host.

Those solutions can be combined as well.
Main disadvantage for using only second solution is not handling a situation where HPF FW is stuck in a while loop. Possibly that could be avoided by strict inspection of code.
