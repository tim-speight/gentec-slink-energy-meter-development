#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>
#include <aSubRecord.h>
#include <registryFunction.h>
#include <epicsExport.h>

unsigned int i, duration;
unsigned long *ptr, *ptr1;
double oldMean, avg, oldSum, newSum, std, p2p, rms, value, min, max;

static long resetStats(aSubRecord *precord)
{
    if (ptr1) {
        free(ptr1);
        ptr1 = NULL;
        ptr = NULL;
    }
    avg = max = min = p2p = std = rms = i = 0;
    *(double *) precord -> vala = avg;
    *(double *) precord -> valb = max;
    *(double *) precord -> valc = min;
    *(double *) precord -> vald = p2p;
    *(double *) precord -> vale = std;
    *(double *) precord -> valf = rms;
    *(long *) precord -> valh = i;
    return 0;
}

static long getAverage(aSubRecord *precord)
{
    i++;

    //Inputs
    value = *(double*) precord -> a;
    duration = *(int*) precord -> b;
    *(char *) precord -> valg = 0; //enable scan
    *(char *) precord -> vali = 0; //enable pause

    //Memory allocation
    if(ptr == NULL) {
        i = 1;
        ptr = (unsigned long*) calloc(duration, sizeof(long));
        ptr1 = ptr;
    }
    *ptr = value;

    //max and min
    if (i == 1 || value < min) {
        min = value;
    }
    if (i == 1 || value > max) {
        max = value;
    }

    //statistics calculations
    if (i == 1) {
        oldMean = avg = value;
        oldSum = 0;
    } else {
        avg = oldMean + (value - oldMean)/(float)i;
        newSum = oldSum + (value - oldMean)*(value - avg);

        oldMean = avg;
        oldSum = newSum;

        std = sqrt(newSum/(i - 1));
        p2p = ((max - min) / avg) * 100;
        rms = (std/avg) * 100;
    }

    //Outputs
    *(double *) precord -> vala = avg;
    *(double *) precord -> valb = max;
    *(double *) precord -> valc = min;
    *(double *) precord -> vald = p2p;
    *(double *) precord -> vale = std;
    *(double *) precord -> valf = rms;
    *(long *) precord -> valh = i;

    //Last runthrough
    if (i >= duration)
    {
        *(char *) precord -> valg = 1; //disable scan
        *(char *) precord -> vali = 1; //disable pause
    } else {
        //Iterate
        *ptr++;
    }
    return 0;
}

epicsRegisterFunction(getAverage);
epicsRegisterFunction(resetStats);