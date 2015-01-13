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
dnl # This file contains functions used to translate existing M4 files into
dnl # PHP
divert(-1)
include(Resources-T.m4)<//>dnl
m4_divert(-1)
dnl # Opening PHP brace and license
m4_divert_push(0)<//>dnl
<?php
//
//  Copyright 2013 Tera Insights LLC
//
//  Licensed under the Apache License, Version 2.0 (the "License");
//  you may not use this file except in compliance with the License.
//  You may obtain a copy of the License at
//
//      http://www.apache.org/licenses/LICENSE-2.0
//
//  Unless required by applicable law or agreed to in writing, software
//  distributed under the License is distributed on an "AS IS" BASIS,
//  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
//  See the License for the specific language governing permissions and
//  limitations under the License.
//
?>
m4_divert_pop(0)

m4_define(</STRIP_WS/>, </dnl
<//>m4_bpatsubst(</$1/>, </^[ ]+\|[ ]+$/>, <//>)<//>dnl
/>)dnl

dnl # Turns a bare value into a single quoted PHP string
m4_define(</TO_PHP_SQ_STRING/>, </dnl
'$1'<//>dnl
/>)dnl)

dnl # Turns a bare value into a double quoted PHP string
m4_define(</TO_PHP_DQ_STRING/>, </dnl
"$1"<//>dnl
/>)dnl)

dnl # Doesn't do anything to the input.
m4_define(</TO_PHP_BARE/>, </dnl
$1<//>dnl
/>)dnl

m4_define(</TO_PHP_NOWDOC/>, </dnl
<<<'EOT'
$1
EOT
/>)dnl

dnl # Turns a list of (name, value) pairs into a PHP associative array.
m4_define(</TO_PHP_ASSOC_ARRAY/>, </dnl
m4_ifval(</$2/>, </dnl
<//>m4_define(</__KEY_FUNC/>, m4_defn(</$2/>))<//>dnl
/>, </dnl
<//>m4_define(</__KEY_FUNC/>, m4_defn(</TO_PHP_SQ_STRING/>))<//>dnl
/>)dnl
m4_ifval(</$3/>, </dnl
<//>m4_define(</__VAL_FUNC/>, m4_defn(</$3/>))<//>dnl
/>, </dnl
<//>m4_define(</__VAL_FUNC/>, m4_defn(</TO_PHP_SQ_STRING/>))<//>dnl
/>)dnl
[dnl
<//>m4_foreach(</_A_/>, m4_quote($1), </dnl
 __KEY_FUNC(STRIP_WS(m4_first(_A_))) => __VAL_FUNC(STRIP_WS(m4_second(_A_))),<//>dnl
<//>/>)dnl
 ]<//>dnl
m4_undefine(</__KEY_FUNC/>)dnl
m4_undefine(</__VAL_FUNC/>)dnl
/>)dnl

dnl # Turns a list of elements into a PHP array.
m4_define(</TO_PHP_ARRAY/>, </dnl
m4_ifval(</$2/>, </dnl
<//>m4_define(</__VAL_FUNC/>, m4_defn(</$2/>))<//>dnl
/>, </dnl
<//>m4_define(</__VAL_FUNC/>, m4_defn(</TO_PHP_SQ_STRING/>))<//>dnl
/>)dnl
[dnl
<//>m4_foreach(</_A_/>, m4_quote($1), </dnl
 __VAL_FUNC(STRIP_WS(_A_)),<//>dnl
/>)dnl
 ]<//>dnl
m4_undefine(</__VAL_FUNC/>)dnl
/>)dnl
