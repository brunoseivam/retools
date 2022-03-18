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

#### `rePutField "pattern" "field" "value"`

Sets the value of a particular field to all records that match the regular expression in `pattern`.
Can be used for the VAL field. Example:

    epics> dbl
    ABC:X
    ABC:Y
    ABC:Z

    epics> rePutField "(.*):.*" "EGU" "units"
    ABC:X: put field(EGU, 'units')
    ABC:Y: put field(EGU, 'units')
    ABC:Z: put field(EGU, 'units')

    epics> dbDumpRecord
    record(ai,"ABC:X") {
        field(DTYP,"Soft Channel")
        field(EGU,"units")
    }
    record(ai,"ABC:Y") {
        field(DTYP,"Soft Channel")
        field(EGU,"units")
    }
    record(ai,"ABC:Z") {
        field(DTYP,"Soft Channel")
        field(EGU,"units")
    }

### Disabling verbose output

By default, retools has verbose output. To disable it, set the variable
`reToolsVerbose` to `0`:

    epics> var reToolsVerbose 0
    epics>

