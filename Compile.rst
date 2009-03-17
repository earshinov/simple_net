Compilation
===========

Build requirements
~~~~~~~~~~~~~~~~~~

* cmake (http://cmake.org/) version >= 2.6
* a C++ compiler (tested with Visual C++ 9 and gcc 4.3.2)

Compilation itself
~~~~~~~~~~~~~~~~~~

To compile run the following commands from the command line in the directory
containing this file.

If you use a Unix-like system:
------------------------------

::

  mkdir build
  cd build
  cmake ..
  make

If you use Windows and Visual Studio:
-------------------------------------

::

  mkdir build
  cd build
  cmake -G <GENERATOR>

Replace ``<GENERATOR>`` with appropriate Visual Studio projects' generator from `this list <http://www.cmake.org/cmake/help/cmake2.6docs.html#section_Generators>`_.

A solution file will be placed in the ``build`` directory. Open it with Visual Studio.

You may also generate a zip archive containing binaries.
Just compile in the Release mode and run ``cpack`` within the ``build`` directory.
