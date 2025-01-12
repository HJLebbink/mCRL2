Release and packaging procedure
===============================

Versioning scheme
-----------------

mCRL2 version numbers are standardised to three fields: ``YYYYMM.B.RRRRR``.
Here ``YYYY`` is the year, ``MM`` the month of the last release. ``B`` is the
bugfix number. ``B = 0`` for the planned (initial) release of a version.
``RRRRR`` is the SVN revision number.

For an official release, the tools will show the version ``YYYYMM.B``,
development versions will show all three fields. This enables quick
identification of the version you are dealing with by inspecting the output of
:option:`--version`. When the toolset is built from an SVN checkout with
modified sources, the revision number is appended with ``M``, such that the
:option:`--version` output shows that the checkout was not clean.

Version numbers are computed in :file:`scripts/MCRL2Version.cmake`. This is the
*only* place where the main version (``YYYYMM.B``) can be defined. The rest of
the source code *should not* contain hardcoded version numbers.

Creating an official release
----------------------------

This section discusses the steps that need to be taken for creating an official
mCRL2 release.

Creating release branch
^^^^^^^^^^^^^^^^^^^^^^^

The first step to take is creating a release branch from ``trunk`` once the
trunk is deemed stable::

  $ svn delete -m"Drop old release branch in preparation of new release" https://svn.win.tue.nl/repos/MCRL2/branches/release
  $ svn copy -m"Cut release branch from trunk" https://svn.win.tue.nl/repos/MCRL2/trunk https://svn.win.tue.nl/repos/MCRL2/branches/release
  
Testing
^^^^^^^

.. warning::

   Before starting with testing, make sure that there is no other mCRL2
   installation in your ``PATH``!

For all testing, we assume that the release branch is checked out in ``/tmp``::

  $ cd /tmp
  $ svn checkout https://svn.win.tue.nl/repos/MCRL2/branches/release mcrl2-release

Before the actual release is tagged, the release branch must be tested on all
supported platforms::

* Windows 32-bit
* Windows 64-bit
* Linux 32-bit
* Linux 64-bit
* Mac OS X 32-bit
* Mac OS X 64-bit


Testing is done by running (a platform specific variation of)::

  $ mkdir mcrl2-release-build
  $ cd mcrl2-release-build
  $ cmake ../mcrl2-release -DCMAKE_INSTALL_PREFIX="/tmp/mcrl2-release-build/install" \
                           -DCMAKE_BUILD_TYPE="Release" \
                           -DMCRL2_ENABLE_TEST_TARGETS="ON" \
                           -DMCRL2_ENABLE_EXPERIMENTAL="ON" \
                           -DMCRL2_ENABLE_DEPRECATED="ON" \
                           -DMCRL2_ENABLE_RELEASE_TEST_TARGETS="ON" \
                           -DMCRL2_ENABLE_RANDOM_TEST_TARGETS="ON"
  $ make install
  $ ctest
  $ mkdir mcrl2-debug-build
  $ cd mcrl2-debug-build
  $ cmake ../mcrl2-release -DCMAKE_INSTALL_PREFIX="/tmp/mcrl2-debug-build/install" \
                           -DCMAKE_BUILD_TYPE="Debug" \
                           -DMCRL2_ENABLE_TEST_TARGETS="ON" \
                           -DMCRL2_ENABLE_EXPERIMENTAL="ON" \
                           -DMCRL2_ENABLE_DEPRECATED="ON" \
                           -DMCRL2_ENABLE_RELEASE_TEST_TARGETS="ON" \
                           -DMCRL2_ENABLE_RANDOM_TEST_TARGETS="ON"
  $ make install
  $ ctest
  

  
Furthermore, integration with CADP and LTSmin must be tested on Linux:

*CADP support*

  .. warning::
  
    When running the following commands, make sure that the environment variables
    for CADP have been set properly; assuming that CADP is installed in
    ``<cadp>`` this can be done using the following commands assuming a 64-bit
    system ::
    
      $ export CADP="<cadp>"
      $ export PATH=$PATH:$CADP/com:$CADP/bin.x64

  Run the following commands::
  
    $ cd /tmp
    $ mkdir mcrl2-cadp-build
    $ cd mcrl2-cadp-build
    $ cmake /tmp/mcrl2-release -DCMAKE_INSTALL_PREFIX="/tmp/mcrl2-cadp-build/install" \
                             -DBUILD_SHARED_LIBS=OFF \
                             -DMCRL2_ENABLE_CADP_SUPPORT=ON \
                             -DMCRL2_CADP_INSTALL_PATH=$CADP
    $ make install
    $ ctest
    $ install/bin/mcrl22lps ../mcrl2-release/examples/academic/abp/abp.mcrl2 abp.lps
    $ install/bin/lps2lts abp.lps abp.bcg
    $ install/bin/ltsinfo abp.bcg
    $ install/bin/ltsconvert -ebisim abp.bcg abp.bisim.bcg
    $ install/bin/ltsinfo abp.bisim.bcg
    $ install/bin/ltsconvert abp.bcg abp.aut
    $ install/bin/ltsconvert -labp.lps abp.bcg abp.lts
    $ install/bin/ltsconvert abp.bcg abp.fsm
    
*LTSmin support*

  Run the following commands::
  
    $ mkdir /tmp/mcrl2-ltsmin-build
    $ cd /tmp/mcrl2-ltsmin-build
    $ cmake ../mcrl2-release -DCMAKE_INSTALL_PREFIX="/tmp/mcrl2-ltsmin-build/install"
    $ make install
    $ cd /tmp
    $ git clone http://fmt.cs.utwente.nl/tools/scm/ltsmin.git ltsmin
    $ cd ltsmin
    $ git checkout -b main origin/maint
    $ git submodule update --init
    $ ./ltsminreconf
    $ ./configure --disable-dependency-tracking --with-mcrl2=/tmp/mcrl2-ltsmin-build/install --prefix=/tmp/ltsmin/install
    $ make install
    $ /tmp/mcrl2-ltsmin-build/install/bin/mcrl22lps /tmp/mcrl2-release/examples/academic/abp/abp.mcrl2 abp.lps
    $ export PATH=/tmp/mcrl2-ltsmin-build/install/bin:$PATH
    $ export LD_LIBRARY_PATH=/tmp/mcrl2-ltsmin-build/install/lib/mcrl2:$LD_LIBRARY_PATH
    $ /tmp/ltsmin/install/bin/lps-reach abp.lps
    $ /tmp/ltsmin/install/bin/lps2lts-grey abp.lps
    $ /tmp/ltsmin/install/bin/lps2lts-gsea abp.lps
    
Updating release number and copyright information
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

* In ``branches/release`` and ``trunk`` update the version number in the file
  ``scripts/MCRL2Version.cmake`` by updating the value assigned to the CMake
  variable ``MCRL2_MAJOR_VERSION``.
  
* In ``branches/release`` and ``trunk`` update the copyright period in the file
  ``COPYING``.
  
* Commit the changes to both ``branches/release`` and ``trunk``.
  
Tagging
^^^^^^^

Once the above procedure has been carried out, and all test have succeeded,
it is time to tag the release::

  $ svn copy https://svn.win.tue.nl/repos/MCRL2/branches/release https://svn.win.tue.nl/repos/MCRL2/tags/mcrl2-VERSION

Source release
^^^^^^^^^^^^^^

The source package for the mCRL2 release is generated as follows::

  $ cd /tmp
  $ svn checkout https://svn.win.tue.nl/repos/MCRL2/tags/mcrl2-VERSION
  $ mkdir mcrl2-package
  $ cd mcrl2-package
  $ cmake ../mcrl2-VERSION -DMCRL2_PACKAGE_RELEASE="ON"
  $ make package_source
  
Upload the source package::

  $ scp mcrl2-VERSION.tar.gz mcrl2@www.win.tue.nl:www/download/release
  
Debian/Ubuntu packages
^^^^^^^^^^^^^^^^^^^^^^

Check out the Debian packaging files for mCRL2 and get the mCRL2 sources::

  $ cd /tmp
  $ svn checkout https://svn.win.tue.nl/repos/MCRL2/packaging
  $ cd packaging/mcrl2
  $ wget http://www.mcrl2.org/download/release/mcrl2-VERSION.tar.gz
  $ mv mcrl2-VERSION.tar.gz mcrl2_VERSION.orig.tar.gz
  $ cd trunk

Check whether the build instructions in ``debian/rules`` are up to date, and
generate a version number (e.g. for an Ubuntu Oneiric PPA build)::

  $ dch -v VERSION-0ubuntu0+1~oneiric -D oneiric
  
.. note::

   It is assumed that the e-mail address listed in the ``dch`` changelog has an
   associated PGP key, that is used to sign the package with ``debuild``. This key
   also needs to be registered with LaunchPad if you want to upload to the PPA.

.. note::

   The changes are saved in the changelog in the MCRL2/packaging repository. To
   save the changelog for a next release, commit the changes to the repository 
   after running ``dch -v VERSION -D unstable``.

Build the source package::

  $ debuild -S -sa

Build the ``.deb`` locally (the ``--buildresult .`` argument makes sure that the
resulting ``.deb`` file is stored in the current directory, omitting it should make
``pbuilder-dist`` put it in ``/var/cache/pbuilder/result``, but on some systems the
file simply disappears::

  $ cd ..
  $ pbuilder-dist oneiric build mcrl2_VERSION-0ubuntu0+1~oneiric.dsc --buildresult .

.. note::

   To use ``pbuilder-dist``, it has to be set up for each distribution you want to
   build for. This can be done by executing::

     $ pbuilder create --debootstrapopts --variant=buildd

Check the generated debian package::

  $ lintian -i mcrl2_VERSION-0ubuntu0+1~oneiric_amd64.deb

Test the generated package by installing it and running some executables. See whether
the graphical applications run correctly and display the right icons. When you have
convinced yourself that the package is good to be distributed, run ``dch`` and 
``debuild`` for every Ubuntu release that is still supported, thus generating a number
of ``.changes`` files. Then upload them to the PPA as follows::

  $ dput ppa:mcrl2/release-ppa mcrl2_VERSION-1ubuntu1ppa1~RELEASENAME.changes

Windows installer
^^^^^^^^^^^^^^^^^

First check out ``tags/mcrl2-VERSION`` using a subversion client, assume to
``mcrl2-VERSION``.

The following can be used as a ``.bat`` file to build the package on 64-bit
Windows::

  call "C:\Program Files\Microsoft SDKs\Windows\v7.1\Bin\SetEnv.Cmd" /Debug /x64 /win7
  set PATH=%PATH%;"C:\Program Files (x86)\CMake 2.8\bin";"C:\Program Files\SlikSvn\bin"
  mkdir package
  cd package
  cmake ..\mcrl2-VERSION -G "NMake Makefiles" -DSVNVERSION:FILEPATH="C:\Program Files\SlikSvn\bin\svnversion" -DSVNCOMMAND:FILEPATH="C:\Program Files\SlikSvn\bin\svn" -DBUILDNAME=MSVC9-Win7-X64-Release -DBOOST_ROOT:PATH="C:\Projects\boost_1_48_0" -DMCRL2_ENABLE_DEPRECATED=ON -DMCRL2_ENABLE_EXPERIMENTAL=ON -DCMAKE_BUILD_TYPE:STRING="Release" -DMCRL2_PACKAGE_RELEASE=ON
  cpack -G NSIS

An alternative way to create a Windows installer, used on a 32bit system::

  cmake . -DBOOST_ROOT=D:/build/boost/boost_1_47 -DMCRL2_PACKAGE_RELEASE=ON
  cmake --build . --config Release --target ALL_BUILD
  cpack -G NSIS

Upload the installer that has been generated to
``http://www.mcrl2.org/download/release``.

Mac OS-X installer for 10.5+
^^^^^^^^^^^^^^^^^^^^^^^^^^^^

First check out ``tags/mcrl2-VERSION`` using a subversion client, assume to
``mcrl2-VERSION``. Also make sure you have PackageMaker installed. This program
is no longer part of the standard Xcode distribution, so should be installed
separately.

Figure out which deployment target you want to compile for. For Mavericks, this
would be 10.9. Bundles compiled for older versions of OSX can usually be run on
newer versions too, but this needs some double checking. Next, find out where
the SDK for this target is located (under Mavericks, it can be found inside the
Xcode executable, for instance
``/Applications/Xcode.app/Contents/Developr/Platforms/MacOSX.platform/Developr/SDKs/MacOSX10.9.sdk``
for the Mavericks SDK).

Then configure cmake::

  $ cmake . -DCMAKE_OSX_DEPLOYMENT_TARGET={SDK version (e.g. '10.9')} \
            -DCMAKE_OSX_SYSROOT={/Path/To/SDK} \
            -DCMAKE_INSTALL_PREFIX=/ \
            -DMCRL2_PACKAGE_RELEASE=ON

As of the 201409.0 release, only the x64 platform is supported (this used to be
i386). No additional CMake variables should have to be set for this if you are
working in a 64-bit version of OSX.

Build the toolset::

  $ make 

Create the DMG-installer::

  $ cpack -G DragNDrop

Upload the disk image that has been generated to
``http://www.mcrl2.org/download/release``.

Checking the installers
^^^^^^^^^^^^^^^^^^^^^^^

Double check that the installers that have been built, as well as a build from
the source tarball succeed!

Updating the website
^^^^^^^^^^^^^^^^^^^^

* Add the old release to :ref:`historic_releases`
* Update the file :file:`downloads-release.inc`
* Generate and upload the new homepage::
  
    $ make doc
    $ rsync -rz --delete doc/sphinx/html/* mcrl2@www:~/www/release


Creating a development snapshot
-------------------------------

Source tarball
^^^^^^^^^^^^^^

A development snapshot can be created by::

  $ svn checkout https://svn.win.tue.nl/repos/MCRL2/trunk trunk
  $ mkdir build
  $ cd build
  $ cmake ../trunk
  $ make package_source
  
This command sequence will generate a source tarball in
:file:`build/mcrl2-VERSION.REVISION.tar.gz`, where ``VERSION`` is the major
version of mCRL2, and REVISION is the revision from which the snapshot was
created.
  
.. warning::

   Do not generate a source tarball using ``svn export``. This will not record
   the information of the SVN revision from which the export was made.

Debian/Ubuntu package
^^^^^^^^^^^^^^^^^^^^^

The building of Debian and Ubuntu packages is based on the source tarball.
Create a directory for the purpose of packaging, say ``ubuntu``, and copy the
tarball to it. Then unpack the tarbal and get the debian configuration. For this
example, we use the source package :file:`mcrl2-201107.1.10245.tar.gz`::

  $ cp mcrl2-201107.1.10245.tar.gz ubuntu
  $ cd ubuntu
  $ cp mcrl2-201107.1.10245.tar.gz mcrl2_201107.1.10245.orig.tar.gz
  $ tar -zxvf mcrl2-201107.1.10245.tar.gz
  $ svn export https://svn.win.tue.nl/repos/MCRL2/packaging/mcrl2/trunk/debian mcrl2-201107.1.10245/debian
  $ cd mcrl2-201107.1.10245

.. warning::

   Note the ``_`` in the name of the file with the ``.orig.tar.gz`` extension.
   This is important!

Now, we update the changelog entry, here we have some different possibilities.

If you are creating a package for debian::
 
  $ dch -v 201107.1.10245-0 -D unstable
  
If you are creating a package for ubuntu (say Ubuntu Oneiric)::

  $ dch -v 201107.1.10245-0ubuntu0 -D oneiric
  
If you are creating a package that should be uploaded to the Ubuntu mCRL2 
development ppa (https://launchpad.net/~mcrl2/+archive/devel-ppa), and you
are building for Ubuntu Oneiric. This allows you to generate the pacage for
multiple Ubuntu versions in the same PPA::

  $ dch -v 201107.1.10245-0ubuntu0+1~oneiric -D oneiric
  
Update the changelog entry to contain the string "Snapshot release" as first
line, and save the changelog.

A source package can now be generated using::

  $ debuild -S -sa
  
This will also sign the source package using your PGP key.

You can now build the package locally (assuming build for Ubunto Oneiric)::

  $ cd ..
  $ pbuilder-dist oneiric build mcrl2-201107.1.10245-0ubuntu0+1~oneiric.dsc
  
This will build the package in a clean environment. As an alternative, you can
upload the package to the mCRL2 development PPA on launchpad using::

  $ dput ppa:mcrl2/devel-ppa mcrl2-201107.1.10245-0ubuntu0+1~oneiric_source.changes




