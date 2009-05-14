#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include <limits.h>
#include <float.h>
#include "../blib/BError.h"
#include "../blib/AlignedEntry.h"
#include "../blib/AlignedEnd.h"
#include "../blib/AlignedRead.h"
#include "../blib/BLib.h"
#include "../blib/ScoringMatrix.h"
#include "Definitions.h"
#include "Filter.h"

/* TODO */
int FilterAlignedRead(AlignedRead *a,
		int algorithm,
		int minScore,
		int minQual,
		int startContig,
		int startPos,
		int endContig,
		int endPos,
		int maxMismatches,
		int maxColorErrors,
		int minDistancePaired,
		int maxDistancePaired,
		int useDistancePaired,
		int contigAbPaired,
		int inversionsPaired,
		int unpaired)
{
	char *FnName="FilterAlignedRead";
	int foundType;
	int32_t *foundTypes=NULL;
	AlignedRead tmpA;
	int32_t i, j, ctr;
	int32_t best, bestIndex, numBest;
	int32_t curContigDistance, curPositionDistance;

	AlignedReadInitialize(&tmpA);

	/* We should only modify "a" if it is going to be reported */ 
	/* Copy in case we do not find anything to report */
	AlignedReadCopy(&tmpA, a);

	if(NoFiltering != algorithm) {
		/* Filter each alignment individually */
		for(i=0;i<tmpA.numEnds;i++) {
			for(j=0;j<tmpA.ends[i].numEntries;j++) {
				/* Check if we should filter */
				if(0<FilterAlignedEntry(&tmpA.ends[i].entries[j],
							tmpA.space,
							minScore,
							minQual,
							startContig,
							startPos,
							endContig,
							endPos,
							maxMismatches,
							maxColorErrors)) {
					/* Copy end here */
					if(j < tmpA.ends[i].numEntries-1) {
						AlignedEntryCopy(&tmpA.ends[i].entries[j], 
								&tmpA.ends[i].entries[tmpA.ends[i].numEntries-1]);
					}
					AlignedEndReallocate(&tmpA.ends[i], tmpA.ends[i].numEntries-1);
					/* Since we removed, do not incrememt */
					j--;
				}
			}
		}
	}

	foundType=NoneFound;
	foundTypes=malloc(sizeof(int32_t)*tmpA.numEnds);
	if(NULL == foundTypes) {
		PrintError(FnName,
				"foundTypes",
				"Could not allocate memory",
				Exit,
				MallocMemory);
	}
	for(i=0;i<tmpA.numEnds;i++) {
		foundTypes[i]=NoneFound;
	}

	/* Pick alignment for each end individually (is this a good idea?) */
	for(i=0;i<tmpA.numEnds;i++) {
		/* Choose each end */
		switch(algorithm) {
			case NoFiltering:
			case AllNotFiltered:
				foundTypes[i] = (0<tmpA.ends[i].numEntries)?Found:NoneFound;
				break;
			case Unique:
				foundTypes[i]=(1==tmpA.ends[i].numEntries)?Found:NoneFound;
				break;
			case BestScore:
			case BestScoreAll:
				best = INT_MIN;
				bestIndex = -1;
				numBest = 0;
				for(j=0;j<tmpA.ends[i].numEntries;j++) {
					if(best < tmpA.ends[i].entries[j].score) {
						best = tmpA.ends[i].entries[j].score;
						bestIndex = j;
						numBest = 1;
					}
					else if(best == tmpA.ends[i].entries[j].score) {
						numBest++;
					}
				}
				if(BestScore == algorithm &&
						1 == numBest) {
					foundTypes[i] = Found;
					/* Copy to front */
					AlignedEntryCopy(&tmpA.ends[i].entries[0], 
							&tmpA.ends[i].entries[bestIndex]);
					AlignedEndReallocate(&tmpA.ends[i], 1);
				}
				else if(BestScoreAll == algorithm) {
					foundTypes[i] = Found;
					ctr=0;
					for(j=0;j<tmpA.ends[i].numEntries;j++) {
						if(tmpA.ends[i].entries[j].score == best) {
							if(ctr != j) {
								AlignedEntryCopy(&tmpA.ends[i].entries[ctr], 
										&tmpA.ends[i].entries[j]);
							}
							ctr++;
						}
					}
					assert(ctr == numBest);
					AlignedEndReallocate(&tmpA.ends[i], numBest);
				}
				break;
			default:
				PrintError(FnName,
						"algorithm",
						"Could not understand algorithm",
						Exit,
						OutOfRange);
				break;
		}
		/* Free if not found */
		if(NoneFound == foundTypes[i]) {
			AlignedEndReallocate(&tmpA.ends[i],
					0);
		}
	}

	if(1 == tmpA.numEnds) {
		foundType=foundTypes[0];
	}
	else if(2 == tmpA.numEnds &&
			1 == a->ends[0].numEntries &&
			1 == a->ends[1].numEntries) {
		foundType=(Found==foundTypes[0] && Found==foundTypes[1])?Found:NoneFound;

		if(Found == foundType) {
			if(1 == useDistancePaired) {
				curContigDistance = a->ends[1].entries[0].contig - a->ends[0].entries[0].contig;
				curPositionDistance = a->ends[1].entries[0].position - a->ends[0].entries[0].position;

				if(0 != curContigDistance ||
						curPositionDistance < minDistancePaired ||
						maxDistancePaired < curPositionDistance) {
					if(a->ends[0].entries[0].strand == a->ends[1].entries[0].strand) {
						if(1 == contigAbPaired) {
							/* Same strand - contig abnormality */
							foundType = ContigAb;
						}
						else {
							foundType = NoneFound;
						}
					}
					else {
						if(1 == inversionsPaired) { 
							/* Different strand - inversion */
							foundType = Inversion;
						}
						else {
							foundType = NoneFound;
						}
					}
				}
			}
		}
	}
	else {
		/* Call found if all have been found */
		foundType=Found;
		for(i=0;Found==foundType && i<tmpA.numEnds;i++) {
			if(NoneFound == foundTypes[i]) {
				foundType=NoneFound;
			}
		}
	}
	/* See if we should check for unpaired alignments */
	if(1 == unpaired && NoneFound == foundType) {
		/* Call unpaired if at least one is found */
		foundType=NoneFound;
		for(i=0;NoneFound==foundType && i<tmpA.numEnds;i++) {
			if(Found == foundTypes[i]) {
				foundType=Found;
			}
		}
		if(Found == foundType) {
			foundType = Unpaired;
		}
	}

	/* If we found, then copy back */
	if(NoneFound != foundType) {
		AlignedReadFree(a);
		AlignedReadCopy(a, &tmpA);
	}
	AlignedReadFree(&tmpA);
	free(foundTypes);

	return foundType;
}

/* TODO */
int FilterAlignedEntry(AlignedEntry *a,
		int space,
		int minScore,
		int minQual,
		int startContig,
		int startPos,
		int endContig,
		int endPos,
		int maxMismatches,
		int maxColorErrors)
{
	int32_t numMismatches, numColorErrors;
	numMismatches=numColorErrors=0;
	/* Check if the alignment has at least the minimum score */
	if(a->score < minScore) {
		return 1;
	}
	/* Check if the alignment has at least the minimum quality */
	if(a->mappingQuality < minQual) {
		return 2;
	}
	/* Check mismatches */
	numMismatches = GetNumMismatchesInAlignedEntry(a);
	if(maxMismatches < numMismatches) {
		return 3;
	}
	if(ColorSpace == space) {
		/* Check color errors */
		numColorErrors = GetNumColorErrorsInAlignedEntry(a, space);
		if(maxColorErrors < numColorErrors) {
			return 4;
		}
	}
	return 0;
}

