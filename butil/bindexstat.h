#ifndef BINDEXSTAT_H_
#define BINDEXSTAT_H_

#include "../blib/RGIndex.h"
#include "../blib/RGBinary.h"
#include "../blib/RGMatch.h"

void PrintSummary(RGIndex*, RGBinary*);
void PrintHistogram(RGIndex*, RGBinary*, int, int, FILE*);
void GetMatchesFromChrPos(RGIndex*, RGBinary*, uint32_t, uint32_t, RGMatch*);

#endif
