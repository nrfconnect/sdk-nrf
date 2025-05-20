.. _test_framework:

Test framework
##############

The |NCS| provides support for writing tests using the following methods:

* Zephyr's native :ref:`zephyr:test-framework` (Ztest).
  This framework has features specific to the Zephyr RTOS, such as test scaffolding and setup or teardown functions.
  Ztest in the |NCS| by :ref:`crypto_test`, which you can check as reference.
* The |NCS|'s framework based on Unity and CMock.
  Read :ref:`ug_unity_testing` for more information.

You can run these tests using either west or Twister, as described in :ref:`running_unit_tests`.

.. toctree::
   :maxdepth: 2
   :caption: Subpages:

   testing_unity_cmock
   running_unit_tests
