#ifndef EPICS_RETOOLS_H
#define EPICS_RETOOLS_H

#include <shareLib.h>

#ifdef __cplusplus
extern "C" {
#endif

epicsShareFunc long reAddAlias(const char *pattern, const char *alias,
        int verbose);
epicsShareFunc long reAddInfo(const char *pattern, const char *name,
        const char *value, int verbose);

#ifdef __cplusplus
}
#endif


#endif
