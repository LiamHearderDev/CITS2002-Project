#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h> 
#include <time.h>
#include <ctype.h>

typedef struct {
	int length;
	int scope; // currently unused. Tbh, this is just to avoid byte padding warnings lol
	char* array[32];
} string_array;

// Defining the structure that will be used for memory of doubles
typedef struct {
	char* name;
	double value;
} memory_line;

memory_line* memory_cache[64];

/* I honestly don't remember what these are for
int memory_size = 0;
int memory_count = 0;
*/


// main functions
int process_keywords(string_array keywords);
int extract_keywords(char* line, string_array *token_stream);

// processing functions
void ml_assign_variable(char* name, double value);
void ml_print(string_array keywords);
char* calc_expression(string_array keywords, int expr_start_pos, int expr_end_pos);

// debugging functions
void print_strings(string_array* strings);
