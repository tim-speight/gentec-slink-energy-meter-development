#include <string.h>
#include <aSubRecord.h>
#include <registryFunction.h>
#include <epicsExport.h>
#include <stdio.h> 

#define MAX_STRING_SIZE 40

// Lookup table: index 0–41 ➜ string labels
static const char *keys[] = {
    "1pJ", "3pJ", "10pJ", "30pJ", "100pJ", "300pJ",
    "1nJ", "3nJ", "10nJ", "30nJ", "100nJ", "300nJ",
    "1uJ", "3uJ", "10uJ", "30uJ", "100uJ", "300uJ",
    "1mJ", "3mJ", "10mJ", "30mJ", "100mJ", "300mJ",
    "1J", "3J", "10J", "30J", "100J", "300J",
    "1kJ", "3kJ", "10kJ", "30kJ", "100kJ", "300kJ",
    "1megJ", "3megJ", "10megJ", "30megJ", "100megJ", "300megJ"
};
#define NUM_KEYS (sizeof(keys)/sizeof(keys[0]))

/// 🔁 Function 1: Convert integer to string
long energyIntToStr(aSubRecord *precord)  {
long idx = (precord->a ? *((long*)precord->a) : -1);

    char *outstr = (char *)precord->vala;
    if(!outstr) return -1;

    if(idx >= 0 && idx < NUM_KEYS) {
        strcpy(outstr, keys[idx]);
    } else {
        strcpy(outstr, "INVALID");
    }
    return 0;
}
/// 🔁 Function 2: Convert string to index
long energyStrToInt(aSubRecord *precord) {
    char *instr = (char *)precord->a;
    long *outidx = (long *)precord->vala;

    if(!instr || !outidx) return -1;

    for(int idx=0; idx < NUM_KEYS; idx++) {
        if(strcmp(instr, keys[idx]) == 0) {
            *outidx = idx;
            return 0;
        }
    }
    *outidx = -1; // Not found 
    return 0;
}

// Register both with EPICS
epicsRegisterFunction(energyIntToStr);
epicsRegisterFunction(energyStrToInt);
