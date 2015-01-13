<?php

// Copyright 2013 Tera Insights, LLC. All Rights Reserved.

/*
 * This file is used for Makefile generation.
 */

namespace grokit {

    function replaceExtension ($val, $oldExt, $newExt) {
        $oldExt = preg_quote($oldExt);
        $pattern = '/' . $oldExt . '$/';
        return preg_replace($pattern, $newExt, $val);
    }


    // Tries to get the value from the container with the key (or property) $key, and returns
    // $default if it doesn't exist.
    function getDefault( &$container, $key, $default ) {
        if( is_array($container) ) {
            if( array_key_exists($key, $container) ) {
                return $container[$key];
            }
        }
        else if( is_object($container) ) {
            if( property_exists($container, $key) ) {
                return $container->$key;
            }
        }
        else {
            grokit_logic_error('Tried to use object of type ' . gettype($container) . ' as a container in ' . __FUNCTION__ );
        }

        return $default;
    }

    // Tries to get the specified value from the container, and errors out if it does
    // not exits.
    function getNoDefault( &$container, $key ) {
        if( is_array($container) ) {
            if( array_key_exists($key, $container) ) {
                return $container[$key];
            }
        }
        else if( is_object($container) ) {
            if( property_exists($container, $key) ) {
                return $container->$key;
            }
        }
        else {
            grokit_logic_error('Tried to use object of type ' . gettype($container) . ' as a container in ' . __FUNCTION__ );
        }

        grokit_logic_error('Tried to get non-existant property \'' . $key . '\' from container');
    }

    function generateMakefile( GenerationInfo $genInfo, $options ) {
        ob_start();

        // Set up directories to copy the sources to and copy them
        $sourcesBaseDir = realpath("../Sources");
        $filesPerJob = $genInfo->filesPerJob();
        foreach( $filesPerJob as $job => $files ) {
            $cDir = $sourcesBaseDir . DIRECTORY_SEPARATOR . $job;
            if (!file_exists( $cDir ) )
                mkdir($cDir, 0777, true);

            foreach( $files as $file ) {
                $cFile = $cDir . DIRECTORY_SEPARATOR . $file;
                copy($file, $cFile);
            }
        }

        // General options
        $generalOptions = getDefault($options, 'general', []);

        $debug = getDefault($generalOptions, 'debug', false);

        // Get C++ Compiler information from options
        $cxxOptions = getDefault($options, 'c++', []);

        $cxxCompiler = getDefault($cxxOptions, 'compiler', 'g++');
        $cxxStandard = getDefault($cxxOptions, 'standard', 'c++11');
        $cxxStdLib = getDefault($cxxOptions, 'stdlib', '');
        $cxxStdLib = \strlen($cxxStdLib) > 0 ? "-stdlib=$cxxStdLib" : '';
        $flags = getDefault($cxxOptions, 'flags', [ '-fPIC', '-g', '-D_FILE_OFFSET_BITS=64', '-DDEBUG']);
        $cxxFlags = implode(' ', $flags);
        $linkFlags = implode(' ', getDefault($cxxOptions, 'linkflags', []));

        $opts = getDefault($cxxOptions, 'optimizations', ['-O3', '-march=native', '-ffast-math']);
        $cxxOpts = $debug ? '-Wall' : implode(' ', $opts);

        $includePaths = explode(':', getenv('GROKIT_HEADER_PATH'));
        $cxxIncludeFlags = implode(' ', array_map(function($val) { return '-I '. $val; }, $includePaths ));

        // Get needed information from genInfo
        $ccFiles = $genInfo->files();
        $oFiles = array_map(function($val) {
            // Replace ending .cc with .o
            return replaceExtension($val, '.cc', '.o');
        }, $ccFiles);

        $libs = $genInfo->libs();

        $cxxLibs = implode(' ', array_map(function($val) { return '-l' . $val; }, $libs));
?>
CXX             := <?=$cxxCompiler?>

CXXOPTS         := <?=$cxxOpts?>

CXXFLAGS        := -std=<?=$cxxStandard?> <?=$cxxStdLib?> <?=$cxxFlags?>

CXXINCLUDE      := <?=$cxxIncludeFlags?>

CXXLIBS         := <?=$cxxLibs?>

CXXLINKFLAGS    := <?=$linkFlags?>

# If CPUPROFILE set, link with gperftools CPU profiler
ifdef CPUPROFILE
CXXLIBS += -lprofiler
endif

Generated.so: <?=implode(' ', $oFiles)?>

	$(CXX) -rdynamic -shared -o Generated.so $(CXXFLAGS) $(CXXOPTS) <?=implode(' ', $oFiles)?> $(CXXLINKFLAGS) $(CXXLIBS)

clean:
	rm <?=implode(' ', $oFiles)?>

<?
        foreach( $ccFiles as $ccFile ) {
            $oFile = replaceExtension($ccFile, '.cc', '.o');
?>
<?=$oFile?> : <?=$ccFile?>

	$(CXX) $(CXXFLAGS) $(CXXOPTS) $(CXXINCLUDE) -c <?=$ccFile?> -o <?=$oFile?>


<?
        } // end foreach ccFile

        file_put_contents('./Makefile', ob_get_clean());
    }
}

?>
