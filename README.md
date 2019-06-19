retools
=======

retools is an EPICS module that contains a few useful EPICS shell commands
that use regular expressions. Uses C++11's regexp.

#### `reGrep "pattern"`

Prints record names that match the regular expression in `pattern`.
Example:

    epics> dbl
    ABC:X
    ABC:Y
    ABC:Z

    epics> reGrep "X"
    ABC:X

    epics> reGrep "[X|Y]"
    ABC:X
    ABC:Y

#### `reTest "pattern" "value"`

Tests a substitution of `pattern` with `value`, useful for testing before
running `reAddAlias` or `reAddInfo`.
Example:

    epics> dbl
    ABC:X
    ABC:Y
    ABC:Z

    epics> reTest "AB(.*):.*$" "$1"
    ABC:X C

#### `reAddAlias "pattern" "alias"`

Adds an alias to all records that match the regular expression in `pattern`.
Example:

    epics> dbl
    ABC:X
    ABC:Y
    ABC:Z

    epics> reAddAlias "ABC:(.*)" "DEF:$1"
    Alias ABC:X -> DEF:X created
    Alias ABC:Y -> DEF:Y created
    Alias ABC:Z -> DEF:Z created

    epics> dbla
    DEF:X -> ABC:X
    DEF:Y -> ABC:Y
    DEF:Z -> ABC:Z

#### `reAddInfo "pattern" "name" "value"`

Adds an info tag to all records that match the regular expression in `pattern`.
Matching groups can be used for `value`. Example:

    epics> dbl
    ABC:X
    ABC:Y
    ABC:Z

    epics> reAddInfo "(.*):.*" "prefix" "$1"
    ABC:X: added info(prefix, 'ABC')
    ABC:Y: added info(prefix, 'ABC')
    ABC:Z: added info(prefix, 'ABC')

    epics> dbDumpRecord
    record(ai,"ABC:X") {
        field(DTYP,"Soft Channel")
        info("prefix","ABC")
    }
    record(ai,"ABC:Y") {
        field(DTYP,"Soft Channel")
        info("prefix","ABC")
    }
    record(ai,"ABC:Z") {
        field(DTYP,"Soft Channel")
        info("prefix","ABC")
    }

### Disabling verbose output

By default, retools has verbose output. To disable it, set the variable
`reToolsVerbose` to `0`:

    epics> var reToolsVerbose 0
    epics>

### Speeding up execution (at a temporary memory comsumption spike cost)

By default, each `re*` command executes immediately. This involves
traversing the entire linked list of records and testing the regular
expression on each record name. If the record in question is an alias,
a second traversal is required (for EPICS <= 3.15) in order to find
the aliased record. These traversals are potentially slow since
there's likely no data locality when following linked list pointers
and the processor cache gets invalidated frequently.

For that reason, it is possible to delay the execution of `re*`
commands (issued prior to `iocInit`) until `iocHookAfterDatabaseRunning`
by setting the variable `reToolsBatch` to `1`:

    epics> var reToolsBatch 1
    epics>

In this situation, at `iocHookAfterDatabaseRunning`, a Hash Map
from record name to `DBENTRY` is built and the `re*` commands are
run against this Hash Map instead. This has the benefit of improving
data locality at a cost of increased memory usage. The Hash Map
is freed after all delayed `re*` commands are run.

Subsequent `re*` commands that are issued after `iocInit` will execute
immediately.

Note: since in batch mode the Hash Map is built before any `re*` command
run, batched `re*` commands won't be able to operate on an alias created
by a previous `reAddAlias` command on the same batch.
