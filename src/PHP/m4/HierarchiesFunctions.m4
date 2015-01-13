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
dnl # This file contains functions used to translate existing interface classes
dnl # written in M4 into PHP
include(Transition.m4)

m4_define(</M4_INTERFACE_CLASS/>, </dnl
<?php
dp\interface_class( TO_PHP_SQ_STRING(</$1/>), TO_PHP_SQ_STRING(</$2/>), TO_PHP_SQ_STRING(</$3/>) );
?>
/>)dnl

m4_define(</M4_INTERFACE_CLASS_END/>, </dnl
<?php
dp\interface_class_end();
?>
/>)dnl

m4_define(</M4_INTERFACE_DEFAULT_CONSTRUCTOR/>, </dnl
<?php
dp\interface_default_constructor();
?>
/>)dnl

m4_define(</M4_INTERFACE_CONSTRUCTOR/>, </dnl
<?php
dp\interface_constructor( TO_PHP_ASSOC_ARRAY(</$1/>) );
?>
/>)dnl

m4_define(</M4_INTERFACE_FUNCTION/>, </dnl
<?php
dp\interface_function( TO_PHP_SQ_STRING(</$1/>), TO_PHP_SQ_STRING(</$2/>), TO_PHP_ASSOC_ARRAY(</$3/>) );
?>
/>)dnl

m4_divert(0)dnl
<?php
require_once('HierarchiesFunctions.php');
?>
