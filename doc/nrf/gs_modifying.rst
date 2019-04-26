.. _gs_modifying:

Modifying a sample application
##############################

After programming and testing a sample application, you probably want to make some modifications to the application, for example, change compilation options or add your own files with additional functionality.

Such changes must be included in the :file:`CMakeList.txt` file for the sample application.
Instead of doing the changes manually in this file, you can use |SES| (SES) to  manage the project.


Adding files
************

To add a file to a project, right-click **Project 'app/libapp.a'** in the Project Explorer.
Select either **Add new file to CMakeLists.txt** to create a file and add it or **Add existing file to CMakeLists.txt** to add a file that already exists.

.. figure:: images/ses_add_files.png
   :alt: Adding files in SES

   Adding files in SES


Configuring compiler settings
*****************************

To add compiler options, application defines, or include directories, right-click **Project 'app/libapp.a'** in the Project Explorer and select **Edit Compile Options in CMakeLists.txt**.

In the window that is displayed, you can define compilation options for the project.

.. figure:: images/ses_compile_options.png
   :alt:

   Setting compiler defines, includes, and options in SES

.. note::
   These compiler settings apply to the application project only.
   To manage Zephyr and other subsystems, go to **Project -> Configure nRF Connect SDK Project**.


Requirements for CMakeLists.txt
*******************************

To be able to manage :file:`CMakeLists.txt` with SES, the CMake commands that are specific to the |NCS| application must be marked so SES can identify them.
Therefore, they must be surrounded by ``# NORDIC SDK APP START`` and ``# NORDIC SDK APP END`` tags.

The following CMake commands can be managed by SES, if they target the ``app`` library:

    - ``target_sources``
    - ``target_compile_definitions``
    - ``target_include_directories``
    - ``target_compile_options``

The :file:`CMakeLists.txt` files for the sample applications in the |NCS| are tagged as required.
Typically, they include at least the :file:`main.c` file as source::

   # NORDIC SDK APP START
   target_sources(app PRIVATE src/main.c)
   # NORDIC SDK APP END
