<?

/* Main php file to process the JSON describing the execution and generate all files needed.

  Usage: php Process.php query.json

*/

/***************** INCLUDE SECTION ******************/
require_once 'grokit_base.php';
require_once 'grokit_math.php';

require_once 'CodeGeneration.php';
require_once 'Print.cc.php';
require_once 'Selection.cc.php';
require_once 'GI.cc.php';

// GLAs
require_once 'GLA_PostFinalize.cc.php';
require_once 'GLA.cc.php';

require_once 'GT.cc.php';
require_once 'GIST.cc.php';
require_once 'JoinLHS.cc.php';
require_once 'JoinLHSHash.cc.php';
require_once 'JoinRHS.cc.php';
require_once 'JoinMergeWorkFuncs.cc.php';
require_once 'Cleaner.cc.php';
require_once 'Cache.cc.php';
require_once 'Cluster.cc.php';

require_once 'grokit_codegen.php';
require_once 'codegen_helpers.php';
require_once 'Make.php';

/*************** HELPER FUNCTIONS ************************/

xdebug_enable();

/*************** COMMAND LINE PARSING *********************/
if ($argc < 2){
  echo "Usage: php Process.php query.json";
  exit(1);
}

if (!file_exists($argv[1])){
  echo "File ".$argv[1]." cannot be read";
  exit(1);
}

$configFile = './config/default.json';

if( $argc >= 3 ) {
    $configFile = $argv[2];
}

if( !file_exists($configFile) ) {
    echo "Configuration file " . $configFile . " does not exist" . PHP_EOL;
    exit(1);
}

$input = json_decode(file_get_contents($argv[1]));

$genInfo = \grokit\parseProgram($input);

$config = json_decode(file_get_contents($configFile));

\grokit\generateMakefile($genInfo["generated"], $config);

$generated = $genInfo['generated'];
$result = [
    'waypoints' => $genInfo['waypoints'],
    'edges' => $genInfo['edges'],
    'jobs' => $generated->jobs(),
    'filesPerJob' => $generated->filesPerJob(),
    'filesPerWaypoint' => $generated->filesPerWaypoint(),
    ];

file_put_contents("./GenerationInfo.json", json_encode($result));

?>
