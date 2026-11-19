#include <string.h>
#include <stdio.h>
#include <epicsTypes.h>
#include <aSubRecord.h>
#include <registryFunction.h>
#include <epicsExport.h>

/*
 * S-Link scale index (manual p.35, *SC / *ST) <-> label, e.g. 18 <-> "1mJ".
 * Index 0..41: mantissa 1,3,10,30,100,300 per decade triple, from 1p to 300M.
 * The base unit (J or W) comes from INPB, the head's commissioned unit
 * ($(device):$(head):BaseUnit, set from head.substitutions), so one
 * table serves energy and power heads alike.
 */
#define LABEL_SIZE 40   /* EPICS DBF_STRING size */
#define NUM_KEYS   42

static const int   mantissas[] = { 1, 3, 10, 30, 100, 300 };
static const char *prefixes[]  = { "p", "n", "u", "m", "", "k", "M" };

static const char *baseUnit(const char *in)
{
    return (in && in[0]) ? in : "J";
}

/// Function 1: Convert scale index to label. INPA=index (LONG), INPB=base unit (STRING)
long energyIntToStr(aSubRecord *precord)
{
    /* FTA=LONG is epicsInt32 (4 bytes); 'long' is 8 bytes on linux-x86_64 */
    epicsInt32 idx = (precord->a ? *((epicsInt32 *)precord->a) : -1);
    char *outstr = (char *)precord->vala;

    if (!outstr) return -1;

    if (idx >= 0 && idx < NUM_KEYS) {
        snprintf(outstr, LABEL_SIZE, "%d%s%s",
                 mantissas[idx % 6], prefixes[idx / 6],
                 baseUnit((const char *)precord->b));
    } else {
        strcpy(outstr, "INVALID");
    }
    return 0;
}

/*
 * Map an SI prefix string to its decade index. Also accepts the legacy
 * "meg" label and both micro signs (U+00B5, U+03BC in UTF-8), so autosaved
 * values from earlier releases ("1megJ", "10uJ") still restore.
 */
static int prefixIndex(const char *p)
{
    if (strcmp(p, "meg") == 0) return 6;
    if (strcmp(p, "\xC2\xB5") == 0 || strcmp(p, "\xCE\xBC") == 0) return 2;
    for (int i = 0; i < 7; i++) {
        if (strcmp(p, prefixes[i]) == 0) return i;
    }
    return -1;
}

/// Function 2: Convert label to scale index. INPA=label (STRING)
/// The J/W suffix is optional and not enforced, so a "10nJ" autosaved
/// before a head was recommissioned as W still restores.
long energyStrToInt(aSubRecord *precord)
{
    const char *instr = (const char *)precord->a;
    epicsInt32 *outidx = (epicsInt32 *)precord->vala;
    char rest[LABEL_SIZE];
    int mantissa, n = 0;
    size_t len;

    if (!instr || !outidx) return -1;

    if (sscanf(instr, " %d%n", &mantissa, &n) != 1) return -1;

    strncpy(rest, instr + n, sizeof(rest) - 1);
    rest[sizeof(rest) - 1] = '\0';

    len = strlen(rest);
    while (len && rest[len - 1] == ' ') rest[--len] = '\0';
    if (len && (rest[len - 1] == 'J' || rest[len - 1] == 'W')) rest[--len] = '\0';

    int decade = prefixIndex(rest);
    if (decade < 0) return -1;

    for (int m = 0; m < 6; m++) {
        if (mantissas[m] == mantissa) {
            *outidx = decade * 6 + m;
            return 0;
        }
    }
    /* Not found: non-zero return stops the aSub writing OUTA, so no
     * scale command is sent to the device. BRSV raises the alarm. */
    return -1;
}

// Register both with EPICS
epicsRegisterFunction(energyIntToStr);
epicsRegisterFunction(energyStrToInt);
