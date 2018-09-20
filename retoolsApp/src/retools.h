#ifndef EPICS_RETOOLS_H
#define EPICS_RETOOLS_H

#include <shareLib.h>

#ifdef __cplusplus
extern "C" {
#endif

epicsShareFunc long reAliasAdd(const char *pattern, const char *alias);
epicsShareFunc long reInfoAdd(const char *pattern, const char *name,
        const char *value);

#ifdef __cplusplus
}
#endif


#endif
