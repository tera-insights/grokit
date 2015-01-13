<?
// Copyright 2013 Tera Insights, LLC. All Rights Reserved.

namespace grokit {

    abstract class JsonValue {
        private $name;
        private $source;

        public function __construct($source) {
            $this->name = generate_name('json_val');
            $this->source = $source;
        }

        public function name() { return $this->name; }
        public function source() { return $this->source; }
        abstract public function init();
    }

    class JsonFile extends JsonValue {
        private $filename;

        public function __construct($source, $file) {
            parent::__construct($source);
            $this->filename = $file;
        }

        public function init() {
            $readerVar = $this->name() . '_reader';
            $istreamVar = $this->name() . '_stream';
?>
            Json::Value <?=$this->name()?>;
            Json::Reader <?=$readerVar?>;
            std::ifstream <?=$istreamVar?>("<?=$this->filename?>");
            <?=$readerVar?>.parse(<?=$istreamVar?>, <?=$this->name()?>);
<?
        }
    }

    class JsonLiteral extends JsonValue {
        private $json = null;

        public function __construct( $source, $js ) {
            parent::__construct($source);

            $this->json = $js;
        }

        public function init() {
?>
    Json::Value <?=$this->name()?>;
<?
            self::printJson($this->json, $this->name());
        }

        private static function printArray($arr, $var) {
?>
    <?=$var?> = Json::Value(Json::arrayValue);
<?
            foreach( $arr as $ind => $val ) {
                $nVar = $var . "[Json::ArrayIndex({$ind})]";
                self::printJson($val, $nVar);
            }
        }

        private static function printObject($obj, $var) {
?>
    <?=$var?> = Json::Value(Json::objectValue);
<?
            foreach( $obj as $name => $val ) {
                $nVar = $var . "[\"{$name}\"]";
                self::printJson($val, $nVar);
            }
        }

        private static function printJson($js, $var) {
            if( is_null($js) ) {
?>
    <?=$var?> = Json::Value(Json::nullValue);
<?
            } else if( is_string($js) ) {
?>
    <?=$var?> = "<?=$js?>";
<?
            } else if( is_numeric($js) ) {
?>
    <?=$var?> = <?=$js?>;
<?
            } else if( is_array($js) ) {
                // If it's an associative array (i.e. contains non-natural-number indicies)
                // treat it as an object instead
                $natural_indicies = true;
                foreach( $js as $ind => $val ) {
                    if( !is_int($ind) || $ind < 0 ) {
                        $natural_indicies = false;
                    }
                }
                if( $natural_indicies )
                    self::printArray($js, $var);
                else
                    self::printObject($js, $var);
            } else if( is_object($js) ) {
                self::printObject($js, $var);
            } else {
                grokit_error("Cannot translate value of type " . gettype($js) . " to JSON");
            }
        }
    }
}

?>
