// serial compare model.cpp : Cache Model for serial compare approach
//

#include "stdafx.h"

#define num_mem_addr_bits 22
#define num_tag_bits 13
#define num_set_bits 9
#define num_byte_bits 1

#define way 8
#define num_set 512

typedef struct tag_RAM_node *tagptr;
struct tag_RAM_node
{
	int tag[num_tag_bits];
	unsigned int v : 1;
}tag_rec;

typedef struct way_node *wptr;
struct way_node
{
	int w_way;
	struct way_node *w_next;
	struct way_node *w_prev;
}way_rec;

tagptr tag_RAM[num_set][way];
int LRU_array[num_set][way];

wptr MRU = NULL;
wptr LRU = NULL;
wptr CU = NULL;

tagptr tag_alloc();
wptr w_alloc();
void decimal_to_binary(int,int *);
void binary_to_decimal(int *, int, int *);
int tag_compare(int[], int[]);
void LRU_stack_update(int, int);

FILE *indata_file;
FILE *outdata_file;

int main()
{
	int *b_mem_addr_ptr;
	int binary[num_mem_addr_bits];
	int tag_bits[num_tag_bits];
	int set_bits[num_set_bits];

	int *set_bits_ptr;
	int *set_ptr;
	int set_num;

	int *tag_bits_ptr;
	
	int test_addr;
	int *test_addr_ptr;
	int access_addr;
	int num_trace_addr;
	int access_index;

	wptr temp_wptr;
	int cmp_res = 0;
	int replace_way;
	int HIT = 0;
	int MISS = 0;
	int num_probe = 0;
	int total_num_probe = 0;
	float ave_num_probe;

	int i, j, k, l, m, n, w, t;
	int flag = 0;

	fopen_s(&indata_file, "../syntrace/trace.dat", "r");
	fopen_s(&outdata_file, "serial_probe.dat", "w");

	/* Create tag_RAM locations and connect to multi-dimensional array */
	for (m = 0; m < num_set; m++)
	{
		for (n = 0; n < way; n++)
			tag_RAM[m][n] = tag_alloc();
	}

	/* Create LRU Stack locations */
	for (m = 0; m < num_set; m++)
	{
		for (n = 0; n < way; n++)
			LRU_array[m][n] = 0;
	}

	/* Create a stack for use in update */
	for (m = 1; m <= way; m++)
	{
		temp_wptr = w_alloc();
		if (m == 1)
		{
			MRU = temp_wptr;
			LRU = temp_wptr;
		}
		else {
			LRU->w_next = temp_wptr;
			temp_wptr->w_prev = LRU;
			LRU = temp_wptr;
		}
	}

	/* Read trace.dat and access tag RAM */
	fscanf_s(indata_file, "%d", &num_trace_addr);
	printf("num_trace_addr: %d\n", num_trace_addr);
	for (i = 1; i < num_trace_addr; i++)
	{
		fscanf_s(indata_file, "%d %d", &access_addr, &access_index);
		b_mem_addr_ptr = binary;
		printf("-------- trace #%d --------\n",i);
		printf("access_addr %d\n", access_addr);
		decimal_to_binary(access_addr, b_mem_addr_ptr);
		printf("\n");
		for (j = num_mem_addr_bits-1; j >=0 ; j--)
			printf("%d", binary[j]);
		printf("\n");
		
		/* Extract and store tag bits */
		printf("tag bits\n");
		for (k = 0; k < num_tag_bits; k++)
		{
			tag_bits[num_tag_bits-1-k] = *(b_mem_addr_ptr + num_mem_addr_bits - 1 - k);
			printf("%d", tag_bits[num_tag_bits - 1 - k]);
		}

		/* Extract and store set bits */
		printf("\nset bits\n");
		for (l = 0; l < num_set_bits; l++)
		{
			set_bits[num_set_bits-1-l] = *(b_mem_addr_ptr + num_mem_addr_bits - 1 - k - l);
			printf("%d", set_bits[num_set_bits - 1 - l]);
		}
		printf("\n");

		test_addr_ptr = &test_addr;
		binary_to_decimal(b_mem_addr_ptr, num_mem_addr_bits, test_addr_ptr);
		//printf("addrress check %d\n", *test_addr_ptr);

		tag_bits_ptr = tag_bits;

		/* Convert set bits to set number and probe set */
		set_bits_ptr = set_bits;
		set_ptr = &set_num;
		binary_to_decimal(set_bits_ptr, num_set_bits, set_ptr);
		printf("set_num: %d\n", *set_ptr);

		printf("Before probing");
		for (w = 0; w < way; w++)
		{
			printf("\nProbe way: %d\n",w);
			printf("tag: ");
			for (t = 0; t < num_tag_bits; t++)
				printf("%d", tag_RAM[*set_ptr][w]->tag[t]);
			if (tag_RAM[*set_ptr][w]->v)
			{
				cmp_res = tag_compare(tag_bits, tag_RAM[*set_ptr][w]->tag);
				printf("\nTag compare: %d\n", cmp_res);
			}
			num_probe++;
			if (cmp_res)
			{
				HIT = 1;
				printf("\nHIT!\n");
				LRU_stack_update(w, set_num);
				total_num_probe = total_num_probe + num_probe;
				fprintf(outdata_file, "%d\n", num_probe);
				break;
			}
		}
		
		printf("\n");
		if (!HIT)
		{
			MISS = 1;
			printf("MISS!\n");
			for (w = 0; w < way; w++)
			{
				if (!tag_RAM[*set_ptr][w]->v)
				{
					printf("found an empty block: %d\n", w);
					flag = 1;
					printf("Tag RAM update\n");
					for (k = 0; k < num_tag_bits; k++)
						tag_RAM[*set_ptr][w]->tag[k] = tag_bits[k];
					tag_RAM[*set_ptr][w]->v = 1;
					LRU_array[*set_ptr][w] = w;
					printf("LRU stack update\n");
					LRU_stack_update(w, set_num);
					break;
				}
			}
			if (!flag)
			{
				replace_way = LRU_array[*set_ptr][way - 1];
				printf("Replacing LRU block %d\n", replace_way); 
				for (k = 0; k < num_tag_bits; k++)
					tag_RAM[*set_ptr][w]->tag[k] = tag_bits[k];
				tag_RAM[*set_ptr][replace_way]->v = 1;
				LRU_stack_update(replace_way, set_num);
			}
			num_probe = 0; // Consider only HIT for ave. num of probes
			total_num_probe = total_num_probe + num_probe;
			fprintf(outdata_file, "%d\n", num_probe);
		}
		// Intialize variables before each new trace address
		num_probe = 0;
		HIT = 0;
		MISS = 0;
		flag = 0;
	}
	ave_num_probe = float(total_num_probe) / float(num_trace_addr);
	fprintf(outdata_file, "Average number of probes = %f\n", ave_num_probe);
	fclose(indata_file);
	fclose(outdata_file);
}

/* Function: Generate a location in the tag RAM */
tagptr tag_alloc()
{
	tagptr tag_ptr;
	tag_ptr = (tagptr)calloc(1, sizeof(tag_rec)); // Allocate and intialize to zero
	
	if (tag_ptr)
		return(tag_ptr);
	else
	{
		printf("Cannot allocate\n");
		return NULL;
	}
}

/* Function: Generate way node structure */
wptr w_alloc()
{
	wptr w_ptr;
	w_ptr = (wptr)malloc(sizeof(way_rec));
	w_ptr->w_way = 0;
	w_ptr->w_next = NULL;
	w_ptr->w_prev = NULL;

	if (w_ptr)
		return(w_ptr);
	else
	{
		printf("Cannot allocate\n");
		return NULL;
	}
}

/* Function: Convert decimal to binary */
void decimal_to_binary(int decimal, int *b_mem_addr_ptr)
{
	int i = 0;
	while (decimal != 0)
	{
		*(b_mem_addr_ptr + i) = decimal % 2;
		//printf("%d", *(b_mem_addr_ptr + i));
		i++;
		decimal = decimal / 2;
	}
	while (i < num_mem_addr_bits) /* to pad with 0's */
	{
		*(b_mem_addr_ptr + i) = 0;
		i++;
	}
}

/* Function: Convert binary to decimal */
void binary_to_decimal(int *bits_arr_ptr, int num_bits, int *decimal_ptr)
{
	int i;
	*decimal_ptr = 0;
	for (i = 0; i < num_bits; i++)
		*decimal_ptr = *decimal_ptr + (*(bits_arr_ptr + i)*pow(2, i));
}

/* Function: Tag compare */
int tag_compare(int tag_in[], int tag_RAM[])
{
	int i;
	int cmp_res;
	for (i = 0; i < num_tag_bits; i++)
	{
		if (tag_in[i] != tag_RAM[i])
		{
			cmp_res = 0;
			return cmp_res;
		}
		else
			cmp_res = 1;
	}
	return cmp_res;
}

/* Function: Copy current set from LRU_array to stack and update */
void LRU_stack_update(int w, int set_num)
{
	int i;
	CU = MRU;
	for (i = 0; i < way; i++)
	{
		CU->w_way = LRU_array[set_num][i];
		printf("LRU_array[set_num][%d]=%d\n", i, LRU_array[set_num][i]);
		CU = CU->w_next;
	}
	CU = MRU;
	for (i = 0; i < way; i++)
	{
		if (CU->w_way == w && CU->w_prev == NULL) // MRU
		{
			printf("HIT/REPLACE block is MRU\n");
			return;
		}
		else if (CU->w_way == w && CU->w_next == NULL) // LRU
		{
			printf("HIT/REPLACE block is LRU ----\n");
			CU->w_prev->w_next = NULL;
			LRU = CU->w_prev;
			CU->w_prev = NULL;
			CU->w_next = MRU;
			MRU->w_prev = CU;
			MRU = CU;
			break;
		}
		else if (CU->w_way == w)
		{
			printf("HIT/REPLACE block is between MRU and LRU ****\n");
			CU->w_prev->w_next = CU->w_next;
			CU->w_next->w_prev = CU->w_prev;
			CU->w_next = MRU;
			CU->w_prev = NULL;
			MRU->w_prev = CU;
			MRU = CU;
			break;
		}
		CU = CU->w_next;
	}
	CU = MRU;
	for (i = 0; i < way; i++)
	{
		LRU_array[set_num][i] = CU->w_way;
		printf("LRU_array[set_num][%d]=%d\n", i, LRU_array[set_num][i]);
		CU = CU->w_next;
	}
}

