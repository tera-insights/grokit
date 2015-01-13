Generalized Iterable State Transforms       {#gist-tutorial}
=====
[TOC]

The Generalized Iterable State Transform (GIST for short) is an abstraction for tasks that receive or
generate an initial set of data, and then perform rounds of transitions
upon that state until the state has converged to the desired result. GISTs
can be used to model simulations and other classes of problems where there is
some amount of setup done for the problem, and then most of the work is
performed by iterating upon the (likely non-relational) data.

## Implementing a GIST {#gist-tut-structure}

In DataPath, a GIST is represented as a collection of 3-4 classes:

1.  A Task (need not be a class, merely a type)
2.  A Local Scheduler
3.  A Convergence GLA
4.  The GIST state

### The Task {#gist-tut-task}

A task represents a single transition that needs to be made on the state, and
should contain any information necessary to perform this transition. There
are no rules regarding what a Task must be, or what interface it must have.
It may be a custom-made class, a class from a pre-made library, or even
a basic C++ type. It is the job of the Local Scheduler to know what Tasks
it needs to produce, and the GIST to know what needs to be done to the state
given a certain task.

### The Local Scheduler {#gist-tut-local-scheduler}

A Local Scheduler is responsible for producing Tasks to be used to perform
state transitions. If the ordering of these Tasks is important, it is up
to the Local Scheduler to produce them in the correct order. For every round of
transitions, there will be multiple Local Schedulers producing Tasks in
parallel, and Tasks being run in parallel. Local Schedulers should not be
affected by changes in the state or other Local Schedulers. Ideally, tasks
produced by one Local Scheduler should not be affected by the actions of
tasks produced by another Local Scheduler.

A Local Scheduler is required to have the following public interface:

~~~~~~~~~~~~~~~~~~~~~~{.cc}
class LocalScheduler {

public:
    bool GetNextTask( Task& );
};
~~~~~~~~~~~~~~~~~~~~~~

The `GetNextTask` method stores the next task to be run in the location
specified by the parameter. If there were no more tasks to be run, the method
should return false, and true otherwise.

### The Convergence GLA {#gist-tut-gla}

The Convergence GLA (CGLA for short) is a specialization of the GLA abstraction
that is used to determine when the GIST is done iterating. There will be one
CGLA for every Local Scheduler, and should keep track of information necessary
to determine whether or not the GIST has converged to its final state.

CGLAs are required to have at least the following public interface:

~~~~~~~~~~~~~~~~~~~~~~{.cc}
class CGLA {

public:
    void AddState( CGLA& );
    bool ShouldIterate();
};
~~~~~~~~~~~~~~~~~~~~~~

The `AddState` method takes another CGLA state, and adds it to the current
state. The `ShouldIterate` method should return true if the GIST requires
another iteration, or false if the GIST has converged.

### The GIST State {#gist-tut-state}

The GIST state is a class that represents the shared data over all of the
threads of execution. It should contain all information that is global to
all GIST functionality.

A GIST state should have the following form:

~~~~~~~~~~~~{.cc}
class GIST {
    // Private data and any helper methods go here.

public:
    // Constructor
    GIST(...);

    // Typedefs highly recommended for readability
    typedef std::pair<LocalScheduler*, CGLA*>   WorkUnit;
    typedef std::vector<WorkUnit>               WorkUnitVector;

    void PrepareRound(WorkUnitVector&, int);
    void DoStep(Task&, CGLA&);

    // Result type specific output methods go here
};
~~~~~~~~~~~~

The constructor may either take constant literal arguments, or it may use
[State Passing](@ref state-tutorial).

The `PrepareRound` method should produce the Local Schedulers and CGLAs for
that round and place them into the vector provided. The integer parameter
is a hint provided to the GIST for the number of work units it would like.
It is fine to provide more work units than the hint, but providing less will
negatively impact the amount of parallelization that can be achieved.

@note The Local Schedulers and GLAs should be allocated using dynamic memory
allocation.

@note The GIST does not need to worry about deallocating Local Schedulers and
CGLAs; the system will automatically deallocate them when they are no longer
needed.

The `DoTask` method takes a Task and a CGLA, and should perform a single
state transition using the information given by the Task. Any information
related to the step's effect on the convergence of the GIST should be fed
to the CGLA at this time.

#### Producing Output

GISTs support all of the same output interfaces as [GLAs](@ref gla-tutorial).
For more information, please see the @ref gla-tut-output section of the
GLA tutorial.

## Integrating a GIST into DataPath {#gist-tut-hookup}

In order for a GIST to be integrated into the system, it must be "tagged" with some
information to let the system know about the properties of the GIST. These
**tags** are placed inside the comments of the GIST source file itself, as part
of a *description block*. For a GIST, the *description block* begins with
`GIST_DESC` and ends with `END_DESC`, with the tags for the GIST between them,
as shown below.

~~~~~~~~~~~~~~{.cc}
/*
 *  GIST_DESC
 *      TAG1(arguments)
 *      TAG2(arguments)
 *      ...
 *  END_DESC
 */
~~~~~~~~~~~~~~

@note Only the GIST needs to have a description block. Description blocks
are not required for the Task, Local Scheduler, or CGLA.

### Required Tags {#gla-tut-tags-req}

The following tags are required to be present for a GIST to be properly
described.

-   **NAME**(string)

    The NAME tag is required to be the first entry in the description block.
    Here, you specify the name of the class that implements the GIST state.

-   **INPUTS**(attribute list)

    The inputs to the AddItem method of the GIST. The system will use this to
    ensure that the GIST receives all of the inputs it requires and that they
    are of the correct types.

-   **OUTPUTS**(attribute list)

    The outputs produced by the GIST. The system will use this to extract
    data from the GIST and package it appropirately.

-   **RESULT\_TYPE**(string)

    This specifies which of the supported result types the GIST uses. This
    will be used to determine how data will be extracted from the GIST.

    Currently supported result types are:

    -   single
    -   multi
    -   fragment
    -   state

-   **TASK_TYPE**(string)

    This specifies the type that is used for the Task.

-   **LOCAL\_SCHEDULER\_TYPE**(string)

    This specifies the type that is used for the Local Scheduler.

-   **GLA\_TYPE**(string)

    This specifies the type that is used for the Convergence GLA.

### Optional Tags {#gla-tut-tags-opt}

The following tags are optional, and specify additional properties and features
of the GIST beyond the basic feature set.

-   **LIBS**(string list)

    Any libraries that need to be linked during compilation in order for
    the GIST to function.

-   **CONSTRUCTOR**(attribute list)

    Any basic constant arguments required by the GIST during construction. If
    no constant states are required, they will be passed directly to the
    constructor. If constant states are required, they will be passed to any
    generated states.

-   **GEN_CONST_STATES**(attribute list)

    Any constant states that need to be generated prior to the GIST being
    constructed and receiving any tuples. If any constructor arguments have
    been specified, they will be passed to the generated states instead of the
    GIST itself.

-   **REQ_CONST_STATES**(attribute list)

    Any external constant states required by the GIST. These states are acquired from
    other waypoints, and are made available to the GIST at construction.

-   **OPT_FINALIZE_AS_STATE**

    If this option is specified, the GIST will be finalized even if it is being
    output as a constant state for another waypoint.

    A GIST that specifies this option must support the following method:

~~~~~~~~~~~~{.cc}
void FinalizeState(void);
~~~~~~~~~~~~

- - - - -

\<\< @ref gla-tutorial | @ref toc | @ref state-tutorial \>\>
