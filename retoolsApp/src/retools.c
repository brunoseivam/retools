#include "retools.h"

#include <iocsh.h>
#include <errlog.h>
#include <dbStaticLib.h>
#include <dbAccessDefs.h>

#define PCRE2_CODE_UNIT_WIDTH 8
#include <pcre2.h>

static pcre2_code *reCompile(const char *pattern)
{
    int errorNumber;
    PCRE2_SIZE errorOffset;
    pcre2_code *re = pcre2_compile(
            (PCRE2_SPTR)pattern, PCRE2_ZERO_TERMINATED, 0,
            &errorNumber, &errorOffset, NULL);

    if (!re) {
        PCRE2_UCHAR buffer[256];
        pcre2_get_error_message(errorNumber, buffer, sizeof(buffer));
        errlogSevPrintf(errlogMajor,
                "PCRE2 compilation failed at offset %d: %s\n",
                (int)errorOffset, buffer);
    }

    return re;
}

static int reSubstitute(pcre2_code *re, const char *subject,
        const char *replacement, char *subst, size_t *substLen)
{
    int numMatches = pcre2_substitute(re,
            (PCRE2_SPTR)subject, PCRE2_ZERO_TERMINATED,
            0, PCRE2_SUBSTITUTE_EXTENDED,
            NULL, NULL,
            (PCRE2_SPTR)replacement, PCRE2_ZERO_TERMINATED,
            (PCRE2_UCHAR*)subst, (PCRE2_SIZE*)substLen);

    if (numMatches < 0) {
        char err[256];
        pcre2_get_error_message(numMatches,
                (PCRE2_UCHAR*)err, (PCRE2_SIZE)sizeof(err));
        errlogSevPrintf(errlogMajor,
                "substitution error: ('%s' -> '%s'): %s\n",
                subject, replacement, err);
        return EXIT_FAILURE;
    }

    return numMatches ? EXIT_SUCCESS : EXIT_FAILURE;
}

#define FOR_EACH_RECORD(entry, code)                                        \
    do {                                                                    \
        if (!pdbbase) {                                                     \
            errlogSevPrintf(errlogMajor, "No database loaded\n");           \
            return EXIT_FAILURE;                                            \
        }                                                                   \
        DBENTRY _entry;                                                     \
        long _status;                                                       \
                                                                            \
        dbInitEntry(pdbbase, &_entry);                                      \
        _status = dbFirstRecordType(&_entry);                               \
                                                                            \
        while (!_status) {                                                  \
            _status = dbFirstRecord(&_entry);                               \
            while (!_status) {                                              \
                DBENTRY entry;                                              \
                dbCopyEntryContents(&_entry, &entry);                       \
                code                                                        \
                dbFinishEntry(&entry);                                      \
                _status = dbNextRecord(&_entry);                            \
            }                                                               \
            _status = dbNextRecordType(&_entry);                            \
        }                                                                   \
        dbFinishEntry(&_entry);                                             \
    }while(0)

long epicsShareAPI reAliasAdd(const char *pattern, const char *alias)
{
    if (!pattern || !alias) {
        errlogSevPrintf(errlogMinor, "Usage: %s \"pattern\" \"alias\"\n",
                __func__);
        return EXIT_FAILURE;
    }

    pcre2_code *re = reCompile(pattern);
    if (!re)
        return EXIT_FAILURE;

    FOR_EACH_RECORD(entry, {
        const char *recName = dbGetRecordName(&entry);
        char subst[256];
        size_t substLen = sizeof(subst);

        if(!reSubstitute(re, recName, alias, subst, &substLen)) {
            if(dbCreateAlias(&entry, subst))
                errlogSevPrintf(errlogMinor, "Failed to alias %s -> %s\n",
                    recName, subst);
            else
                printf("Alias %s -> %s created\n", recName, subst);
        }
    });

    pcre2_code_free(re);
    return EXIT_SUCCESS;
}

long epicsShareAPI reInfoAdd(const char *pattern, const char *name,
        const char *value)
{
    if (!pattern || !name || !value) {
        errlogSevPrintf(errlogMinor,
                "Usage: %s \"pattern\" \"name\" \"value\"\n", __func__);
        return EXIT_FAILURE;
    }

    pcre2_code *re = reCompile(pattern);
    if (!re)
        return EXIT_FAILURE;

    FOR_EACH_RECORD(entry, {
        const char *recName = dbGetRecordName(&entry);
        char subst[256];
        size_t substLen = sizeof(subst);

        if(!reSubstitute(re, recName, value, subst, &substLen)) {
            if(dbPutInfo(&entry, name, subst))
                errlogSevPrintf(errlogMajor,
                        "%s: Failed to add info(%s, '%s')\n",
                        recName, name, subst);
            else
                printf("%s: added info(%s, '%s')\n", recName, name, subst);
        }
    });

    pcre2_code_free(re);
    return EXIT_SUCCESS;
}


// EPICS registration code

static const iocshArg reAliasAddArg0 = { "pattern", iocshArgString };
static const iocshArg reAliasAddArg1 = { "name", iocshArgString };
static const iocshArg * const reAliasAddArgs[2] = {
    &reAliasAddArg0, &reAliasAddArg1
};
static const iocshFuncDef reAliasAddFuncDef = {
    "reAliasAdd", 2, reAliasAddArgs
};

static void reAliasAddCallFunc(const iocshArgBuf *args) {
    reAliasAdd(args[0].sval, args[1].sval);
}

static const iocshArg reInfoAddArg0 = { "pattern", iocshArgString };
static const iocshArg reInfoAddArg1 = { "name", iocshArgString };
static const iocshArg reInfoAddArg2 = { "value", iocshArgString };
static const iocshArg * const reInfoAddArgs[3] = {
    &reInfoAddArg0, &reInfoAddArg1, &reInfoAddArg2
};
static const iocshFuncDef reInfoAddFuncDef = { "reInfoAdd", 3, reInfoAddArgs };

static void reInfoAddCallFunc(const iocshArgBuf *args) {
    reInfoAdd(args[0].sval, args[1].sval, args[2].sval);
}

static void retools_registrar(void) {
    iocshRegister(&reAliasAddFuncDef, reAliasAddCallFunc);
    iocshRegister(&reInfoAddFuncDef, reInfoAddCallFunc);
}

#include <epicsExport.h>
epicsExportRegistrar(retools_registrar);
