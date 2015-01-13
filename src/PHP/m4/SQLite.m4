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
dnl # This file contains functions used to translate existing files that use
dnl # the M4 SQLite helper macros into PHP.
include(Transition.m4)

m4_divert_push(0)<//>dnl
<?php
require_once('SQLite.php');
?>
m4_divert_pop(0)

m4_define(</SQL_STATEMENTS_NOREZ/>, </dnl
<?php
dp\sql_statements_norez( TO_PHP_NOWDOC(</$1/>), TO_PHP_ARRAY(</$2/>) );
?>
/>)dnl

m4_define(</SQL_STATEMENT_PARAMETRIC_NOREZ/>, </dnl
<?php
dp\sql_statement_parametric_norez( TO_PHP_NOWDOC(</$1/>), TO_PHP_ARRAY(</$2/>), TO_PHP_ARRAY(</$3/>));
?>
/>)dnl

m4_define(</SQL_INSTANTIATE_PARAMETERS/>, </dnl
<?php
dp\sql_instantiate_parameters( TO_PHP_ARRAY(</$*/>) );
?>
/>)dnl

m4_define(</SQL_PARAMETRIC_END/>, </dnl
<?php
dp\sql_parametric_end();
?>
/>)dnl

m4_define(</SQL_STATEMENT_SCALAR/>, </dnl
<?php
dp\sql_statement_scalar( TO_PHP_NOWDOC(</$1/>), TO_PHP_SQ_STRING(</$2/>), TO_PHP_SQ_STRING(</$3/>), TO_PHP_ARRAY(</$4/>));
?>
/>)dnl

m4_define(</SQL_STATEMENT_TABLE/>, </dnl
<?php
dp\sql_statement_table( TO_PHP_NOWDOC(</$1/>), TO_PHP_ASSOC_ARRAY(</$2/>), TO_PHP_ARRAY(</$3/>) );
?>
/>)dnl

m4_define(</SQL_END_STATEMENT_TABLE/>, </dnl
<?php
dp\sql_end_statement_table();
?>
/>)dnl

m4_define(</SQL_OPEN_DATABASE/>, </dnl
<?php
dp\sql_open_database( TO_PHP_SQ_STRING(</$1/>) );
?>
/>)dnl

m4_define(</SQL_EXISTING_DATABASE/>, </dnl
<?php
dp\sql_existing_database( TO_PHP_SQ_STRING(</$1/>) );
?>
/>)dnl

m4_define(</SQL_DATABASE_OBJECT/>, </dnl
<?=dp\sql_database_object()?><//>dnl
/>)dnl

m4_define(</SQL_CLOSE_DATABASE/>, </dnl
<?php
dp\sql_close_database();
?>
/>)dnl

m4_divert(0)
