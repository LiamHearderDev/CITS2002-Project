#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h> 
#include <time.h>
#include <ctype.h>
#include <stdbool.h>
#include <math.h>

// This is to define how long a memory array could be.
#define MEMORY_LENGTH 64

// A structure for storing an array of strings.
// This is necessary to keep track of the size of string arrays because we cannot
// check the length of a string array if it is defined as `char** names` or passed
// as a function parameter. This struct allows us to contain the array AND the
// length in a single data type without data loss.
typedef struct {
	int length;
	int scope; // currently unused. Tbh, this is just to avoid byte padding warnings lol.
	char* array[32];
} string_array;

// The structure used for storing variables in memory for later retrieval.
// This is necessary for storing a dynamic number of variables in memory.
// This struct should NEVER be used in isolation. Always use a pre-defined
// array (such as `memory_cache[n]`, or `functions_cache[n].memory_line`).
typedef struct {
	char* name;
	double value;
} memory_line;

// Global variable containing an array of memory_line structures.
// This is used to store global variables in .ml programs.
memory_line* memory_cache[MEMORY_LENGTH];

// A structure containing information about ml functions.
// @name = The name of the function.
// @lines = An array of strings. Each string is a line from the .ml program within this function.
// @local_memory = An array of memory_line's which serves as local memory for the function.
typedef struct {
	char* name;
	string_array lines; // This stores the lines which will be processed when the function is called
	memory_line* local_memory[MEMORY_LENGTH]; // Stores local memory (parameters)
	int param_count;
} function;

// Global variable containing an array of function structures.
// This is used to store functions defined by .ml programs.
function* functions_cache[MEMORY_LENGTH];


// primary functions

int process_keywords(string_array keywords, memory_line* memory[64], int line_no);
int extract_keywords(char* line, string_array *token_stream);
void extract_functions(string_array* file_lines);


// processing functions

void ml_assign_variable(char* name, double value, memory_line* memory[64]);

bool ml_check_variable(const char* name, memory_line* memory[64]);
double ml_retrieve_variable(const char* name, memory_line* memory[64]);

bool ml_check_function(const char* name);
function* ml_retrieve_function(const char* name);

void ml_print(string_array keywords, memory_line* memory[64]);

void calc_expression(string_array keywords, int expr_start_pos, int expr_end_pos, char* stream, memory_line* memory[64]);

void ml_add_function(function* function_info);

void dtos(double value, char* buffer);

// Removes an input char from the input stream.
void remove_character(char to_remove, char* stream);

void remove_strings_from_array(string_array* str_array, int start_pos, int end_pos);


// debugging functions

void print_strings(string_array* strings);
