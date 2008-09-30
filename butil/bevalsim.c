#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>
#include <time.h>

#include "../blib/BLibDefinitions.h"
#include "../blib/BError.h"
#include "../blib/AlignEntries.h"
#include "bevalsim.h"

#define Name "bevalsim"
#define COUNT_ROTATE_NUM 100000

/* Parses a balign file resulting from usin reads generated by
 * bgeneratereads to give accuracy statistics for the mapping.
 * */

int main(int argc, char *argv[]) 
{
	char baf[MAX_FILENAME_LENGTH]="\0";
	char outputID[MAX_FILENAME_LENGTH]="\0";

	if(argc == 3) {

		/* Get cmd line options */
		strcpy(baf, argv[1]);
		strcpy(outputID, argv[2]);

		/* Check cmd line options */

		/* Run program */
		Evaluate(baf, outputID);

		/* Terminate */
		fprintf(stderr, "%s", BREAK_LINE);
		fprintf(stderr, "Terminating successfully.\n");
		fprintf(stderr, "%s", BREAK_LINE);
	}
	else {
		fprintf(stderr, "Usage: %s [OPTIONS]\n", Name);
		fprintf(stderr, "\t<bfast aligned file>\n");
		fprintf(stderr, "\t<output id>\n");
	}
	return 0;
}

void ReadTypeInitialize(ReadType *r)
{
	r->strand=0;
	r->contig=0;
	r->pos=0;
	r->space=NTSpace;
	r->pairedEnd=0;
	r->pairedEndLength=0;
	r->readLength=0;
	r->whichReadVariants=0;
	r->startIndel=0;
	r->indelLength=0;
	r->numSNPs=0;
	r->numErrors=0;
	r->deletionLength=0;
	r->insertionLength=0;
	r->aContig=0;
	r->aPos=0;
	r->aStrand=0;
}

void ReadTypeCopy(ReadType *dest,
		ReadType *src)
{
	/* Only copy meta data */ 
	dest->strand=src->strand;
	dest->contig=src->contig;
	dest->pos=src->pos;
	dest->space=src->space;
	dest->pairedEnd=src->pairedEnd;
	dest->pairedEndLength=src->pairedEndLength;
	dest->readLength=src->readLength;
	dest->whichReadVariants=src->whichReadVariants;
	dest->startIndel=src->startIndel;
	dest->indelLength=src->indelLength;
	dest->numSNPs=src->numSNPs;
	dest->numErrors=src->numErrors;
	dest->deletionLength=src->deletionLength;
	dest->insertionLength=src->insertionLength;
}

void ReadTypePrint(ReadType *r, FILE *fp)
{
	fprintf(fp, "strand=%c\n", r->strand);
	fprintf(fp, "contig=%d\n", r->contig);
	fprintf(fp, "pos=%d\n", r->pos);
	fprintf(fp, "space=%d\n", r->space);
	fprintf(fp, "pairedEnd=%d\n", r->pairedEnd);
	fprintf(fp, "pairedEndLength=%d\n", r->pairedEndLength);
	fprintf(fp, "readLength=%d\n", r->readLength);
	fprintf(fp, "whichReadVariants=%d\n", r->whichReadVariants);
	fprintf(fp, "startIndel=%d\n", r->startIndel);
	fprintf(fp, "indelLength=%d\n", r->indelLength);
	fprintf(fp, "numSNPs=%d\n", r->numSNPs);
	fprintf(fp, "numErrors=%d\n", r->numErrors);
	fprintf(fp, "deletionLength=%d\n", r->deletionLength);
	fprintf(fp, "insertionLength=%d\n", r->insertionLength);
}

int ReadTypeCompare(ReadType *a,
		ReadType *b)
{
	/* Only compare meta data */ 
	/* Nice use of if, else if, and else statements */
	if(a->space != b->space) {
		return (a->space < b->space)?-1:1;
	}
	else if(a->pairedEnd != b->pairedEnd) {
		return (a->pairedEnd < b->pairedEnd)?-1:1;
	}
	else if(a->pairedEndLength != b->pairedEndLength) {
		return (a->pairedEndLength < b->pairedEndLength)?-1:1;
	}
	else if(a->readLength != b->readLength) {
		return (a->readLength < b->readLength)?-1:1;
	}
	else if(a->whichReadVariants != b->whichReadVariants) {
		return (a->whichReadVariants < b->whichReadVariants)?-1:1;
	}
	else if(a->indelLength != b->indelLength) {
		return (a->indelLength < b->indelLength)?-1:1;
	}
	else if(a->numSNPs != b->numSNPs) {
		return (a->numSNPs < b->numSNPs)?-1:1;
	}
	else if(a->numErrors != b->numErrors) {
		return (a->numErrors < b->numErrors)?-1:1;
	}
	else if(a->deletionLength != b->deletionLength) {
		return (a->deletionLength < b->deletionLength)?-1:1;
	}
	else if(a->insertionLength != b->insertionLength) {
		return (a->insertionLength < b->insertionLength)?-1:1;
	}
	else {
		return 0;
	}
}

int ReadTypeReadFromBAF(ReadType *r, 
		FILE *fp)
{
	char *FnName = "ReadTypeReadFromBAF";
	AlignEntries a;
	int i;
	char r1[SEQUENCE_LENGTH]="\0";
	char r2[SEQUENCE_LENGTH]="\0";

	/* Initialize */
	AlignEntriesInitialize(&a);

	/* Read in align entries */
	if(EOF==AlignEntriesRead(&a, fp, PairedEndDoesNotMatter, SpaceDoesNotMatter, BALIGN_DEFAULT_OUTPUT)) {
		return EOF;
	}
	/* There should be only one */
	if(a.numEntriesOne > 1 ||
			(a.pairedEnd == 1 && a.numEntriesOne > 1)) {
		PrintError(FnName,
				NULL,
				"There was more than one alignment for a given read",
				Exit,
				OutOfRange);
	}

	r->aContig = a.entriesOne->contig;
	r->aPos = a.entriesOne->position;
	r->aStrand = a.entriesOne->strand;
	/* Convert into read type */
	int tempPairedEnd = a.pairedEnd;
	r->space = a.space;
	/* Get the rest from read name */
	if(tempPairedEnd == 0) {
		if(EOF == sscanf(a.readName, 
					">strand=%c_contig=%d_pos=%d_pe=%d_pel=%d_rl=%d_wrv=%d_si=%d_il=%d_r1=%s",
					&r->strand,
					&r->contig,
					&r->pos,
					&r->pairedEnd,
					&r->pairedEndLength,
					&r->readLength,
					&r->whichReadVariants,
					&r->startIndel,
					&r->indelLength,
					r1)) {
			PrintError(FnName,
					a.readName,
					"Could not parse read name (0)",
					Exit,
					OutOfRange);
		}
	}
	else {
		if(EOF == sscanf(a.readName, 
					">strand=%c_contig=%d_pos=%d_pe=%d_pel=%d_rl=%d_wrv=%d_si=%d_il=%d_r1=%s_r2=%s",
					&r->strand,
					&r->contig,
					&r->pos,
					&r->pairedEnd,
					&r->pairedEndLength,
					&r->readLength,
					&r->whichReadVariants,
					&r->startIndel,
					&r->indelLength,
					r1,
					r2)) {
			PrintError(FnName,
					a.readName,
					"Could not parse read name (1)",
					Exit,
					OutOfRange);
		}
	}
	/* Parse r1 and r2 */
	assert(r->pairedEnd == tempPairedEnd);
	assert(r->readLength == (int)strlen(r1));
	assert(r->pairedEnd == 0 || r->readLength == (int)strlen(r2));
	r->numSNPs = 0;
	r->numErrors = 0;
	r->deletionLength = 0;
	r->insertionLength = 0;
	for(i=0;i<r->readLength;i++) {
		switch(r1[i]) {
			case '0':
				/* Default */
				break;
			case '1':
				/* Insertion */
				r->insertionLength++;
				break;
			case '2':
				/* SNP */
				r->numSNPs++;
				break;
			case '3':
				/* Error */
				r->numErrors++;
				break;
			case '4':
				/* InsertionAndSNP */
				r->insertionLength++;
				r->numSNPs++;
				break;
			case '5':
				/* InsertionAndError */
				r->insertionLength++;
				r->numErrors++;
				break;
			case '6':
				/* SNPAndError */
				r->numSNPs++;
				r->numErrors++;
				break;
			case '7':
				/* InsertionSNPAndError */
				r->insertionLength++;
				r->numSNPs++;
				r->numErrors++;
				break;
			default:
				PrintError(FnName,
						"r1[i]",
						"Could not understand type",
						Exit,
						OutOfRange);
		}
		if(r->pairedEnd == 1) {
			switch(r2[i]) {
				case '0':
					/* Default */
					break;
				case '1':
					/* Insertion */
					r->insertionLength++;
					break;
				case '2':
					/* SNP */
					r->numSNPs++;
					break;
				case '3':
					/* Error */
					r->numErrors++;
					break;
				case '4':
					/* InsertionAndSNP */
					r->insertionLength++;
					r->numSNPs++;
					break;
				case '5':
					/* InsertionAndError */
					r->insertionLength++;
					r->numErrors++;
					break;
				case '6':
					/* SNPAndError */
					r->numSNPs++;
					r->numErrors++;
					break;
				case '7':
					/* InsertionSNPAndError */
					r->insertionLength++;
					r->numSNPs++;
					r->numErrors++;
					break;
				default:
					PrintError(FnName,
							"r2[i]",
							"Could not understand type",
							Exit,
							OutOfRange);
			}
		}
	}
	if(r->startIndel >= 0 && r->insertionLength == 0) {
		r->deletionLength = r->indelLength;
	}

	/* Delete align entries */
	AlignEntriesFree(&a);

	return 1;
}

void StatInitialize(Stat *s, 
		ReadType *r)
{
	s->numCorrectlyAligned[0]=0;
	s->numCorrectlyAligned[1]=0;
	s->numCorrectlyAligned[2]=0;
	s->numCorrectlyAligned[3]=0;
	s->numCorrectlyAligned[4]=0;
	ReadTypeCopy(&s->r, r);
	s->numReads=0;
}

void StatPrint(Stat *s, FILE *fp)
{
	fprintf(fp, "%d %d %d %d %d %d ",
			s->numReads,
			s->numCorrectlyAligned[0],
			s->numCorrectlyAligned[1],
			s->numCorrectlyAligned[2],
			s->numCorrectlyAligned[3],
			s->numCorrectlyAligned[4]);
	fprintf(fp, "%d %d %d %d %d %d %d %d %d\n",
			s->r.space,
			s->r.pairedEnd,
			s->r.pairedEndLength,
			s->r.readLength,
			s->r.indelLength,
			s->r.numSNPs,
			s->r.numErrors,
			s->r.deletionLength,
			s->r.insertionLength);
}

void StatAdd(Stat *s, ReadType *r)
{
	int diff;
	if(r->strand == r->aStrand &&
			r->contig == r->aContig) {
		diff = (r->pos > r->aPos)?(r->pos - r->aPos):(r->aPos - r->pos);

		/* Update */
		if(diff <= 10000) {
			s->numCorrectlyAligned[4]++;
			if(diff <= 1000) {
				s->numCorrectlyAligned[3]++;
				if(diff <= 100) {
					s->numCorrectlyAligned[2]++;
					if(diff <= 10) {
						s->numCorrectlyAligned[1]++;
						if(diff <= 0) {
							s->numCorrectlyAligned[0]++;
						}
					}
				}
			}
		}
	}
	s->numReads++;
}

void StatsInitialize(Stats *s) 
{
	s->stats=NULL;
	s->numStats=0;
}

void StatsPrintHeader(FILE *fp)
{
	fprintf(fp, "# COL | Description\n");
	fprintf(fp, "# 0    | number of reads\n");
	fprintf(fp, "# 1    | number of correctly aligned within 0 bases\n");
	fprintf(fp, "# 2    | number of correctly aligned within 10 bases\n");
	fprintf(fp, "# 3    | number of correctly aligned within 100 bases\n");
	fprintf(fp, "# 4    | number of correctly aligned within 1000 bases\n");
	fprintf(fp, "# 5    | number of correctly aligned within 10000 bases\n");
	fprintf(fp, "# 6    | space\n");
	fprintf(fp, "# 7    | paired end\n");
	fprintf(fp, "# 8    | paired end length\n");
	fprintf(fp, "# 9    | read length\n");
	fprintf(fp, "# 10   | indel length\n");
	fprintf(fp, "# 11   | number of snps\n");
	fprintf(fp, "# 12   | number of errors\n");
	fprintf(fp, "# 13   | deletion length\n");
	fprintf(fp, "# 14   | insertion length\n");
}

void StatsPrint(Stats *s, FILE *fp)
{
	int32_t i;
	StatsPrintHeader(fp);
	for(i=0;i<s->numStats;i++) {
		StatPrint(&s->stats[i], fp);
	}
}

void StatsAdd(Stats *s, ReadType *r)
{
	int32_t i;
	char *FnName="StatsAdd";

	/* Check if it fits somewhere */
	for(i=0;i<s->numStats;i++) {
		if(ReadTypeCompare(r, &s->stats[i].r)==0) {
			/* Add to current */
			StatAdd(&s->stats[i], r);
			return; /* Get out of here */
		}
	}
	/* Otherwise start a new start entry */
	s->numStats++;
	s->stats = realloc(s->stats, sizeof(Stat)*s->numStats);
	if(NULL==s->stats) {
		PrintError(FnName,
				"s->stats",
				"Could not allocate memory",
				Exit,
				MallocMemory);
	}
	/* Initialize */
	StatInitialize(&s->stats[s->numStats-1], r);
	/* Add */
	StatAdd(&s->stats[s->numStats-1], r);
}

void StatsDelete(Stats *s)
{
	free(s->stats);
	s->stats=NULL;
	s->numStats=0;
}

void Evaluate(char *baf,
		char *outputID)
{
	char *FnName="Evaluate";
	FILE *fpIn;
	FILE *fpOut;
	ReadType r;
	Stats s;
	int32_t count;
	char outputFileName[MAX_FILENAME_LENGTH]="\0";

	/* Open the baf file */
	if(!(fpIn=fopen(baf, "rb"))) {
		PrintError(FnName,
				baf,
				"Could not open file for reading",
				Exit,
				OpenFileError);
	}

	ReadTypeInitialize(&r);
	StatsInitialize(&s);

	count = 0;
	fprintf(stderr, "Currently on:\n%d", 0);
	while(EOF != ReadTypeReadFromBAF(&r, fpIn)) {
		count++;
		if(count % COUNT_ROTATE_NUM == 0) {
			fprintf(stderr, "\r%d[%d]", 
					count,
					s.numStats
					);
		}

		/* Process the read */
		StatsAdd(&s, &r);

		/* Reinitialize */
		ReadTypeInitialize(&r);
	}
	fprintf(stderr, "\r%d\n", count);
	
	/* Create output file name */
	sprintf(outputFileName, "%s.evalsim.%s.txt",
			PROGRAM_NAME,
			outputID);
	/* Open the output file */
	if(!(fpOut=fopen(outputFileName, "wb"))) {
		PrintError(FnName,
				outputFileName,
				"Could not open file for writing",
				Exit,
				WriteFileError);
	}

	/* Print Stats */
	StatsPrint(&s, fpOut);

	/* Delete Stats */
	StatsDelete(&s);

	/* Close the files */
	fclose(fpIn);
	fclose(fpOut);
}
