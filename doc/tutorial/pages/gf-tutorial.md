Generalized Filters      {#gf-tutorial}
===================
[TOC]

The Generalized Filter (GF for short) is an abstraction for tasks that take
tuples as input, one at a time, and determine whether or not those tuples
pass certain criteria specified by the GF. Tuples that pass are allowed to
continue to the next waypoint.

## Implementing a GF {#gf-tut-structure}

Generalized Filters have a very simple interface, consisting of only a
class with a constructor and a single method. The GF does not need to be aware of any
attributes in the tuples that do not take part in the filtration; any extra
attributes needed by subsequent waypoints will automatically be passed through
if the tuple passes the filter.

### Construction of the Filter {#gf-tut-construct}

The GF may be constructed through the use of zero or
more constant literal arguments, or through the use of
[Constant States](@ref state-tutorial).

In the first case, the constructor should simply take the constant arguments,
if any. An example of this is the constructor for the
[PatternMatcher](@ref GF/PatternMatcher.h) GF:

@snippet GF/PatternMatcher.h constructor

### The Filtration Function {#gf-tut-filter}

The filtration functionality of the GF is fulfilled through a single function:
`Filter`. This function takes constant references to any attributes required
to perform the filtration.

Here is an example from the [PatternMatcher](@ref GF/PatternMatcher.h) GF:

@snippet GF/PatternMatcher.h filter

## Integrating a GF into DataPath {#gf-tut-hookup}

In order for a GF to be integrated into the system, it must be "tagged" with some
information to let the system know about the properties of the GF. These **tags**
are placed inside the comments of the GF source file itself, as part of a
*description block*. For a GF, the *description block* begins with `GF_DESC` and
ends with `END_DESC`, with the tags for the GF between them, as shown below.

~~~~~~~~~~~~~~~~~~~~~~~{.cc}
/*
 *  GF_DESC
 *      TAG1(arguments)
 *      TAG2(arguments)
 *      ...
 *  END_DESC
 */
~~~~~~~~~~~~~~~~~~~~~~~

### Required Tags {#gf-tut-tags-req}

The following tags are required to be present for a GF to be properly
integrated into the system.

-   **NAME**(string)

    The NAME tag is required to be the first entry in the description block.
    Here, you specify the name of the class that implements the GF.

-   **INPUTS**(attribute list)

    The inputs to the `Filter` method of the GF. The system will use this to
    ensure that the GF receives all of the inputs that it requires and that they
    are of the correct type.

### Optional Tags {#gf-tut-tags-opt}

The following tags are optional, and specify additional properties and
features of a GF beyond the basic feature set.

-   **LIBS**(string list)

    Any libraries that need to be linked during compilation in order for
    the GF to function.

-   **CONSTRUCTOR**(attribute list)

    Any basic constant arguments required by the GF during construction. If
    no constant states are required, they will be passed directly to the
    constructor. If constant states are required, they will be passed to any
    generated states.

-   **GEN_CONST_STATES**(attribute list)

    Any constant states that need to be generated prior to the GF being
    constructed and receiving any tuples. If any constructor arguments have
    been specified, they will be passed to the generated states instead of the
    GF itself.

-   **REQ_CONST_STATES**(attribute list)

    Any external constant states required by the GF. These states are acquired from
    other waypoints, and are made available to the GF at construction.

### Examples

@snippet GF/PatternMatcher.h description

- - - - -

\<\< @ref user-types-funcs | @ref toc | @ref gt-tutorial >>
