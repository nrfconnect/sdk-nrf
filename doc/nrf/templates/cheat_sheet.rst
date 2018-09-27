:orphan:

.. _doc_cheatsheet:

Cheat Sheet
###########

This topic is not meant to be included in the output, but to serve as reference for common RST syntax and linking between doc sets.

RST syntax
**********

Simple Markup
=============

*italics* **bold** ``code``

howtoemphasize\ **inside**\ aword

:file:`name` or ``name``

:command:`make`

Lists
=====

* This is a bulleted list.
* Another item.

  #. This is a list.
  #. More.
* One more.


#. text

   - bullet
   - bullet
#. text
#. text

   a. different numbering
   #. another item


Definition list
===============

term (up to a line of text)
   Definition of the term, which must be indented

   and can even consist of multiple paragraphs

next term
   Description.

Indentation
===========

Lorem ipsum dolor sit amet, consectetur adipiscing elit. Duis elementum odio vel viverra malesuada. Cras aliquet libero non iaculis rhoncus.

  Sed pulvinar augue a cursus mollis. Vivamus nec feugiat augue, sed vehicula
  purus. Fusce feugiat rutrum dui ut convallis. Lorem ipsum dolor sit amet,
  consectetur adipiscing elit. Lorem ipsum dolor sit amet, consectetur
  adipiscing elit.

Pellentesque nec metus vitae felis porta accumsan eget non ipsum.

| Etiam lacinia, sapien quis eleifend posuere,
| urna eros porttitor lorem,
| vitae faucibus libero diam sed nunc.
| Maecenas vitae ornare est.
| Phasellus nec molestie elit.

Nam hendrerit ex pretium, facilisis lectus sit amet, semper eros::

  Curabitur at nulla velit. Ut turpis nunc, porttitor eu laoreet in, venenatis at quam. Morbi at nisl id nibh laoreet cursus ac vitae massa.

.. _tables:

Tables
======

.. _table:

+------------------------+------------+----------+----------+
| Header row, column 1   | Header 2   | Header 3 | Header 4 |
| (header rows optional) |            |          |          |
+========================+============+==========+==========+
| body row 1, column 1   | column 2   | column 3 | column 4 |
+------------------------+------------+----------+----------+
| body row 2             | ...        | ...      |          |
+------------------------+------------+----------+----------+

We can create more complex tables as ASCII art:

+------------------------+------------+----------+----------+
| Header row, column 1   | Header 2   | Header 3 | Header 4 |
| (header rows optional) |            |          |          |
+========================+============+==========+==========+
| body row 1, column 1 lsjdlfkj       | column 3 | column 4 |
+------------------------+------------+----------+----------+
| body row 2             | ...        | ...      | test     |
+------------------------+------------+----------+ spanning +
| body row 3             | ...        | ...      |          |
+------------------------+------------+----------+----------+

.. table:: Truth table for "not"
   :widths: auto

   =====  =====
     A    not A
   =====  =====
   False  True
   True   False
   =====  =====

.. list-table:: Frozen Delights!
   :widths: 15 10 30
   :header-rows: 1

   * - Treat
     - Quantity
     - Description
   * - Albatross
     - 2.99
     - On a stick!
   * - Crunchy Frog
     - 1.49
     - If we took the bones out, it wouldn't be
       crunchy, now would it?
   * - Gannet Ripple
     - 1.99
     - On a stick!

.. csv-table:: Frozen Delights!
   :header: "Treat", "Quantity", "Description"
   :widths: 15, 10, 30

   "Albatross", 2.99, "On a stick!"
   "Crunchy Frog", 1.49, "If we took the bones out, it wouldn't be
   crunchy, now would it?"
   "Gannet Ripple", 1.99, "On a stick!"


Links
=====

Nulla aliquam lacinia risus `eget congue <http://www.nordicsemi.com>`_. Morbi posuere a nisi id `vulputate`_. Curabitur ac lacus magna. Aliquam urna lacus, ornare non iaculis vel, ullamcorper vel dolor. Nunc vel dui vulputate, imperdiet metus eget, tristique dui. Etiam non lorem vel magna dictum aliquam. Nunc hendrerit facilisis diam, non mollis leo commodo nec.

.. _vulputate: http://www.nordicsemi.com

www.nordicsemi.com http://www.nordicsemi.com

:ref:`tables`

`override <tables>`_

:ref:`Overriding link text <table>`

Use :ref: to link to IDs and :doc: to link to files.


Footnotes
=========

Integer ac ex lacinia [Ref1]_, tristique odio vitae, ullamcorper enim. Mauris aliquet rutrum justo eget dapibus. Nunc nec justo a nulla tristique accumsan. Integer sit amet porta mauris, quis aliquet [Ref2]_ justo. Ut et nulla interdum, facilisis lacus eu, consectetur libero. Cras faucibus ut turpis eu aliquam. Etiam feugiat, urna a lobortis aliquet, ex enim [Ref1]_ varius felis, et porta arcu felis vel odio. Lorem ipsum dolor sit amet, consectetur adipiscing elit. Integer consectetur vel elit vel fringilla. Nam eu ultricies leo, a volutpat erat. Nam tempor dolor vel consequat aliquam. Maecenas ornare condimentum mattis. Aliquam tellus tortor, varius ac iaculis ut, tincidunt quis quam.

.. [Ref1] Footnote 1
.. [Ref2] Footnote 2

Variables
=========

Curabitur nisl |sapien|, posuere auctor metus et, convallis varius turpis. Sed elementum rhoncus |sapien|, dictum posuere lectus.

.. |sapien| replace:: some Latin word

Linking between doc sets
************************

Read out an objects.inv file::

   python3 -m sphinx.ext.intersphinx objects.inv

Doxygen
=======

.. doxygenfunction:: event_manager_init
   :project: nrf

See `the breathe documentation <https://breathe.readthedocs.io/en/latest/directives.html#directives>`_ for information about what you can link to.

Linked RST project
==================

:doc:`zephyr:index` - link to a page

:ref:`zephyr:getting_started` - link to an ID
