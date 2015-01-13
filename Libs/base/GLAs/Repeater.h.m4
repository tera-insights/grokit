dnl # Repeater Meta-GLA
dnl #
dnl # Author: Christopher Dudley
dnl #
dnl # This GLA groups together several GLAs into one package.
dnl # The input of the Repeater is sent to all interior GLAs.
dnl # The output of the Repeater is all of the outputs of the interior GLAs
dnl # glued together.
dnl #
dnl # Currently, only GLAs that return a single tuple are supported.
dnl #
dnl # Usage
dnl # $1=GLA_NAME name of the class generated
dnl # All other arguments are names of the GLAs to repeat input to.

GLA_TEMPLATE_DESC(</Repeater/>)
m4_define(</Repeater/>, </dnl
m4_redefine(</GLA_NAME/>, </$1/>)dnl
dnl
m4_ifdef_undef(</MY_INPUTS/>)dnl
m4_ifdef_undef(</MY_OUTPUTS/>)dnl
m4_ifdef_undef(</INNER_GLA/>)dnl
m4_ifdef_undef(</MY_REZTYPE/>)dnl
dnl
m4_redefine(</OUTVAL/>, 0)dnl
m4_redefine(</GLAVAL/>, 0)dnl
dnl
dnl # This block of code takes the arguments to the macro past the first and
dnl # adds tuples of the form (name, type) to INNER_GLA, where name is a unique
dnl # name created on the stop and type is the type of the GLA.
dnl # for each gla, name_INPUT and name_OUTPUT are also defined, and the
dnl # variables in the output have values appended to them to make
dnl # them unique.
dnl # MY_INPUTS and MY_OUTPUTS contain lists of the total inputs and outputs
dnl # to the repeater.
m4_foreach(</_ARG_/>, m4_quote(m4_shift($@)), </
<//>m4_redefine(</_GLANAME_/>, </gla_/>GLAVAL)
<//>m4_autoincr(</GLAVAL/>)
<//>m4_redefine(</_GLA_/>, </(/>_GLANAME_</, />_ARG_</)/>)
<//>m4_append(</INNER_GLA/>, m4_quote(_GLA_), </</, />/>)
<//>m4_ifndef(_ARG_</_INPUT/>, </SCAN_GLA_FILE(/>_ARG_</)/>)
dnl # Handle GLA inputs
<//>m4_ifndef(</MY_INPUTS/>, </m4_define(</MY_INPUTS/>, m4_defn(_ARG_</_INPUT/>))/>)
<//>m4_redefine(_GLANAME_</_INPUT/>, defn(</MY_INPUTS/>))
dnl # Handle GLA outputs
<//>m4_foreach(</_OUTPUT/>, _ARG_</_OUTPUT/>, </
<//><//>m4_redefine(</_TMP_/>, m4_quote(</(/>m4_first(_OUTPUT)</_/>OUTVAL</, />m4_second(_OUTPUT)</)/>))
<//><//>m4_append(</MY_OUTPUTS/>, _TMP_, </</, />/>)
<//><//>m4_append(_GLANAME_</_OUTPUT/>, _TMP_)
<//><//>m4_autoincr(</OUTVAL/>)
<//>/>)
dnl # Handle result types
<//>m4_ifndef(</MY_REZTYPE/>,</
<//><//>m4_define(</MY_REZTYPE/>, m4_quote(</GLA_REZTYPE_/>_ARG_))/>,</
<//><//>m4_if(m4_quote(MY_REZTYPE), reval(</GLA_REZTYPE_/>_ARG_), <//>, </
<//><//><//>m4_errprintn(</Have GLAs with different result types in the same multiplexer! />)
<//><//><//>m4_exit(1)
<//><//>/>)
<//>/>)
/>)dnl
dnl
m4_if(MY_REZTYPE, single, <//>, </
<//>m4_errprintn(</Multiplexing GLAs only supported for GLAs that return a single result./>)
<//>m4_exit(1)
/>)dnl
dnl
/** Information for Meta-GLAs
 * GLA_DESC
 *
 * NAME(GLA_NAME)
 * INPUTS(MY_INPUTS)
 * OUTPUTS(MY_OUTPUTS)
 * RESULT_TYPE(MY_REZTYPE)
 *
 * END_DESC
 */

class GLA_NAME {

m4_foreach(</__GLA__/>, m4_quote(INNER_GLA), </dnl
    m4_second(__GLA__) m4_first(__GLA__);
/>)dnl

public:
    GLA_NAME</()/> {}
    ~GLA_NAME</()/> {}

    void AddItem(TYPED_ARGS(MY_INPUTS)) {
        // Call AddItem on each gla individually.

m4_foreach(</__GLA__/>, m4_quote(INNER_GLA), </dnl
        m4_first(__GLA__).AddItem(ARGS(m4_quote(reval(m4_first(__GLA__)</_INPUT/>))));
/>)dnl
    }

    void AddState(GLA_NAME& other) {
        // Call AddState on each GLA individually.

m4_foreach(</__GLA__/>, m4_quote(INNER_GLA), </dnl
        m4_first(__GLA__).AddState(other.m4_first(__GLA__));
/>)dnl
    }

m4_if(MY_REZTYPE, </multi/>, </dnl
    void Finalize() {
        // Call Finalize on each GLA individually

m4_foreach(</__GLA__/>, m4_quote(INNER_GLA), </dnl
        m4_first(__GLA__).Finalize();
/>)dnl
    }
/>)dnl

    void GetResult(TYPED_REF_ARGS(MY_OUTPUTS)) {
        // Call GetResult on each GLA individually.

m4_foreach(</__GLA__/>, m4_quote(INNER_GLA), </dnl
        m4_first(__GLA__).GetResult(ARGS(m4_quote(reval(m4_first(__GLA__)</_OUTPUT/>))));
/>)dnl
    }
};
/>)
