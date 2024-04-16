.. _test_framework:

Test framework
##############

The |NCS| provides support for writing tests using the following methods:

* Zephyr's own :ref:`zephyr:test-framework` (Ztest).
  This method is adopted in the |NCS| by :ref:`crypto_test`, which you can check as reference.
* The |NCS|'s method based on Unity and CMock.
  Read :ref:`ug_unity_testing` for more information.

.. toctree::
   :maxdepth: 2
   :caption: Subpages:

   testing_unity_cmock
