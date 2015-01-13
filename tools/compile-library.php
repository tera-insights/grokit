#!/usr/bin/php
<?

/* Copyright 2013, Tera Insights LLC, All Rights Reserved */

/*

This program compiles a library (organized into a set of directories)
into a "library".  The library consists of a large php file that, when
included, will "learn" all there is to know about the library.

 */

// list of abstractions to look for
$abstractions = array( 'Type', "GF", "GI", "GIST",
    "GLA", "GT", "UDF" );

function handleInclude($filePath, $ns, $fp) {
    if( !is_file($filePath) )
        return;

    $fileName = pathinfo($filePath, PATHINFO_FILENAME);
    $ext = pathinfo($filePath, PATHINFO_EXTENSION);

    if( $ext == 'php' ) {
        if( !checkFile($filePath, $ns) ) {
            return;
        }

        fwrite($fp, file_get_contents($filePath));
    }
    else if( $ext == 'h' || $ext == 'hh' || $ext == 'hpp' ) {
        $contents = file_get_contents($filePath);
        $info = [ 'kind' => 'HEADER' ];

        // Scan the contents for includes and get the files included
        $sysReg = "/#include[[:space:]]*<([^>]+)>/";
        $userReg = "/#include[[:space:]]*\"([^\"]+)\"/";
        $sysMatch = [];
        $userMatch = [];

        preg_match_all($sysReg, $contents, $sysMatch);
        preg_match_all($userReg, $contents, $userMatch);

        $sysHeaders = [];
        $userHeaders =[];
        foreach( $sysMatch[1] as $match ) {
            $sysHeaders[] = $match;
        }
        foreach( $userMatch[1] as $match ) {
            $userHeaders[] = $match;
        }

        $info['system_headers'] = $sysHeaders;
        $info['lib_headers'] = $userHeaders;

        // Now replace them with blank strings
        $contents = preg_replace($sysReg, '', $contents);
        $contents = preg_replace($userReg, '', $contents);

        // Get a printable version of the info array to return from the
        // function.
        $infoPrint = var_export($info, true);

        // Close current namespace and open $ns\include
        // Also start function
        fwrite($fp, <<<EOT
<?
} // namespace {$ns}

namespace {$ns}\includes {

function {$fileName}() {
    \$gContent = '';
    ob_start();
?>

EOT
);
        fwrite($fp, $contents);

        // Close include namespace and reopen previous one
        fwrite($fp, <<<EOT
<?
    \$gContent .= ob_get_clean();
    \$info = {$infoPrint};
    \$info['global_content'] = \$gContent;
    return \$info;
} // function {$fileName}

} // namespace {$ns}\includes

namespace {$ns} {
?>

EOT
);
    }
}

function checkFile($filePath, $ns) {
    // Create a temporary file to use for checking.
    $tmpname = tempnam('/tmp', 'checklib');
    if( $tmpname === false ) {
        echo "Failed to create temporary file" . PHP_EOL;
        exit;
    }
    $tempfile = fopen($tmpname, 'w');

    // Add the namespace and extra functions
    fwrite($tempfile, <<<EOT
<?php
namespace {$ns} {
?>

EOT
);

    // Output the contents of the file
    fwrite($tempfile, file_get_contents($filePath));

    // End the namespace
    fwrite($tempfile, <<<EOT
<?php
} // namespace {$ns}
?>

EOT
);

    // Close the temporary file
    fclose($tempfile);

    exec("php -l $tmpname", $ignore, $ret);

    // Delete temporary file
    unlink($tmpname);

    if( $ret != 0 ) {
        // Red, bold exclamation marks
        $exclam = "\033[0;31m\033[1m!!\033[0m";
        echo $exclam . "  Syntax checking for $filePath failed." . PHP_EOL;
        return false;
    }

    return true;
}

function compileLibrary($dirName, $output_dir){
    global $abstractions;

    if (!is_dir($dirName)){
        echo "Skipping $dirName since not a directory\n";
        return;
    }

    echo "Building library $dirName.php\n";
    $fp = fopen($output_dir . DIRECTORY_SEPARATOR . $dirName . '.php', 'w');

    /* Write preamble of the namespace */
    fwrite($fp, <<<EOT
<?
namespace {$dirName}{

EOT
);


    fwrite($fp, <<<'EOT'
// need to redefine some functions to make sure we can track namespaces
  function declareFunction($name, $args, $call){ return declareFunctionGlobal(__NAMESPACE__, $name, $args, $call); }
  function declareType($name, $tName, $tArgs) { return declareTypeGlobal(__NAMESPACE__, $name, $tName, $tArgs); }
?>

EOT
);

    if( is_dir($dirName . '/' . 'include') ) {
        echo "Scanning includes directory" . PHP_EOL;
        $files = scandir($dirName . '/' . 'include');
        foreach( $files as $id=>$f ) {
            $fullName = $dirName . '/include/' . $f;
            handleInclude($fullName, $dirName, $fp);
        }
    }

    if( is_dir($dirName . '/' . 'libs') ) {
        $soDir = $output_dir . '/' . $dirName;
        if( ! is_dir($soDir) ) {
            mkdir($soDir);
        }

        echo "Scanning libs directory" . PHP_EOL;
        $files = scandir($dirName . '/' . 'libs');
        foreach( $files as $id => $f ) {
            $fullName = $dirName . '/libs/' . $f;
            $ext = pathinfo($fullName, PATHINFO_EXTENSION);

            // Copy an .so and .a files to the installed libraries
            if( $ext == 'so' || $ext == 'a' ) {
                copy($fullName, $soDir . '/' . $f);
            }
        }
    }

    foreach($abstractions as $ab){
        $abs = $ab.'s'; // need for dir names
        echo "\tScanning $abs.";
        if (!is_dir($dirName.'/'.$abs)){
            echo "Not found, skipping.\n";
            continue;
        } else {
            echo "OK\n";
        }

        $files = scandir($dirName.'/'.$abs);
        //    print_r($files);
        // scan files in each abstraction directory
        foreach($files as $id=>$f){
            $fullName = $dirName.'/'.$abs.'/'.$f;

            if (!is_file($fullName)) continue; // a directory, silently skip
            $fName = pathinfo($f, PATHINFO_BASENAME);;
            if ( pathinfo($f, PATHINFO_EXTENSION) != 'php'){
                continue;
            }

            echo "Processing $fName." . PHP_EOL ;

            // Check the result of the syntax checking
            if( !checkFile( $fullName, $dirName ) ) {
                continue;
            }

            fwrite($fp, <<<EOT
<?/*///////////// $fName ///////////////*/?>

EOT
        );
            fwrite($fp, file_get_contents($fullName));
        }
    }

    fwrite($fp, <<<EOT
<?
} // namespace {$dirName}
?>

EOT
);


    fclose($fp);
}



/******** MAIN FUNCTION ******/


if ($argc < 3){
    echo "Usage: compile-library.php dest library1 library2 ... \n";
    echo "The result of the compilation will be files library1.php, library2.php, ...\n";
    echo "Use install-library.php to install the library in a system dir\n";
    exit(1);
}

array_shift($argv); // get rid of the executable name
$outdir = $argv[0];
$output_dir = realpath($argv[0]);
array_shift($argv);

if( $output_dir === false ) {
    echo "Unable to find specified output directory $outdir" . PHP_EOL;
    exit(1);
}

// going through the arguments and compiling the libraries
foreach ($argv as $lib)
    $lib_real = realpath($lib);
if( $lib_real === false ) {
    echo "Unable to find library directory $lib" . PHP_EOL;
    continue;
}

$lib_dirpath = dirname($lib_real);
chdir($lib_dirpath);
$lib_basename = basename($lib_real);

compileLibrary($lib_basename, $output_dir);

?>
