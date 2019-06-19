#include "retools.h"

#include <iocsh.h>
#include <errlog.h>
#include <dbStaticLib.h>
#include <dbAccessDefs.h>
#include <initHooks.h>

#include <functional>
#include <regex>
#include <vector>
#include <unordered_map>

using std::regex;
using std::regex_match;
using std::regex_replace;
using std::regex_error;
using std::string;
using std::vector;
using std::unordered_map;

int reToolsVerbose = 1;

/*
 * reToolsBatch controls when retools commands will run:
 *   - If set to 0, retools commands will run immediately.
 *   - If set to 1, retools commands that are issued before
 *     iocInit will be collected and run as a batch at
 *     AfterDatabaseRunning.
 *
 * retools commands issued after iocInit run immediately
 * regardless of this variable's value. It is not recommended
 * to set this variable to 1 on memory constrained systems.
 */
int reToolsBatch = 0;

static bool pastInit = false;

typedef std::function<bool(string const &)> match_func_t;
typedef std::function<void(string const &, DBENTRY*)> foreach_func_t;
typedef std::function<void(DBENTRY*, string const &, string const &)> replace_func_t;

// Store pending actions
struct Pending {
    regex re;
    string replace;
    replace_func_t func;

    Pending(regex & re, string const & replace, replace_func_t func)
    : re(re), replace(replace), func(func) {}
};
static vector<Pending> pending;

/*
 * Apply `func` on all records whose names satisfy `match`.
 */
static int forEachMatchingRecord(match_func_t match, foreach_func_t func)
{
    DBENTRY _entry;
    dbInitEntry(pdbbase, &_entry);

    long _status = dbFirstRecordType(&_entry);

    while (!_status) {
        _status = dbFirstRecord(&_entry);
        while (!_status) {
            string recName(dbGetRecordName(&_entry));

            // There was a match
            if (match(recName)) {
                DBENTRY entry;

                // We matched with an alias, find the aliased record
                // TODO: EPICS7 has dbFollowAlias instead
                if (dbIsAlias(&_entry)) {
                    dbInitEntry(pdbbase, &entry);
                    dbFindField(&_entry, "NAME");
                    dbFindRecord(&entry, dbGetString(&_entry));
                } else
                    dbCopyEntryContents(&_entry, &entry);

                func(recName, &entry);

                dbFinishEntry(&entry);
            }

            _status = dbNextRecord(&_entry);
        }
        _status = dbNextRecordType(&_entry);
    }
    dbFinishEntry(&_entry);
    return EXIT_SUCCESS;
}

static int reReplace(string const & pattern, string const & replace, replace_func_t func)
{
    regex re;

    try {
        re = regex(pattern);
    }
    catch (const regex_error & e) {
        errlogSevPrintf(errlogMajor, "Regex compilation failed: %s\n", e.what());
        return EXIT_FAILURE;
    }

    // We were asked to run as batch at iocInit, so queue the request for later
    if (reToolsBatch && !pastInit) {
        pending.emplace_back(re, replace, func);
        return EXIT_SUCCESS;
    }

    // TODO: optimization: not all functions want to perform regex_replace
    return forEachMatchingRecord(
        /* match */ [&](string const & name) { return regex_match(name, re); },
        /* func  */ [&](string const & name, DBENTRY *entry) {
            func(entry, name, regex_replace(name, re, replace));
        }
    );
}

static void reInitHook(initHookState state)
{
    if (state != initHookAfterDatabaseRunning || !reToolsBatch)
        return;

    // Build a map of all existing records
    unordered_map<string, DBENTRY> map;

    forEachMatchingRecord(
        /* match */ [](string const &) { return true; },
        /* func  */ [&](string const & name, DBENTRY *entry) { dbCopyEntryContents(entry, &map[name]); }
    );

    // Apply all pending actions
    for (auto & pair : map) {
        string const & name = pair.first;
        DBENTRY *entry = &pair.second;

        for (auto & action : pending)
            if (regex_match(name, action.re))
                action.func(entry, name, regex_replace(name, action.re, action.replace));
    }

    pastInit = true;
}


// Exported functions

long epicsShareAPI reGrep(const char *pattern)
{
    if (!pattern) {
        errlogSevPrintf(errlogMinor, "Usage: %s \"pattern\"\n", __func__);
        return EXIT_FAILURE;
    }

    return reReplace(pattern, "",
        [](DBENTRY *entry, string const & recName, string const & value) {
            printf("%s\n", recName.c_str());
        });
}

long epicsShareAPI reTest(const char *pattern, const char *value)
{
    if (!pattern || !value) {
        errlogSevPrintf(errlogMinor, "Usage: %s \"pattern\" \"value\"\n",
                __func__);
        return EXIT_FAILURE;
    }

    return reReplace(pattern, value,
        [](DBENTRY *entry, string const & recName, string const & value) {
            printf("%s\t%s\n", recName.c_str(), value.c_str());
        });
}

long epicsShareAPI reAddAlias(const char *pattern, const char *alias)
{
    if (!pattern || !alias) {
        errlogSevPrintf(errlogMinor, "Usage: %s \"pattern\" \"alias\"\n",
                __func__);
        return EXIT_FAILURE;
    }

    return reReplace(pattern, alias,
        [](DBENTRY *entry, string const & recName, string const & alias) {
            if(dbCreateAlias(entry, alias.c_str()))
                errlogSevPrintf(errlogMinor, "Failed to alias %s -> %s\n",
                    recName.c_str(), alias.c_str());
            else if(reToolsVerbose)
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

    return reReplace(pattern, value,
        [name](DBENTRY *entry, string const & recName, string const & value) {
            if(dbPutInfo(entry, name, value.c_str()))
                errlogSevPrintf(errlogMajor,
                    "%s: Failed to add info(%s, '%s')\n", recName.c_str(),
                    name, value.c_str());
            else if(reToolsVerbose)
                printf("%s: added info(%s, '%s')\n", recName.c_str(), name,
                    value.c_str());
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

static void retools_registrar(void) {
    iocshRegister(&reGrepFuncDef, reGrepCallFunc);
    iocshRegister(&reTestFuncDef, reTestCallFunc);
    iocshRegister(&reAddAliasFuncDef, reAddAliasCallFunc);
    iocshRegister(&reAddInfoFuncDef, reAddInfoCallFunc);

    initHookRegister(reInitHook);
}

#include <epicsExport.h>
epicsExportRegistrar(retools_registrar);
epicsExportAddress(int, reToolsVerbose);
epicsExportAddress(int, reToolsBatch);
