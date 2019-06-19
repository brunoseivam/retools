#include <dbStaticLib.h>
#include <dbUnitTest.h>
#include <testMain.h>
#include <errlog.h>
#include <dbAccess.h>
#include <retools.h>
#include <string.h>

#include <dbmf.h>

void testReTools_registerRecordDeviceDriver(DBBASE *pdbbase);

static void test_reAddAlias(int batch)
{
    testDiag("Test reAddAlias (batch=%d)", batch);
    testdbPrepare();

    testdbReadDatabase("testReTools.dbd", NULL, NULL);
    testReTools_registerRecordDeviceDriver(pdbbase);
    testdbReadDatabase("test.db", NULL, NULL);

    reToolsBatch = batch;

    // Alias records ending in A to X, eg:
    //  DIAG_MTCA01:PICO3_CH0:A -> DIAG_MTCA01:PICO03_CH0:X
    testOk1(!reAddAlias("(.*):A", "$1:X"));

    eltc(0);
    testIocInitOk();
    eltc(1);

    // Check that all records ending in A were aliased
    const char *fmt = "DIAG_MTCA01:PICO3_CH%d:%s";
    int i = 0;
    for (i = 0; i < 4; ++i) {
        char alias[256];
        snprintf(alias, sizeof(alias), fmt, i, "X");

        char name[256];
        snprintf(name, sizeof(name), fmt, i, "A");

        DBENTRY entry;
        dbInitEntry(pdbbase, &entry);

        long found = !dbFindRecord(&entry, alias);
        testOk(found, "%s found", alias);

        if (found) {
            testOk(dbIsAlias(&entry), "%s is alias", alias);
            dbFindField(&entry, "NAME");
            testOk(!strcmp(name, dbGetString(&entry)), "%s alias is correct",
                    alias);
        } else {
            testSkip(2, "alias not found");
        }

        dbFinishEntry(&entry);
    }

    testIocShutdownOk();
    testdbCleanup();
}

static void test_reAddInfo(int batch)
{
    testDiag("Test reAddInfo (batch=%d)", batch);
    testdbPrepare();

    testdbReadDatabase("testReTools.dbd", NULL, NULL);
    testReTools_registerRecordDeviceDriver(pdbbase);
    testdbReadDatabase("test.db", NULL, NULL);

    reToolsBatch = batch;

    // Add info("archive", "monitor 1") to records that end in B
    const char *infoName = "archive";
    const char *infoMonitor = "monitor 1";
    testOk1(!reAddInfo("(.*):B", infoName, infoMonitor));

    eltc(0);
    testIocInitOk();
    eltc(1);

    // Check that all records ending in B had the info tag added
    const char *fmt = "DIAG_MTCA01:PICO3_CH%d:%s";
    int i = 0;
    for (i = 0; i < 4; ++i) {
        char name[256];
        snprintf(name, sizeof(name), fmt, i, "B");

        DBENTRY entry;
        dbInitEntry(pdbbase, &entry);

        if (dbFindRecord(&entry, name))
            testAbort("%s not found", name);

        long found = !dbFirstInfo(&entry);
        testOk(found, "%s: info found", name);

        if (found) {
            testOk(!strcmp(infoName, dbGetInfoName(&entry)),
                    "%s info name is correct", name);
            testOk(!strcmp(infoMonitor, dbGetInfoString(&entry)),
                    "%s info value is correct", name);
        } else {
            testSkip(2, "info not found");
        }
        dbFinishEntry(&entry);
    }

    testIocShutdownOk();
    testdbCleanup();
}

static void test_perf(int batch) {
    testDiag("Test perf (batch=%d)", batch);
    testdbPrepare();

    testdbReadDatabase("testReTools.dbd", NULL, NULL);
    testReTools_registerRecordDeviceDriver(pdbbase);

    // Load 50k records, of 5 different prefixes (1-5)
    testDiag("Creating records");
    int p, t;

    for(p = 0; p < 5; ++p) {
        for (t = 0; t < 5000; ++t) {
            char macros[256];
            snprintf(macros, sizeof(macros), "P=PRE%d,R=N%04d", p + 1, t);
            testdbReadDatabase("perf.db", NULL, macros);
        }
    }

    testDiag("%d Records created", p*t);

    reToolsVerbose = 0;
    reToolsBatch = batch;

    epicsTimeStamp before, after;
    epicsTimeGetCurrent(&before);

    testOk1(!reAddAlias("^PRE(.*):N(.*)$", "N$1:PRE$2"));

    // Add info("test", "p=x,n=y") to all records
    const char *infoName = "test";
    const char *infoValue = "p=$1,n=$2";
    testOk1(!reAddInfo("^PRE(.*):N(.*)$", infoName, infoValue));

    eltc(0);
    testIocInitOk();
    eltc(1);

    epicsTimeGetCurrent(&after);
    testDiag("Test took: %.3f seconds", epicsTimeDiffInSeconds(&after, &before));

    testIocShutdownOk();
    testdbCleanup();
}

MAIN(reToolsTest)
{
    testPlan(56);

    // Immediate
    test_reAddAlias(0);
    test_reAddInfo(0);

    // Batch
    test_reAddAlias(1);
    test_reAddInfo(1);

    // Performance test
    test_perf(0);
    test_perf(1);

    return testDone();
}

