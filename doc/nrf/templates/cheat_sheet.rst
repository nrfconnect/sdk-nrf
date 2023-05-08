:orphan:

.. _doc_cheatsheet:

Cheat Sheet
###########

.. contents::
   :local:
   :depth: 2

This topic is not meant to be included in the output, but to serve as reference for common RST syntax and linking between doc sets.

RST syntax
**********

Simple Markup
=============

*italics* **bold** ``code``

howtoemphasize\ **inside**\ aword

:file:`name` or ``name``

:command:`make`

:guilabel:`Install`

:makevar:`ZEPHYR_MBEDTLS_NRF_MODULE_DIR` (this is a name of the make variable)

:envvar:`$HOME` (this is an environment variable)

Lists
=====

* This is a bulleted list.
* Another item.

  #. This is a list.
  #. More.
* One more.


#. text

   * bullet
   * bullet

#. text
#. text

   a. different numbering
   #. another item

Diagrams
========

You can include Message Sequence Chart (MSC) diagrams in RST by using the ``.. msc::`` directive and including the MSC content, using the syntax described for example here: http://www.mcternan.me.uk/mscgen/.
Use validator at https://mscgen.js.org/ for syntax check.
Example:

.. code-block:: none

   .. msc::
       hscale = "1.3";
       Module,Application;
       Module<<Application      [label="nrf_cloud_connect() returns successfully"];
       Module>>Application      [label="NRF_CLOUD_EVT_TRANSPORT_CONNECTED"];
       Module>>Application      [label="NRF_CLOUD_EVT_USER_ASSOCIATION_REQUEST"];
       Module<<Application      [label="nrf_cloud_user_associate()"];
       Module>>Application      [label="NRF_CLOUD_EVT_USER_ASSOCIATED"];
       Module>>Application      [label="NRF_CLOUD_EVT_READY"];
       Module>>Application      [label="NRF_CLOUD_EVT_TRANSPORT_DISCONNECTED"];


This will generate the following output:

   .. msc::
       hscale = "1.3";
       Module,Application;
       Module<<Application      [label="nrf_cloud_connect() returns successfully"];
       Module>>Application      [label="NRF_CLOUD_EVT_TRANSPORT_CONNECTED"];
       Module>>Application      [label="NRF_CLOUD_EVT_USER_ASSOCIATION_REQUEST"];
       Module<<Application      [label="nrf_cloud_user_associate()"];
       Module>>Application      [label="NRF_CLOUD_EVT_USER_ASSOCIATED"];
       Module>>Application      [label="NRF_CLOUD_EVT_READY"];
       Module>>Application      [label="NRF_CLOUD_EVT_TRANSPORT_DISCONNECTED"];


Definition list
===============

term (up to a line of text)
   Definition of the term, which must be indented

   and can even consist of multiple paragraphs

next term
   Description.

Glossary
--------

See :ref:`glossary` for usage.

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
   * - Line-break example
     - x
     - | First line
       | Second line

.. csv-table:: Frozen Delights!
   :header: "Treat", "Quantity", "Description"
   :widths: 15, 10, 30

   "Albatross", 2.99, "On a stick!"
   "Crunchy Frog", 1.49, "If we took the bones out, it wouldn't be
   crunchy, now would it?"
   "Gannet Ripple", 1.99, "On a stick!"

To force line-breaks in a table, use a line block:

| First line
| Second line

Tabs
====

Example usage of tabs is to show how the same procedure works on different operating systems:

.. tabs::

   .. group-tab:: Windows

      Windows tab

   .. group-tab:: Linux

      Linux tab

   .. group-tab:: macOS

      macOS tab

Toggle
======

The ``.. toggle::`` directive lets you create expandable sections.
Useful to hide content that is potentially irrelevant to many customers.

.. toggle::

   Lorem ipsum dolor sit amet, consectetur adipisicing elit, sed do eiusmod tempor incididunt ut labore et dolore magna aliqua.
   Ut enim ad minim veniam, quis nostrud exercitation ullamco laboris nisi ut aliquip ex ea commodo consequat.
   Duis aute irure dolor in reprehenderit in voluptate velit esse cillum dolore eu fugiat nulla pariatur.
   Excepteur sint occaecat cupidatat non proident, sunt in culpa qui officia deserunt mollit anim id est laborum.

Embedding media
===============

This is how you can embed a video:

.. raw:: html

   <h1 align="center">
   <iframe width="560" height="315" src="https://www.youtube.com/embed/9Ar13rMxGIk" title="Getting started with Matter" frameborder="0" allow="accelerometer; autoplay; clipboard-write; encrypted-media; gyroscope; picture-in-picture; web-share" allowfullscreen></iframe>
   </h1>

In general, the ``.. raw:: html`` directive will process any HTML code provided in it.

Links
=====

Nulla aliquam lacinia risus `eget congue <https://www.nordicsemi.com>`_. Morbi posuere a nisi id `vulputate`_. Curabitur ac lacus magna. Aliquam urna lacus, ornare non iaculis vel, ullamcorper vel dolor. Nunc vel dui vulputate, imperdiet metus eget, tristique dui. Etiam non lorem vel magna dictum aliquam. Nunc hendrerit facilisis diam, non mollis leo commodo nec.

.. _vulputate: https://www.nordicsemi.com

www.nordicsemi.com https://www.nordicsemi.com

:ref:`tables`

`override <tables_>`_

:ref:`Overriding link text <table>`

Use :ref: to link to IDs and :doc: to link to files.

If you have a link ID in :file:`links.txt` that consists of only one word, you cannot have a heading anywhere that is the same.
If you do, you'll get an error about a duplicate ID in :file:`links.txt`.

Notes and similar elements
==========================

.. note::
   This is a note.

.. tip::
   This is a tip.

.. important::
   This is important stuff.

.. caution::
   This is a caution note.
   Use for things like security issues or to prevent major flaws in the application.

.. warning::
   This is a warning.
   Do not overuse this.
   Use only to warn against device bricking, unrecoverable issues, or potential injury (when describing hardware).

Images
======

.. figure:: /images/ncs-toolchain.svg
   :alt: Alt text

   Figure caption

Make sure to correctly specify the path to the image file.
A slash at the beginning of the path indicates an absolute path and will link to the common image folder in :file:`doc/nrf`.
Starting the path without the forward slash indicates a relative path.

Math equations
==============

.. math::

   (a + b)^2 = a^2 + 2ab + b^2

Footnotes
=========

Integer ac ex lacinia [Ref1]_, tristique odio vitae, ullamcorper enim. Mauris aliquet rutrum justo eget dapibus. Nunc nec justo a nulla tristique accumsan. Integer sit amet porta mauris, quis aliquet [Ref2]_ justo. Ut et nulla interdum, facilisis lacus eu, consectetur libero. Cras faucibus ut turpis eu aliquam. Etiam feugiat, urna a lobortis aliquet, ex enim [Ref1]_ varius felis, et porta arcu felis vel odio. Lorem ipsum dolor sit amet, consectetur adipiscing elit. Integer consectetur vel elit vel fringilla. Nam eu ultricies leo, a volutpat erat. Nam tempor dolor vel consequat aliquam. Maecenas ornare condimentum mattis. Aliquam tellus tortor, varius ac iaculis ut, tincidunt quis quam.

.. [Ref1] Footnote 1
.. [Ref2] Footnote 2

Variables
=========

Curabitur nisl |sapien|, posuere auctor metus et, convallis varius turpis. Sed elementum rhoncus |sapien|, dictum posuere lectus.

.. |sapien| replace:: some Latin word

Reuse
*****

To reuse text from the same or another file in the same doc set:

Include 1:
  .. include:: cheat_sheet.rst
     :start-line: 19
     :end-line: 27

Include 2:
  .. include:: ../getting_started/installing.rst
     :start-after: west-error-start
     :end-before: west-error-end

To reuse text from another doc set:

Include 3:
  .. ncs-include:: develop/getting_started/index.rst
     :docset: zephyr
     :auto-dedent:
     :start-line: 6
     :end-line: 16

Include 4:
  .. ncs-include:: nfc/doc/type_2_tag.rst
     :docset: nrfxlib
     :auto-dedent:
     :start-after: Version 1.0*.
     :end-before: If you use the supplied library,

You can also use ncs-include if you want to use the indentation options inside the nrf doc set:

Include 5 (similar to include 2, but improved indentation):
  .. ncs-include:: ../getting_started/installing.rst
     :start-after: west-error-start
     :end-before: west-error-end
     :auto-dedent:

See https://github.com/nrfconnect/sdk-nrf/commit/fa5bd7330538f6a12e059c9d60fa2696e48fcf3a for implementation and usage.

.. tip::
   If you need a "start-after" text that occurs more than once inside a document, you can combine ``:start-after:`` with ``:start-line:``.
   Sphinx will then use the first occurrence of the "start-after" text after the specified start line.

Shortcuts
=========

Defined in the :file:`shortcuts.txt` file.
Usage: |NCS|

They can be used for replacing single words but also for multiple paragraphs.

Including text inside a nested list
===================================

List 1:

#. Step A1

Including the first three sub-list items of **2. Step A2** in the second list

.. include_startingpoint

2. Step A2

   a. substepA1
   #. substepA2
   #. substepA3

.. include_endpoint

3. Step A3
4. Step A4

Another list which includes the steps from the previous list:

#. Step B1

.. include:: cheat_sheet.rst
   :start-after: include_startingpoint
   :end-before: include_endpoint
..

   d. substepB4
   #. substepB5


Including code snippets in RST
******************************

To include a code snippet from a code file in an .rst file, you need to edit both.

Defining snippet in code file
=============================

Use RST tags in the comments to enclose the code you want to become a snippet.

Use the following tag formatting::

* Starting tag: /** .. include_startingpoint_<name of file where the snippet goes>_rst_<number of snippet within the .c file> */
* Ending tag: /** .. include_endpoint_<name of file where the snippet goes>_rst_<number of snippet within the .c file> */

For example::

    /** .. include_startingpoint_scan_rst_1 */
        static void scan_filter_no_match(struct bt_scan_device_info *device_info,
                     bool connectable)
    {
        struct bt_conn *conn;
        char addr[BT_ADDR_LE_STR_LEN];
        if (device_info->adv_info.adv_type == BT_LE_ADV_DIRECT_IND) {
            bt_addr_le_to_str(device_info->addr, addr, sizeof(addr));
            printk("Direct advertising received from %s\n", addr);
            bt_scan_stop();

            conn = bt_conn_create_le(device_info->addr,
                         device_info->conn_param);

            if (conn) {
                default_conn = bt_conn_ref(conn);
                bt_conn_unref(conn);
            }
        }
    }
    /** .. include_endpoint_scan_rst_1 */


Including snippet in .rst
=========================

To include the snippet you defined in a code file into an .rst file, use the following RST syntax:

.. parsed-literal::
   :class: highlight

   .\. literalinclude:: <path to the code file with the snippet>
      :language: <coding language, for example 'c', 'python', 'ruby'>
      :start-after: <opening tag of the snippet>
      :end-before: <closing tag of the snippet>

For example:

.. parsed-literal::
   :class: highlight

   .\. literalinclude:: ../../samples/bluetooth/central_hids/src/main.c
       :language: c
       :start-after: include_startingpoint_scan_rst_1
       :end-before: include_endpoint_scan_rst_1

Code blocks
===========

To manually type a block of code, you can use ``code-block``:

  .. code-block:: console

     python buildprog.py -c app -b debug -d both

Linking between doc sets
************************

Read out an objects.inv file::

   python3 -m sphinx.ext.intersphinx objects.inv

Doxygen
=======

We usually include doxygen groups::

   .. doxygengroup:: group_name
      :project: nrf
      :members:

See `the breathe documentation <https://breathe.readthedocs.io/en/latest/directives.html#directives>`_ for information about what you can link to.

To link to doxygen macros, enums or functions use:

* :c:macro:`BT_HIDS_INFORMATION_LEN`
* :c:func:`bt_hids_init`
* :c:struct:`bt_gatt_dm`
* :c:type:`slm_data_handler_t`
* :c:enum:`at_param_type` (for the whole list)
* :c:enumerator:`PEER_STATE_CONN_FAILED` (for a list item)
* :c:member:`ble_peer_event.state`

For C++ elements:

* :cpp:func:`mbedtls_entropy_init`
* :cpp:type:`bt_mesh_time_tai`
* :cpp:enum:`bt_mesh_time_role`
* :cpp:member:`request_msg_recv`

Kconfig
=======

Link to Kconfig options using :kconfig:option:`CONFIG_NORDIC_SECURITY_BACKEND`.

Linked RST project
==================

:doc:`zephyr:index` - link to a page

:ref:`zephyr:getting_started` - link to an ID

Heading lvl 4
-------------

Lower hierarchy section.

Heading lvl 5
~~~~~~~~~~~~~

Even lower hierarchy section.

Numbered sections
*****************

This is a special type of headings that will be automatically turned into steps of a large procedure.
For usage example, see :ref:`gs_installing`.

.. rst-class:: numbered-step

First step
==========

Content of first numbered heading.

.. rst-class:: numbered-step

Second step
===========

Content of second numbered heading.

.. rst-class:: numbered-step

Third step
==========

Content of third numbered heading.
