DataPath requires a few libraries in order to compile and run. Below is a list
of the current libraries required and locations they can be downloaded from:

LEMON Graph Library
Website:    http://lemon.cs.elte.hu/trac/lemon
Version:    1.2.3

ANTLR Parser Generator
Website:    http://www.antlr.org/
Version:    3.4
Note:       For ANTLR, we require that a driver program called antlr3 be
            available. Some distributions (e.g. Fedora) already provide such
            a driver. If your distribution does not provide a driver, or
            you are installing ANTLR yourself, please create an executable
            script called antlr3 somewhere in the shell's search path that
            when invoked will launch ANTLR with the given parameters.

ANTLR C Runtime
Website:    http://www.antlr.org/download/C
Version:    3.4

SQLite
Website:    http://www.sqlite.org/
Version:    3

================================== pkg-config ==================================

DataPath relies upon pkg-config to determine if required libraries are
installed, and to retrieve linking information for them. Any shared libraries
need to have pkg-config metadata files available. Some libraries (such as LEMON)
will install the needed meta-data files during installation. Unfortunately, not
all libraries will do this. Fortunately, the meta-data files are very simple
and can easily be created for libraries missing them. Examples of these files
can be found in the pkgconfig directory. Just create a file called <library>.pc
with the same structure as the examples, and place it in a directory within
pkg-config's search path (which is configurable using the PKG_CONFIG_PATH
environment variable).

For more information about pkg-config, view the guide at:
http://people.freedesktop.org/~dbn/pkg-config-guide.html

============================== Compiling DataPath ==============================

Once all of the required libraries have been installed and the correct meta-data
files placed in the proper locations, you can compile DataPath by going into the
`src` directory and executing:

    ./compile.datapath.sh
