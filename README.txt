BrainSuite16a1
-------------------------------------------------------------------------------
Copyright and licenses for the files in this distribution are provided in
the file LICENSE.txt. This distribution also includes third party files, which
are described in the file ThirdPartyLicenses.txt.

This distribution contains the files necessary to build BrainSuite16a1 and
associated command line tools.

Please visit http://brainsuite.org/download/ to download the official binary
builds or for more information on the BrainSuite project. Please visit our
forum at http://forums.brainsuite.org/ with any questions regarding building
the software.

Requirements
------------
C++11 compiler
We have built the official releases using the following compilers on the
following platforms:

Mac: clang, Apple LLVM version 7.0.0 (clang-700.0.72) (included with XCode)
Windows: Microsoft Visual C++ 2013 compiler
Linux: clang 3.4

We recommend using these compilers to ensure cross-platform consistency.

Qt Toolkit
The Qt toolkit is available from: http://www.qt.io/developers/
The official BrainSuite16a1 builds use version 5.6.1 of the Qt Toolkit.

zlib: http://www.zlib.net/
zlib v. 1.2.8 is included in the 3rd Party directory.


Mac & Linux
-----------
You will need zlib installed on your system. If you do not already have it,
you may either download a new copy from zlib: http://www.zlib.net/ or use
the one in the 3rdParty directory. Follow the build instructions for your
particular machine. You will also need GNU make.

The command line tools and GUI can be built separately in either order.

Command Line:
From the command line, navigate to the corticalextraction directory. Run the
script ./buildall.sh to build the software. This will create a set of binaries
in the directory corticalextraction/bin/$MACHTYPE, where $MACHTYPE is the
system variable describing the type of machine on which you are compiling
(e.g., x86_64-apple-darwin14, x86_64-pc-linux-gnu). You can also run

./buildall.sh install

to install these files to your home directory, ~/BrainSuite16a1/bin/$MACHTYPE.
You may want to move these to ~/BrainSuite16a1/bin/ if you are not sharing your
home directory on multiple machines.

The diffusion tools can be built in the same manner. Navigate to the diffusion
directory and run

./buildall.sh install

which will build odfmax, htrack, conmat, and dwisplit.

BrainSuite16a1 GUI:
The GUI may be built using QtCreator. Open the BrainSuite16a1.pro file, located
in the BrainSuite16a1 directory.

Windows
-----------
Command Line:
Open the corticalextraction.sln file in Microsoft Visual Studio (VS2013).
Build the libraries first, then the command line tools.
To build the diffusion tools, open diffusion.sln in Microsoft Visual Studio.
You must have already built the libraries in corticalextraction.sln.

BrainSuite16a1 GUI:
The GUI may be built using QtCreator. Open the BrainSuite16a1.pro file, located
in the BrainSuite16a1 directory. The required zlib libraries must first be
built using Microsoft Visual C++. Make sure you build the correct version for
the type of build you are making in Qt, e.g., 32-bit vs. 64-bit, MD vs. MT,
Release vs Debug. Qt is dynamically linked, so build the MD versions of zlib.

CerebroHelper
-------------
The BrainSuite GUI calls a command line program named CerebroHelper. CerebroHelper
uses the AIR libraries, which are available from the UCLA Brain Mapping Center
website:

http://www.bmap.ucla.edu/portfolio/software/AIR/

Please note that those source files are under the AIR software license. We have
included the source code for cerebrohelper in the corticalextraction/cerebrohelper 
directory. You will need to first download and build AIR. The cerebrohelper.cpp file
will need to be compiled using the AIR header files and linked with libRPW.a.

To avoid platform and compiler differences, build the AIR libraries and cerebrohelper 
using the clang compiler or Microsoft Visual Studio C++.

For example, if AIR has been extracted and built at /path/to/AIR5.3.0/, then this command:

clang++ cerebrohelper.cpp -I /path/to/AIR5.3.0/ -L /path/to/AIR5.3.0/src -lRPW -o cerebrohelper

will build cerebrohelper on Mac or Linux. 

