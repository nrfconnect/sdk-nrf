.. _customservice_readme:

Custom service template
#######################

The XYZ Service is a custom service that does something.

.. tip::
   Explain what the service does in a few sentences.
   Focus on the main idea, not the details.
   More detailed information can be given in the sections for the characteristics of the service.

The XYZ Service is used in the **insert link** sample.

Service UUID
************

The 128-bit service UUID is **UUID**.

Characteristics
***************

This service has **X** characteristics.

Name of the characteristic (UUID)
=================================

.. note::
   Add a subsection for each characteristic, and list the properties.
   Do not list properties that are not relevant.

Write
   This is what happens when you write to the characteristic.

Write Without Response
   * Write any data to the characteristic to do something.
   * Write 0 bytes to the characteristic to do something else.

Read
   The read operation returns 3*4 bytes (12 bytes) that contain sth:

   * 4 bytes unsigned: Bla
   * 4 bytes unsigned: Bla2
   * 4 bytes unsigned: Bla3

Notify
   Information about what happens.

API documentation
*****************

.. note::
   Remove the code block and replace with the name of the doxygen group that contains the API.

.. code-block:: python

   .. doxygengroup:: doxygen_group_name
      :project: nrf
