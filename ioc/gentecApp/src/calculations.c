#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <epicsTypes.h>
#include <aSubRecord.h>
#include <registryFunction.h>
#include <epicsExport.h>

/*
 * Running statistics for one head (Welford's algorithm).
 * State is held per aSub record in precord->dpvt, so each head's
 * :statistics record accumulates independently.
 */
typedef struct {
    epicsUInt32 n;
    double mean, m2, min, max;
} statsState;

/*
 * :reset -> clears the published outputs and raises a reset request on
 * :statistics.C (OUTG). The accumulator itself is cleared by getAverage
 * on its next sample, so only the owning record touches its state.
 */
static long resetStats(aSubRecord *precord)
{
    *(double *) precord->vala = 0;      /* average */
    *(double *) precord->valb = 0;      /* max */
    *(double *) precord->valc = 0;      /* min */
    *(double *) precord->vald = 0;      /* p2p */
    *(double *) precord->vale = 0;      /* std */
    *(double *) precord->valf = 0;      /* rms */
    *(epicsInt32 *) precord->valg = 1;  /* reset request -> :statistics.C */
    *(epicsInt32 *) precord->valh = 0;  /* elapsedTime */
    return 0;
}

static long getAverage(aSubRecord *precord)
{
    statsState *s = (statsState *) precord->dpvt;
    double value = *(double *) precord->a;
    epicsInt32 duration = *(epicsInt32 *) precord->b;
    epicsInt32 *resetReq = (epicsInt32 *) precord->c;
    double std = 0, p2p = 0, rms = 0;

    if (!s) {
        s = calloc(1, sizeof(statsState));
        if (!s)
            return -1;
        precord->dpvt = s;
    }

    if (duration < 1)
        duration = 1;

    if (*resetReq) {
        memset(s, 0, sizeof(*s));
        *resetReq = 0;
    }

    s->n++;
    if (s->n == 1) {
        s->mean = s->min = s->max = value;
        s->m2 = 0;
    } else {
        double delta = value - s->mean;
        s->mean += delta / s->n;
        s->m2 += delta * (value - s->mean);
        if (value < s->min) s->min = value;
        if (value > s->max) s->max = value;

        std = sqrt(s->m2 / (s->n - 1));
        if (s->mean != 0.0) {
            p2p = ((s->max - s->min) / s->mean) * 100;
            rms = (std / s->mean) * 100;
        }
    }

    /* Outputs */
    *(double *) precord->vala = s->mean;
    *(double *) precord->valb = s->max;
    *(double *) precord->valc = s->min;
    *(double *) precord->vald = p2p;
    *(double *) precord->vale = std;
    *(double *) precord->valf = rms;
    *(epicsInt32 *) precord->valh = (epicsInt32) s->n;

    /* Last run through: disable scan and pause */
    *(epicsInt8 *) precord->valg = (s->n >= (epicsUInt32) duration);
    *(epicsInt8 *) precord->vali = (s->n >= (epicsUInt32) duration);
    return 0;
}

epicsRegisterFunction(getAverage);
epicsRegisterFunction(resetStats);
