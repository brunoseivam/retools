#include "retools.h"

#include <iocsh.h>
#include <errlog.h>
#include <dbStaticLib.h>
#include <dbAccessDefs.h>
#include <epicsStdio.h>
#include <functional>
#include <regex>

using std::regex;
using std::regex_replace;
using std::regex_error;
using std::string;

#define VERBOSE_EACH_ALIAS  (1<<0)
#define VERBOSE_PRINT_TOTAL (1<<1)

int reToolsVerbose = 1;

typedef std::function<void(DBENTRY*,string const &,string const &)>
    foreach_func_t;

int forEachMatchingRecord(string const & pattern, string const & replace,
    foreach_func_t func)
{
    regex re;

    try {
        re = regex(pattern);
    }
    catch (const regex_error & e) {
        errlogSevPrintf(errlogMajor, "Regex compilation failed: %s\n", e.what());
        return EXIT_FAILURE;
    }

    DBENTRY _entry;
    dbInitEntry(pdbbase, &_entry);

    long _status = dbFirstRecordType(&_entry);
    unsigned int n_records = 0;

    while (!_status) {
        _status = dbFirstRecord(&_entry);
        while (!_status) {
            string recName(dbGetRecordName(&_entry));

            // There was a match
            if (regex_match(recName, re)) {
                DBENTRY entry;

                // We matched with an alias, find the aliased record
                if (dbIsAlias(&_entry)) {
                    dbInitEntry(pdbbase, &entry);
                    dbFindField(&_entry, "NAME");
                    dbFindRecord(&entry, dbGetString(&_entry));
                } else
                    dbCopyEntryContents(&_entry, &entry);

                func(&entry, recName, regex_replace(recName, re, replace));
                dbFinishEntry(&entry);
                // Count how many matches
                n_records++;
            }

            _status = dbNextRecord(&_entry);
        }
        _status = dbNextRecordType(&_entry);
    }
    dbFinishEntry(&_entry);
    // If desired, display total of records processed
    if(reToolsVerbose & VERBOSE_PRINT_TOTAL)
        printf("Total matches: %u\n", n_records);
    return EXIT_SUCCESS;
}

long epicsShareAPI reGrep(const char *pattern)
{
    if (!pattern) {
        errlogSevPrintf(errlogMinor, "Usage: %s \"pattern\"\n", __func__);
        return EXIT_FAILURE;
    }

    return forEachMatchingRecord(pattern, "",
        [](DBENTRY *entry, string const & recName, string const & value) {
            fprintf(epicsGetStdout(),"%s\n", recName.c_str());
        });
}

long epicsShareAPI reTest(const char *pattern, const char *value)
{
    if (!pattern || !value) {
        errlogSevPrintf(errlogMinor, "Usage: %s \"pattern\" \"value\"\n",
                __func__);
        return EXIT_FAILURE;
    }

    return forEachMatchingRecord(pattern, value,
        [](DBENTRY *entry, string const & recName, string const & value) {
            fprintf(epicsGetStdout(),"%s\t%s\n", recName.c_str(), value.c_str());
        });
}

long epicsShareAPI reAddAlias(const char *pattern, const char *alias)
{
    if (!pattern || !alias) {
        errlogSevPrintf(errlogMinor, "Usage: %s \"pattern\" \"alias\"\n",
                __func__);
        return EXIT_FAILURE;
    }

    return forEachMatchingRecord(pattern, alias,
        [](DBENTRY *entry, string const & recName, string const & alias) {
            if(dbCreateAlias(entry, alias.c_str()))
                errlogSevPrintf(errlogMinor, "Failed to alias %s -> %s\n",
                    recName.c_str(), alias.c_str());

            else if(reToolsVerbose & VERBOSE_EACH_ALIAS)
                fprintf(epicsGetStdout(),"Alias %s -> %s created\n", recName.c_str(),
                    alias.c_str());
        });
}

long epicsShareAPI reAddInfo(const char *pattern, const char *name,
        const char *value)
{
    if (!pattern || !name || !value) {
        errlogSevPrintf(errlogMinor,
                "Usage: %s \"pattern\" \"name\" \"value\"\n", __func__);
        return EXIT_FAILURE;
    }

    return forEachMatchingRecord(pattern, value,
        [name](DBENTRY *entry, string const & recName, string const & value) {
            if(dbPutInfo(entry, name, value.c_str()))
                errlogSevPrintf(errlogMajor,
                    "%s: Failed to add info(%s, '%s')\n", recName.c_str(),
                    name, value.c_str());
            else if(reToolsVerbose)
                fprintf(epicsGetStdout(),"%s: added info(%s, '%s')\n", recName.c_str(), name,
                    value.c_str());
        });
}

long epicsShareAPI rePutField(const char *pattern, const char *field,
        const char *value)
{
    if (!pattern || !field || !value) {
        errlogSevPrintf(errlogMinor,
                "Usage: %s \"pattern\" \"field\" \"value\"\n", __func__);
        return EXIT_FAILURE;
    }

    return forEachMatchingRecord(pattern, value,
        [field](DBENTRY *entry, string const & recName, string const & value) {
            DBADDR addr;
            string fieldName = recName + "." + field;

            if(dbNameToAddr(fieldName.c_str(), &addr))
                errlogSevPrintf(errlogMajor,
                    "%s: does not possess field %s \n", recName.c_str(),
                    field);
            else if(dbPutField(&addr, DBR_STRING, value.c_str(),1L)) 
                errlogSevPrintf(errlogMajor,
                    "%s: Failed to put field(%s, '%s')\n", recName.c_str(),
                    field, value.c_str());
            else if(reToolsVerbose)
                fprintf(epicsGetStdout(),"%s: put field(%s, '%s')\n", recName.c_str(), field,
                    value.c_str());
        });
}

long epicsShareAPI reGetField(const char *pattern, const char *field)
{
    if (!pattern || !field) {
        errlogSevPrintf(errlogMinor,
                "Usage: %s \"pattern\" \"field\"\n", __func__);
        return EXIT_FAILURE;
    }

    return forEachMatchingRecord(pattern, "",
        [field](DBENTRY *entry, string const & recName, string const & value) {
            DBADDR addr;
            string fieldName = recName + "." + field;
            char buffer[MAX_STRING_SIZE+1];
            long one = 1;

            if(dbNameToAddr(fieldName.c_str(), &addr))
                errlogSevPrintf(errlogMajor,
                    "%s: does not possess field %s \n", recName.c_str(),
                    field);
            else if(dbGetField(&addr, DBR_STRING, &buffer, NULL, &one, NULL))
                errlogSevPrintf(errlogMajor,
                    "%s: Failed to get field %s\n", recName.c_str(),
                    field);
            else{
                fprintf(epicsGetStdout(),"%s.%s '%s'\n", recName.c_str(), field, buffer);
                }
        });
}


// EPICS registration code
static const iocshArg reGrepArg0 = { "pattern", iocshArgString };
static const iocshArg * const reGrepArgs[1] = { &reGrepArg0 };
static const iocshFuncDef reGrepFuncDef = { "reGrep", 1, reGrepArgs };
static void reGrepCallFunc(const iocshArgBuf *args) { reGrep(args[0].sval); }

static const iocshArg reTestArg0 = { "pattern", iocshArgString };
static const iocshArg reTestArg1 = { "name", iocshArgString };
static const iocshArg * const reTestArgs[2] = { &reTestArg0, &reTestArg1 };
static const iocshFuncDef reTestFuncDef = { "reTest", 2, reTestArgs };
static void reTestCallFunc(const iocshArgBuf *args) {
    reTest(args[0].sval, args[1].sval);
}

static const iocshArg reAddAliasArg0 = { "pattern", iocshArgString };
static const iocshArg reAddAliasArg1 = { "name", iocshArgString };
static const iocshArg * const reAddAliasArgs[2] = {
    &reAddAliasArg0, &reAddAliasArg1
};
static const iocshFuncDef reAddAliasFuncDef = {
    "reAddAlias", 2, reAddAliasArgs
};

static void reAddAliasCallFunc(const iocshArgBuf *args) {
    reAddAlias(args[0].sval, args[1].sval);
}

static const iocshArg reAddInfoArg0 = { "pattern", iocshArgString };
static const iocshArg reAddInfoArg1 = { "name", iocshArgString };
static const iocshArg reAddInfoArg2 = { "value", iocshArgString };
static const iocshArg * const reAddInfoArgs[3] = {
    &reAddInfoArg0, &reAddInfoArg1, &reAddInfoArg2
};
static const iocshFuncDef reAddInfoFuncDef = { "reAddInfo", 3, reAddInfoArgs };

static void reAddInfoCallFunc(const iocshArgBuf *args) {
    reAddInfo(args[0].sval, args[1].sval, args[2].sval);
}

static const iocshArg rePutFieldArg0 = { "pattern", iocshArgString };
static const iocshArg rePutFieldArg1 = { "field", iocshArgString };
static const iocshArg rePutFieldArg2 = { "value", iocshArgString };
static const iocshArg * const rePutFieldArgs[3] = {
    &rePutFieldArg0, &rePutFieldArg1, &rePutFieldArg2
};
static const iocshFuncDef rePutFieldFuncDef = { "rePutField", 3, rePutFieldArgs };

static void rePutFieldCallFunc(const iocshArgBuf *args) {
    rePutField(args[0].sval, args[1].sval, args[2].sval);
}

static const iocshArg reGetFieldArg0 = { "pattern", iocshArgString };
static const iocshArg reGetFieldArg1 = { "field", iocshArgString };
static const iocshArg * const reGetFieldArgs[2] = {
    &reGetFieldArg0, &reGetFieldArg1
};
static const iocshFuncDef reGetFieldFuncDef = { "reGetField", 2, reGetFieldArgs };

static void reGetFieldCallFunc(const iocshArgBuf *args) {
    reGetField(args[0].sval, args[1].sval);
}


static void retools_registrar(void) {
    iocshRegister(&reGrepFuncDef, reGrepCallFunc);
    iocshRegister(&reTestFuncDef, reTestCallFunc);
    iocshRegister(&reAddAliasFuncDef, reAddAliasCallFunc);
    iocshRegister(&reAddInfoFuncDef, reAddInfoCallFunc);
    iocshRegister(&rePutFieldFuncDef, rePutFieldCallFunc);
    iocshRegister(&reGetFieldFuncDef, reGetFieldCallFunc);
}

#include <epicsExport.h>
epicsExportRegistrar(retools_registrar);
epicsExportAddress(int, reToolsVerbose);
