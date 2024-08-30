#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h> 
#include <time.h>

typedef struct {
	char* array[64];
	int length;
} string_array;

// Defining the structure that will be used for memory of doubles
typedef struct {
	char* name;
	double value;
} memory_line;

memory_line* memory[64];

/* I honestly don't remember what these are for
int memory_size = 0;
int memory_count = 0;
*/

int extract_keywords(char* line, string_array *token_stream);

int process_keywords(string_array keywords);

void assign_variable(char* name, double value);

void print_strings(string_array* strings);