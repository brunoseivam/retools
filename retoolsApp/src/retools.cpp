#include "retools.h"

#include <iocsh.h>
#include <errlog.h>
#include <dbStaticLib.h>
#include <dbAccessDefs.h>

#include <functional>
#include <regex>

using std::regex;
using std::regex_replace;
using std::regex_error;
using std::string;

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

    while (!_status) {
        _status = dbFirstRecord(&_entry);
        while (!_status) {
            DBENTRY entry;
            dbCopyEntryContents(&_entry, &entry);

            string recName(dbGetRecordName(&entry));
            string subst(regex_replace(recName, re, replace));

            // There was a match
            if (recName != subst)
                func(&entry, recName, subst);

            dbFinishEntry(&entry);
            _status = dbNextRecord(&_entry);
        }
        _status = dbNextRecordType(&_entry);
    }
    dbFinishEntry(&_entry);
    return EXIT_SUCCESS;
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
            else
                printf("Alias %s -> %s created\n", recName.c_str(),
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
            else
                printf("%s: added info(%s, '%s')\n", recName.c_str(), name,
                    value.c_str());
        });
}


// EPICS registration code

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

static void retools_registrar(void) {
    iocshRegister(&reAddAliasFuncDef, reAddAliasCallFunc);
    iocshRegister(&reAddInfoFuncDef, reAddInfoCallFunc);
}

#include <epicsExport.h>
epicsExportRegistrar(retools_registrar);