Generalized Linear Aggregates   {#gla-tutorial}
=============================
[TOC]

The Generalized Linear Aggregate (GLA for short) is an abstraction for tasks
that make a passes over a set of data, and then produce results
after all data has been processed. Each tuple from the input data set is
fed to the GLA exactly once in each pass, and in no particular order. They are
used to perform aggregation and analytic tasks, such as sums, averages,
linear regressions, and more.

## Designing a GLA {#gla-tut-theory}

**TODO**: Expand this entire section. Add formulas and actual mathematical
definitions.

A GLA is made of up a **state** and two operations: *AddItem* and *AddState*. The
state and its operations must fulfill certain requirements to be considered
a GLA and accepted by the system.

### AddItem

*AddItem* must be associative. This means that for a given state and set of
tuples, adding all of the tuples to the state in different orders should
produce equivalent states.

### AddState

*AddState* must be both associative and commutative. This essentially means that
given a set of states, adding them together in different orders should produce
equivalent resultant states.

## Implementing a GLA {#gla-tut-structure}

**TODO**: expand.

Once a GLA has been designed, it must be implemented to conform to the
interface the system expects.

### Construction of the State {#gla-tut-construct}

The GLA state may be constructed using
zero or more constant literal arguments (as seen below with the Average GLA):

@snippet GLA/AverageGLA.h average-constructor

The GLA may also be constructed using [Constant States](@ref state-tutorial).

### Receiving Input {#gla-tut-input}

All GLAs receive input in the same way: tuples will be passed to the GLA one
at a time through the `AddItem` method, with each attribute of the tuple as a
separate parameter. The parameters will be passed as constant references to
prevent modification.

@note The references given to the GLA are only guaranteed to point to valid data
for the duration of the `AddItem` call. If the GLA needs to store its input, it
**must** make copies.

Here is an example of the `AddItem` method of the Average GLA:

@snippet GLA/AverageGLA.h average-additem

### Merging States {#gla-tut-merge}

All GLAs also are merged together in the same way: one GLA state will be told
to add another state to itself through the `AddState` method. The `AddState`
method should take exactly one argument, which should be a reference to
another state of the same GLA.

@note It is acceptable to make modifications to the other state.

@note The state given by the reference is only guaranteed to exist for the
duration of the `AddState` call. If the GLA needs to store information from
the other state, it must either make copies or remove the information from
other state.

Here is an example of the `AddState` method of the Average GLA:

@snippet GLA/AverageGLA.h average-add-state

### Producing Results {#gla-tut-output}

There are several different ways in which a GLA may produce results, depending
upon the nature and number of the results.

#### Producing a Single Result (single) {#gla-tut-output-single}

This is the simplest way of producing relational data. With this method, exactly
once tuple is produced as output from the GLA. The method `GetResult` will be
called on the GLA, with references to the storage locations of the attributes
as parameters.

@note There is no separate finalization step with this output method.

An example of this output method is the `GetResult` method of the Average GLA:

@snippet GLA/AverageGLA.h average-getresult

#### Producing Multiple Results (multi) {#gla-tut-output-multi}

This output method allows for a GLA to produce zero or more tuples of relational
data. A GLA with this output method must support the following methods:

~~~~~~~~~~~{.cc}
void Finalize( void );
bool GetNextResult( Attribute1Type&, Attribute2Type&, ... );
~~~~~~~~~~~

The method `Finalize` will be called first, allowing for the GLA to prepare for
result extraction. Afterwards, `GetNextResult` will be called repeatedly.
`GetNextResult` should place the values of the next result into the parameters
and then return true, or return false if there are no more results to produce.

##### Example

A simple example of this type is the [CountDistinct](@ref GLA/CountDistinct.h)
GLA. Below are the `Finalize` and `GetNextResult` methods:

@snippet GLA/CountDistinct.h count-distinct-finalize

@snippet GLA/CountDistinct.h count-distinct-get-next-result

#### Producing Groups of Results (fragment) {#gla-tut-output-fragment}

The `fragment` output method allows for multiple tuples to be produced in
parallel. This is an excellent choice for GLAs that produce large amounts
of data as a result, as long as the results can be broken into discrete
fragments that have no effect on one another. A GLA with this output method
must have the following:

~~~~~~~~~~~~~~{.cc}
// Replace NAME below with the GLA's class name
class NAME_Iterator;

int GetNumFragments( void );
NAME_Iterator* Finalize( int );
bool GetNextResult( NAME_Iterator*, Attribute1Type&, Attribute2Type&, ... );
~~~~~~~~~~~~~~

The `GetNumFragments` method will be called first, and should return the number
of fragments the GLA can break the output into. It is acceptable for the number
of fragments to be 0, in which case no output will be produced.

The `Finalize` method takes an int, which is the ID of the fragment to be
produced. The ID will be in the range \[0, N\), where N is the number of fragments
returned by `GetNumFragments`. The `Finalize` method should return a pointer to
an iterator, which will be passed to the `GetNextResult` method to keep track
of what tuples still need to be produced for that fragment. What the iterator
contains and how the `GetNextResult` method uses it are implementation defined.

The `GetNextResult` method now takes a pointer to an iterator as the first
parameter, but otherwise should act exactly the same as the `GetNextResult`
method for the `multi` output option. Attributes of the output tuple should be
stored in the locations specified by the parameters and true should be returned
if a tuple as successfully produced, and false if there are no more tuples to
produce.

##### Example

In the examples, there is a version of the CountDistinct GLA that uses the
`fragment` interface, called [CountDistinctFrag](@ref GLA/CountDistinctFrag.h).

#### Producing Non-Relational Data (state) {#gla-tut-output-state}

Instead of producing relational data in the form of tuples, a GLA can instead
produce non-relational data by marking its return type as `state`. In this
case, the GLA state itself will be the output of the waypoint. The system will
package up the GLA state appropriately and then send it to its next
destination.

The system itself does not require any additional methods to be specified in
this case. However, the GLA itself will likely need to specify additional
methods in order for other objects in further waypoints to access its data.

@note The system can optionally finalize a GLA before producing it as a state.
For more information, see the [optional tags](@ref gla-tut-tags-opt) section.

### Iteration {#gla-tut-iteration}

If a single pass over the data is not sufficient for the task, a GLA can be
specified as iterative, and will be allowed to make zero or more additional
passes over the data.

An iterative GLA **must** have a single generated constant state, which
should be used to store inter-iteration data. The constant state's class
name should be NAME\_ConstState, where NAME is the name of the GLA. Any
intra-iteration data should be stored within the GLA state itself.

A GLA state will be constructed with the constant state as its first
constructor argument, passed as a constant reference. The GLA must also
support one additional method:

~~~~~~~~~~~~~{.cc}
bool ShouldIterate(NAME_ConstState&);
~~~~~~~~~~~~~

The `ShouldIterate` method will be called prior to any out the output-method
operations, and should return true if the GLA needs to make another pass
over the data. The constant state is passed as a standard reference, and the
GLA is allowed to make any needed modifications to the constant state at this
time.

For an example of an iterative GLA, see the
[LogisticRegressionIRLS GLA](@ref GLA/LogisticRegressionIRLS.h).

## Integrating a GLA into DataPath {#gla-tut-hookup}

In order for a GLA to be integrated into the system, it must be "tagged" with some
information to let the system know about the properties of the GLA. These
**tags** are placed inside the comments of the GLA source file itself, as part
of a *description block*. For a GLA, the *description block* begins with
`GLA_DESC` and ends with `END_DESC`, with the tags for the GLA between them,
as shown below.

~~~~~~~~~~~~~~{.cc}
/*
 *  GLA_DESC
 *      TAG1(arguments)
 *      TAG2(arguments)
 *      ...
 *  END_DESC
 */
~~~~~~~~~~~~~~

### Required Tags {#gla-tut-tags-req}

The following tags are required to be present for a GLA to be properly
described.

-   **NAME**(string)

    The NAME tag is required to be the first entry in the description block.
    Here, you specify the name of the class that implements the GLA state.

-   **INPUTS**(attribute list)

    The inputs to the AddItem method of the GLA. The system will use this to
    ensure that the GLA receives all of the inputs it requires and that they
    are of the correct types.

-   **OUTPUTS**(attribute list)

    The outputs produced by the GLA. The system will use this to extract
    data from the GLA and package it appropirately.

-   **RESULT\_TYPE**(string)

    This specifies which of the supported result types the GLA uses. This
    will be used to determine how data will be extracted from the GLA.

    Currently supported result types are:

    -   single
    -   multi
    -   fragment
    -   state

### Optional Tags {#gla-tut-tags-opt}

The following tags are optional, and specify additional properties and features
of the GLA beyond the basic feature set.

-   **LIBS**(string list)

    Any libraries that need to be linked during compilation in order for
    the GLA to function.

-   **CONSTRUCTOR**(attribute list)

    Any basic constant arguments required by the GLA during construction. If
    no constant states are required, they will be passed directly to the
    constructor. If constant states are required, they will be passed to any
    generated states.

-   **GEN_CONST_STATES**(attribute list)

    Any constant states that need to be generated prior to the GLA being
    constructed and receiving any tuples. If any constructor arguments have
    been specified, they will be passed to the generated states instead of the
    GLA itself.

-   **REQ_CONST_STATES**(attribute list)

    Any external constant states required by the GLA. These states are acquired from
    other waypoints, and are made available to the GLA at construction.

-   **OPT_ITERABLE**

    Marks the GLA as being iterable.

    @note If this option is specified, it is assumed that the GLA requires
    exactly one generated constant state of the type NAME_ConstState.
    The external constant states are not affected.


-   **OPT_CHUNKBOUNDARY**

    If this option is specified, the GLA will be notified every type that it
    has finished processing a chunk. Any GLA that specifies this option must
    support the following method:

~~~~~~~~~~~~{.cc}
void ChunkBoundary(void);
~~~~~~~~~~~~

-   **OPT_FINALIZE_AS_STATE**

    If this option is specified, the GLA will be finalized even if it is being
    output as a constant state for another waypoint.

    A GLA that specifies this option must support the following method:

~~~~~~~~~~~~{.cc}
void FinalizeState(void);
~~~~~~~~~~~~

### Examples {#gla-tut-tags-example}

Here is an example of a basic GLA description for the Average GLA:

@snippet GLA/AverageGLA.h average-desc

Here is an example of the description of theLogisticRegressionIRLS GLA, which
is more complex and supports iteration.

@snippet GLA/LogisticRegressionIRLS.h ex-logreg-desc

- - - - -

\<\< @ref gt-tutorial | @ref toc | @ref gist-tutorial \>\>
