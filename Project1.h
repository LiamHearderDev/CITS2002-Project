#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h> 
#include <time.h>
#include <ctype.h>
#include <stdbool.h>
#include <math.h>

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

// Structure for information about ml defined functions
typedef struct {
	char* name;
	string_array lines; // This stores the lines which will be processed when the function is called
	memory_line* local_memory[64]; // Stores local memory (parameters)
} function;

function* functions_cache[64];

/* I honestly don't remember what these are for
int memory_size = 0;
int memory_count = 0;
*/


// main functions

int process_keywords(string_array keywords, memory_line* memory[64], int line_no);
int extract_keywords(char* line, string_array *token_stream);
void extract_functions(string_array* file_lines);


// processing functions

void ml_assign_variable(char* name, double value, memory_line* memory[64]);

bool ml_check_variable(const char* name, memory_line* memory[64]);
double ml_retrieve_variable(char* name, memory_line* memory[64]);

bool ml_check_function(const char* name);
function* ml_retrieve_function(const char* name);

void ml_print(string_array keywords, memory_line* memory[64]);

void calc_expression(string_array keywords, int expr_start_pos, int expr_end_pos, char* stream, memory_line* memory[64]);

void ml_add_function(function* function_info);

void dtos(double value, char* buffer);

// Removes an input char from the input stream.
void remove_character(char to_remove, char* stream);


// debugging functions

void print_strings(string_array* strings);
