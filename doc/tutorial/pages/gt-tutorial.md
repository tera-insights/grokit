Generalized Transforms      {#gt-tutorial}
==================================
[TOC]

The Generalized Transform (GT for short) is an abstraction for tasks that
take tuples as input and, for each input tuple, produce zero or more
tuples as output, with a different schema than the input tuple. As the name
would suggest, this abstraction is useful for representing transforms upon
a set of data.

## Implementing a GT {#gt-tut-structure}

Generalized Transforms have a relatively simple interface, but are slightly more
complex than Generalized Filters. They have the same construction rules as
GFs, but unlike GFs, they have multiple different interfaces available for
receiving and producing data, depending upon how the transform will produce
results.

### Construction of the Transform {#gt-tut-construct}

There are two ways in which a GT may be constructed: through the use of zero or
more constant literal arguments, or through the use of
[Constant States](@ref state-tutorial).

In the first case, the constructor should simply take the constant arguments,
if any. An example of this is the constructor for the
[IntervalToDiscrete](@ref GT/IntervalToDiscrete.h) GT:

@snippet GT/IntervalToDiscrete.h constructor

### Performing a Transformation {#gt-tut-transform}

There are several different ways in which a Generalized Transform may apply a
transformation upon a tuple, depending on the number of output tuples that
are possible. The transformation methods are:

-   `single`:   Exactly one output tuple is produced for every input tuple.
-   `multi`:    Zero or more output tuples are produced for every input tuple.

#### One-to-One Transforms {#gt-tut-output-single}

For one-to-one transforms, exactly one tuple is produced for every input tuple.
In this case, the `ProcessTuple` method will be called. The parameters to this
method are constant references to the attributes of the input tuple followed by
references to the storage locations of the attributes of the output tuple.

An example of this output method is the [Fahrenheit to Celsius](@ref GT/FtoC.h)
transform:

@snippet GT/FtoC.h process-tuple

#### One-to-Many Transforms {#gt-tut-output-multi}

For one-to-many transforms, the interface is split into two separate methods.
First, the `ProcessTuple` method is called with only constant references to
the input tuple attributes as parameters. Then `GetNextResult` is repeatedly
called with references to the storage locations of the output tuple attributes
as parameters. `GetNextResult` should return true if the output tuple is valid,
and false if there are no more output tuples.

Here is pseudo-code for how the system will interact with the transform with
this output method:

~~~~~~~~~~~~~~~~~~~{.cc}
for each tuple {
    gt.ProcessTuple(Input Attributes);
    while( gt.GetNextResult(Output Attributes) ) {
        // Store results
    }
}
~~~~~~~~~~~~~~~~~~~


An example of this output method is the
[Interval to Discrete](@ref GT/IntervalToDiscrete.h) transform:

@snippet GT/IntervalToDiscrete.h process-tuple

@snippet GT/IntervalToDiscrete.h get-next-result

## Integrating a GT into DataPath {#gt-tut-hookup}

In order for a GT to be integrated into the system, it must be "tagged" with some
information to let the system know about the properties of the GT. These
**tags** are placed inside the comments of the GT source file itself, as part
of a *description block*. For a GT, the *description block* begins with
`GT_DESC` and ends with `END_DESC`, with the tags for the GT between them,
as shown below.

~~~~~~~~~~~~~~{.cc}
/*
 *  GT_DESC
 *      TAG1(arguments)
 *      TAG2(arguments)
 *      ...
 *  END_DESC
 */
~~~~~~~~~~~~~~

### Required Tags {#gt-tut-tags-req}

The following tags are required to be present for a GT to be properly
described.

-   **NAME**(string)

    The NAME tag is required to be the first entry in the description block.
    Here, you specify the name of the class that implements the GT state.

-   **INPUTS**(attribute list)

    The inputs to the AddItem method of the GT. The system will use this to
    ensure that the GT receives all of the inputs it requires and that they
    are of the correct types.

-   **OUTPUTS**(attribute list)

    The outputs produced by the GT. The system will use this to extract
    data from the GT and package it appropirately.

-   **RESULT\_TYPE**(string)

    This specifies which of the supported result types the GT uses. This
    will be used to determine how data will be extracted from the GT.

    Currently supported result types are:

    -   single
    -   multi

### Optional Tags {#gt-tut-tags-opt}

The following tags are optional, and specify additional properties and features
of the GT beyond the basic feature set.

-   **LIBS**(string list)

    Any libraries that need to be linked during compilation in order for
    the GT to function.

-   **CONSTRUCTOR**(attribute list)

    Any basic constant arguments required by the GT during construction. If
    no constant states are required, they will be passed directly to the
    constructor. If constant states are required, they will be passed to any
    generated states.

-   **GEN_CONST_STATES**(attribute list)

    Any constant states that need to be generated prior to the GT being
    constructed and receiving any tuples. If any constructor arguments have
    been specified, they will be passed to the generated states instead of the
    GT itself.

-   **REQ_CONST_STATES**(attribute list)

    Any external constant states required by the GT. These states are acquired from
    other waypoints, and are made available to the GT at construction.

### Examples

Here is the description for the [Fahrenheit to Celsius](@ref GT/FtoC.h) transform:

@snippet GT/FtoC.h description

Here is the description for the
[Interval to Discrete](@ref GT/IntervalToDiscrete.h) transform:

@snippet GT/IntervalToDiscrete.h description

- - - - -

\<\< @ref gf-tutorial | @ref toc | @ref gla-tutorial >>
