.. _custom_ltk_test:

nRF Custom LTK Test
###################

.. contents::
   :local:
   :depth: 2

This test code verifies custom LTK feature implemented in ``host_extensions.h``.

Test Cases
**********

LTK test ``custom_ltk.sh``

Purpose: verify that custom LTK can be set and used to establish secure connection.

Test procedure:
    1. Peripheral device starts connectable advertising.
    2. Central device starts scanning and initiates connection.
    3. Connection is established and both sides set custom LTK key in the connection callback, authentication flag set to false.
    4. Central device initiates security update to level BT_SECURITY_L2, both sides successfully finish security update
       with level set to BT_SECURITY_L2.
    5. Central initiates disconnect and both sides disconnect successfully.
    6. The steps above repeated two more times with authenticated flag set to true and security level elevated to
       level BT_SECURITY_L4.

Expected result: connected devices can establish secure connection using provided custom LTK.


LTK coexistence test ``custom_ltk_coex.sh``

Purpose: verify that custom LTK for secure connections coexist with normal SMP workflow.

Test procedure:
    1. Enable passkey authentication on both devices.
    2. Peripheral device starts connectable advertising.
    3. Central device starts scanning and initiates connection.
    4. Connection is established and both sides set custom LTK key in the connection callback with authentication flag enabled.
    5. Central device initiates security update to level BT_SECURITY_L4, both sides successfully finish security update
       bypassing passkey authentication and with level set to BT_SECURITY_L4.
    6. Central initiates disconnect and both sides disconnect successfully.
    7. Central and peripheral connect again, this time no LTK is set.
    8. Central device initiates security update to level BT_SECURITY_L4, which completes successfully on both sides using
       passkey confirmation procedure, with established security level being BT_SECURITY_L4.
    9. Central initiates disconnect and both sides disconnect successfully.
    10. Clear all pairing information on both sides.
    11. Repeat steps 7-9 with bondable flag being set for the connection.

Expected result: security updates using custom LTK can coexist with SMP authentication mechanism.


LTK invalid test ``custom_ltk_invalid.sh``

Purpose: verify that not setting or setting mismatching LTK keys prevents secure connection setup.

Test procedure:
    1. Peripheral device starts connectable advertising.
    2. Central device starts scanning and initiates connection.
    3. Connection is established, with peripheral device enabling LTK for the connection.
    4. Central device initiates security update to level BT_SECURITY_L2, but the security update fails on both sides.
    5. Central initiates disconnect and both sides disconnect successfully.
    6. The procedure above is repeated with different LTK sets on central and peripheral.

Building and running
********************

These tests are run as part of nRF Connect SDK CI with specific configurations.

For more information about BabbleSim tests, see the :ref:`documentation in Zephyr <zephyr:bsim>`.
