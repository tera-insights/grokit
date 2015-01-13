divert(-1)
#
#  Copyright 2012 Christopher Dudley
#
#  Licensed under the Apache License, Version 2.0 (the "License");
#  you may not use this file except in compliance with the License.
#  You may obtain a copy of the License at
#
#      http://www.apache.org/licenses/LICENSE-2.0
#
#  Unless required by applicable law or agreed to in writing, software
#  distributed under the License is distributed on an "AS IS" BASIS,
#  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
#  See the License for the specific language governing permissions and
#  limitations under the License.
#
# This file serves as the base M4 layer for DataPath.
# GNU M4 is required.

# Set the quoting system to </ />.
changequote()
changequote(</, />)

# Change the prefix for define, defn, and undefine to m4_ so that there is less
# of a chance of interference from user-generated code.

define(</m4_define/>,       defn(</define/>))
define(</m4_defn/>,         defn(</defn/>))
define(</m4_undefine/>,     defn(</undefine/>))

m4_undefine(</define/>)
m4_undefine(</defn/>)
m4_undefine(</undefine/>)

# Macros for renaming macros. Useful, since we will be renaming a few to
# lessen the chance of user code accidently calling them

m4_define(</m4_copy/>,
</m4_define(</$2/>, m4_defn(</$1/>))/>)

m4_define(</m4_rename/>,
</m4_copy(</$1/>, </$2/>)m4_undefine(</$1/>)/>)


m4_define(</m4_rename_m4/>,
</m4_rename(</$1/>, </m4_$1/>)/>)

# Rename whatever macros we need to

m4_rename_m4(</ifdef/>)
m4_rename(</ifelse/>, </m4_if/>)

m4_rename_m4(</builtin/>)
m4_rename_m4(</changecom/>)
m4_rename_m4(</changequote/>)
m4_ifdef(</changeword/>,
</m4_undefine(</changeword/>)/>)
m4_rename_m4(</debugfile/>)
m4_rename_m4(</debugmode/>)
m4_rename_m4(</decr/>)
m4_rename_m4(</divert/>)
m4_rename_m4(</divnum/>)
m4_rename_m4(</dumpdef/>)
m4_rename_m4(</errprint/>)
m4_rename_m4(</esyscmd/>)
m4_rename_m4(</eval/>)
m4_rename_m4(</format/>)
m4_undefine(</include/>)
m4_rename_m4(</incr/>)
m4_rename_m4(</index/>)
m4_rename_m4(</indir/>)
m4_rename_m4(</len/>)
m4_rename(</m4exit/>, </m4_exit/>)
m4_undefine(</m4wrap/>)
m4_ifdef(</mkstemp/>,
</m4_rename_m4(</mkstemp/>)
m4_copy(</m4_mkstemp/>, </m4_maketemp/>)
m4_undefine(</maketemp/>)/>,
</m4_rename_m4(</maketemp/>)
m4_copy(</m4_maketemp/>, </m4_mkstemp/>)/>)
m4_rename(</patsubst/>, </m4_bpatsubst/>)
m4_rename_m4(</popdef/>)
m4_rename_m4(</pushdef/>)
m4_rename(</regexp/>, </m4_bregexp/>)
m4_rename_m4(</shift/>)
m4_undefine(</sinclude/>)
m4_rename_m4(</substr/>)
m4_ifdef(</symbols/>,
</m4_rename_m4(</symbols/>)/>)
m4_rename_m4(</syscmd/>)
m4_rename_m4(</sysval/>)
m4_rename_m4(</traceoff/>)
m4_rename_m4(</traceon/>)
m4_rename_m4(</translit/>)
m4_rename_m4(</undivert/>)

m4_define(</m4_location/>,
</__file__:__line__/>)

m4_define(</m4_errprintn/>,
</m4_errprint(</$1
/>)/>)

m4_define(</m4_warning/>,
</m4_errprintn(m4_location</: warning: $1/>)/>)

m4_define(</m4_fatal/>,
</m4_errprintn(m4_location</: error: $1/>)dnl
m4_exit(m4_if(</$2/>,, 1, </$2/>))/>)

m4_define(</m4_assert/>,
</m4_if(m4_eval(</$1/>), 0,
       </m4_fatal(</assert failed: $1/>, </$2/>)/>)/>)

### Some additional useful macros

# Expands to $2 if $1 is not the empty string, $3 otherwise
m4_define(</m4_ifval/>,
</m4_if(</$1/>, <//>, </$3/>, </$2/>)/>)

# Simply the opposite of ifdef,
# expands to $2 if a macro with the name $1 is not defined, and
# $3 if it is defined
m4_define(</m4_ifndef/>,
</m4_ifdef(</$1/>, </$3/>, </$2/>)/>)

# Macros for shifting arguments twice or 3 times
m4_define(</m4_shift2/>, </m4_shift(m4_shift($@))/>)
m4_define(</m4_shift3/>, </m4_shift(m4_shift(m4_shift($@)))/>)

# Macros for quoting arguments
m4_define(</m4_dquote/>, </</$@/>/>)
m4_define(</m4_quote/>, </</$*/>/>)

# Include macro to prevent multiple includes
m4_define(</m4_include/>,
</m4_ifndef(</m4_include($1)/>,
</m4_builtin(</include/>,</$1/>)<//>dnl
m4_define(</m4_include($1)/>)/>)/>)

m4_define(</m4_sinclude/>,
</m4_ifndef(</m4_include($1)/>,
</m4_builtin(</sinclude/>,</$1/>)<//>dnl
m4_define(</m4_include($1)/>)/>)/>)

# Case macro
m4_define(</m4_case/>,
</m4_if(</$#/>, 0, <//>,
       </$#/>, 1, <//>,
       </$#/>, 2, </$2/>,
       </$1/>, </$2/>, </$3/>,
       </$0(</$1/>, m4_shift3($@))/>)/>)

# foreach macro
m4_define(</m4_foreach/>,
</m4_if(</$2/>, <//>, <//>,
       </m4_pushdef(</$1/>)_$0(</$1/>, </$3/>, <//>, $2)m4_popdef(</$1/>)/>)/>)

m4_define(</_m4_foreach/>,
</m4_if(</$#/>, </3/>, <//>,
       </m4_define(</$1/>, </$4/>)$2<//>$0(</$1/>, </$2/>, m4_shift3($@))/>)/>)

# Macro for appending to lists
m4_define(</m4_append/>,
</m4_define(</$1/>, m4_ifdef(</$1/>, </m4_defn(</$1/>)</$3/>/>)</$2/>)/>)

###
### Miscellaneous macros found to be useful
###

# ensures that a macro has been completely undefined
# this is because GNU M4's define and undefine actually behave as pushdef and popdef
m4_define(</m4_undefine_full/>, </dnl
<//>m4_ifdef(</$1/>, </m4_undefine(</$1/>)m4_undefine_full(</$1/>)/>)<//>dnl
/>)

# Undefines a macro if it is defined
m4_define(</m4_ifdef_undef/>, </dnl
<//>m4_ifdef(</$1/>, </m4_undefine(</$1/>)/>)dnl
/>)dnl

# If a macro with the name $1 is defined, undefines it.
# Then, defines $1 to mean $2.
m4_define(</m4_redefine/>,</dnl
<//>m4_ifdef_undef(</$1/>)dnl
<//>m4_define(</$1/>, </$2/>)dnl
/>)

# transforms an ASCII string to uppercase
m4_define(</m4_to_upper/>, </dnl
<//>m4_translit(</$1/>, </abcdefghijklmnopqrstuvwxyz/>, </ABCDEFGHIJKLMNOPQRSTUVWXYZ/>)<//>dnl
/>)

# transforms an ASCII string to lowercase
m4_define(</m4_to_lower/>, </dnl
<//>m4_translit(</$1/>, </ABCDEFGHIJKLMNOPQRSTUVWXYZ/>, </abcdefghijklmnopqrstuvwxyz/>)<//>dnl
/>)

# Strip quotes form text
m4_define(</m4_strip_dquotes/>, </dnl
<//>m4_bpatsubst(</$1/>, </^"\(.*\)"$/>, </\1/>)<//>dnl
/>)

m4_define(</m4_strip_squotes/>, </dnl
<//>m4_bpatsubst(</$1/>, </^'\(.*\)'$/>, </\1/>)<//>dnl
/>)

m4_define(</m4_strip_quotes/>, </dnl
<//>m4_bpatsubst(</$1/>, </^\(["']\)\(.*\)\1$/>, </\2/>)<//>dnl
/>)

# Macros to push and pop diversions

m4_define(</m4_divert_push/>,
    </m4_ifval(</$1/>,
        </m4_pushdef(</m4_previous_diversion/>, m4_divnum)<//>m4_divert($1)/>,
        </m4_warning(</WARNING: m4_divert_push with no argument ignored/>)/>)dnl
/>)dnl

m4_define(</m4_divert_pop/>,
    </m4_ifndef(</m4_previous_diversion/>, </m4_fatal(</ERROR: too many calls to m4_divert_pop!/>)/>)/><//>dnl
<//></m4_ifval(</$1/>,
        </m4_if($1, m4_divnum,
            </m4_divert(m4_previous_diversion)<//>m4_popdef(</m4_previous_diversion/>)/>,
            </m4_warning(</WARNING: m4_divert_pop did not specify the correct diversion to pop!/>)/>)/>)dnl
/>)dnl

# For loop macro.
m4_define(</m4_forloop/>, </dnl
m4_pushdef(</$1/>, </$2/>)_$0($@)m4_popdef(</$1/>)dnl
/>)dnl

m4_define(</_m4_forloop/>, </dnl
$4<//>m4_if($1, </$3/>, <//>, </m4_define(</$1/>, m4_incr($1))$0($@)/>)dnl
/>)dnl

m4_divert(0)<//>dnl
