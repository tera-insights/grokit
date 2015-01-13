Templates       {#template-tutorial}
=========

In the previous sections, we have shown how to directly specify types,
functions, filters, transforms, aggregates, and GISTs (for brevity, all of these
will be grouped together as *tasks*). In addition to directly specifying a
task, DataPath allows for the user to specify **Templates**, which create new
tasks on the fly when provided with parameters. In this way, general tasks can
be parameterized and the general structure and logic can be reused without
needing to manually create separate concrete tasks for each specific use of the
general task.

@warning **Templates** are an advanced feature of DataPath. The templating mechanism
used by DataPath is based on M4, a turing-complete macro processing language.
It can be extremely powerful, but it can also be difficult to use as well. We
are always trying to improve the library of macros available to make defining
**Templates** easier, but it is by no means complete. Use **Templates** at your
own risk.

@note Templates for types are not yet completely implemented, and thus not
available for use.

- - - - -

\<\< @ref state-tutorial | @ref toc | @ref license \>\>
