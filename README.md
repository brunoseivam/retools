retools
=======

retools is an EPICS module that contains a few useful EPICS shell commands
that use regular expressions. Depends on libpcre2.

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

