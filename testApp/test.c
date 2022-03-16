#include <dbStaticLib.h>
#include <dbUnitTest.h>
#include <testMain.h>
#include <errlog.h>
#include <dbAccess.h>
#include <retools.h>
#include <string.h>

void testReTools_registerRecordDeviceDriver(DBBASE *pdbbase);

static void test_reAddAlias(void)
{
    testDiag("Test reAddAlias");
    testdbPrepare();

    testdbReadDatabase("testReTools.dbd", NULL, NULL);
    testReTools_registerRecordDeviceDriver(pdbbase);
    testdbReadDatabase("test.db", NULL, NULL);

    eltc(0);
    testIocInitOk();
    eltc(1);

    testOk1(!reAddAlias("(.*):A", "$1:X"));

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
        }

        dbFinishEntry(&entry);
    }

    testIocShutdownOk();
    testdbCleanup();
}

static void test_reAddInfo(void)
{
    testDiag("Test reAddInfo");
    testdbPrepare();

    testdbReadDatabase("testReTools.dbd", NULL, NULL);
    testReTools_registerRecordDeviceDriver(pdbbase);
    testdbReadDatabase("test.db", NULL, NULL);

    eltc(0);
    testIocInitOk();
    eltc(1);

    const char *infoName = "archive";
    const char *infoMonitor = "monitor 1";
    testOk1(!reAddInfo("(.*):B", infoName, infoMonitor));


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
        }
        dbFinishEntry(&entry);
    }

    testIocShutdownOk();
}

static void test_rePutField(void)
{
    testDiag("Test rePutField");
    testdbPrepare();

    testdbReadDatabase("testReTools.dbd", NULL, NULL);
    testReTools_registerRecordDeviceDriver(pdbbase);
    testdbReadDatabase("test.db", NULL, NULL);

    eltc(0);
    testIocInitOk();
    eltc(1);

    const char *fieldName = "EGU";
    const char *fieldValue = "units";
    testOk1(!rePutField("(.*):B", fieldName, fieldValue));


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

        long found = !dbFindField(&entry, fieldName);
        testOk(found, "%s: field found", name);

        if (found) {
            testOk(!strcmp(fieldValue, dbGetString(&entry)),
                    "%s info name is correct", name);
        }
        dbFinishEntry(&entry);
    }

    testIocShutdownOk();
}


MAIN(reToolsTest)
{
    testPlan(0);
    test_reAddAlias();
    test_reAddInfo();
    test_rePutField();
    return testDone();
}

