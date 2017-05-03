// syntrace.cpp : Synthetic Trace Generation
// Algorithm based on:
// D.Thiebaut, J.L.Wolf, and H.S.Stone, "Synthetic Traces for Trace-Driven Simulation of Cache Memories",
// IEEE Trans. on Computers, vol. 41, 4, April 1992, pp. 388-410

#include "stdafx.h"

typedef struct stack_node *sptr;
struct stack_node
{
	int address;
	struct stack_node *s_next;
	struct stack_node *s_prev;
}stack_rec;

sptr MRU = NULL;
sptr LRU = NULL;
sptr CU = NULL;

sptr s_alloc();
sptr traverse_stack(int);
void update_stack(sptr);

FILE *pgm_seg_file;
FILE *all_addr_file;
FILE *trace_file;

int main()
{
	int num_segmnt;
	int seg_start_addr, seg_end_addr;
	int k, count = 0;

	int num_addr = 0;
	int in_int = 0;
	sptr temp_sptr;
	sptr access_node;
	int i, j;
	float p1, p2;
	float theta = 3.07f; // 3.07f;  // locality parameter
	float A = 19.33f; // 19.33f;     // size of ave. neighbourhood
	int Nt = 2000;        // length of trace
	int index;            // hit index
	int access_addr;
	double u;

	fopen_s(&pgm_seg_file, "pgm.dat", "r");
	fopen_s(&all_addr_file, "in.dat", "r+");
	fopen_s(&trace_file, "trace.dat", "w");

	/* Generate addresses from the program segments */
	fscanf_s(pgm_seg_file, "%d", &num_segmnt);
	for (k = 1; k <= num_segmnt; k++)
	{
		fscanf_s(pgm_seg_file, "%d %d", &seg_start_addr, &seg_end_addr);
		while (seg_start_addr <= seg_end_addr)
		{
			count = count + 1;
			fprintf(all_addr_file, "%d\n", seg_start_addr);
			seg_start_addr = seg_start_addr + 1;
		}
	}
	fclose(pgm_seg_file);
	rewind(all_addr_file);

	printf("Nt = %d\n\n", Nt);

	/* Create and initialize stack */
	num_addr = count;
	for (i = 1; i <= num_addr; i++)
	{
		temp_sptr = s_alloc();
		if (i == 1)
		{
			fscanf_s(all_addr_file, "%d", &in_int);
			temp_sptr->address = in_int;
			MRU = temp_sptr;
			LRU = temp_sptr;
			//printf("Address: %d\n", temp_sptr->address);
		}
		else
		{
			fscanf_s(all_addr_file, "%d", &in_int);
			temp_sptr->address = in_int;
			LRU->s_next = temp_sptr;
			temp_sptr->s_prev = LRU;
			LRU = temp_sptr;
			//printf("Address: %d\n", temp_sptr->address);
		}
	}
	fclose(all_addr_file);

    /* Generation of synthetic addresses */
	fprintf(trace_file, "%d\n", Nt);
	for (j = 1; j <= Nt; j++)
	{
		//printf("********** Nt=%d **********\n", j);
		u = double(rand()) / RAND_MAX;
		//printf("Random number generated is %f\n", u);
		if (u < 1 / theta)
		{
			p1 = round(u / (pow(A, theta) / theta));
			p2 = 1 / (1 - theta);
			//printf("p1=%f, p2=%f\n", p1, p2);
			index = pow(p1, p2);
			//printf("Inside u<theta ---- index = %d\n", index);
		}
		else
		{
			index = round(u*pow(A, theta / (theta - 1)));
			//printf("Inside else ---- index = %d\n", index);
		}

		if (index < 1)
			index = 1;
		if (index > count)
		{
			printf("index = %d # of stack elements = %d *** exiting ***\n", index, count);
			exit(0);
		}

		access_node = traverse_stack(index);
		access_addr = access_node->address;

		printf("access_addr ---- %d\n", access_node->address);
		fprintf(trace_file, "%d  %d\n", access_addr, index);

		update_stack(access_node);
	}
	fclose(trace_file);
	printf("Number of addresses stored in stack = %d\n", count);
}

/* Function: Generate stack_node structure */
sptr s_alloc()
{
	sptr s_ptr;
	s_ptr = (sptr)malloc(sizeof(stack_rec));
	s_ptr->address = 0;
	s_ptr->s_next = NULL;
	s_ptr->s_prev = NULL;

	if (s_ptr)
		return(s_ptr);
	else
	{
		printf("Unable to allocate\n");
		return NULL;
	}
}

/* Function: Traverse stack to find node */
/* corresponding to the generated index  */
sptr traverse_stack(int index)
{
	int i;
	if (index == 1)
	{
		//printf("index=1 ----inside traverse stack\n");
		return MRU;
	}
	CU = MRU;
	for (i = 1; i < index; i++)
		CU = CU->s_next;
	return CU;
}

/* Function: Update stack (index) to MRU position */
void update_stack(sptr access_node)
{
	if (access_node == MRU)
		return;
	else if (access_node == LRU)
	{
		access_node->s_prev->s_next = NULL;
		LRU = access_node->s_prev;
		access_node->s_prev = NULL;
		access_node->s_next = MRU;
		MRU->s_prev = access_node;
		MRU = access_node;
	}
	else
	{
		access_node->s_prev->s_next = access_node->s_next;
		access_node->s_next->s_prev = access_node->s_prev;
		access_node->s_next = MRU;
		access_node->s_prev = NULL;
		MRU->s_prev = access_node;
		MRU = access_node;
	}
}