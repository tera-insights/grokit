dnl #
dnl #  Copyright 2013 Christopher Dudley
dnl #
dnl #  Licensed under the Apache License, Version 2.0 (the "License");
dnl #  you may not use this file except in compliance with the License.
dnl #  You may obtain a copy of the License at
dnl #
dnl #      http://www.apache.org/licenses/LICENSE-2.0
dnl #
dnl #  Unless required by applicable law or agreed to in writing, software
dnl #  distributed under the License is distributed on an "AS IS" BASIS,
dnl #  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
dnl #  See the License for the specific language governing permissions and
dnl #  limitations under the License.
dnl #
dnl # This file contains functions used to translate existing Data classes
dnl # written in M4 into PHP
include(Transition.m4)

dnl # Require correct php library
m4_divert_push(0)<//>dnl
<?php
require_once('DataFunctions.php');
?>
m4_divert_pop(0)

m4_define(</M4_CREATE_BASE_DATA_TYPE/>, </dnl
<?php
dp\create_base_data_type( "$1", "$2", TO_PHP_ASSOC_ARRAY( </$3/> ), TO_PHP_ASSOC_ARRAY(</$4/>) );
?>
/>)dnl

m4_define(</M4_CREATE_DATA_TYPE/>, </dnl
<?php
dp\create_data_type( "$1", "$2", TO_PHP_ASSOC_ARRAY( </$3/> ), TO_PHP_ASSOC_ARRAY(</$4/>) );
?>
/>)dnl

m4_divert(0)
