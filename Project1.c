
//  CITS2002	Project 1 2024
//  Student1:   23074422   Liam-Hearder
//  Student2:   23782402   Tanmay-Arggarwal
//  Platform:   Linux


/*

 Sections Included in this file:
  1. includes
  2. macros
  3. structure definition
  4. global variable definitions
  5. function definitions
  6. function implementations
  
*/



// ~~~~~~~~~~ 1. Includes ~~~~~~~~~~ //

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <stdbool.h>
#include <fcntl.h>
#include <unistd.h>



// ~~~~~~~~~~ 2. Macros ~~~~~~~~~~ //

// The maximum number of addresses a memory array can store.
#define MEMORY_LENGTH 64

// The maximum number of strings a string_array can store.
#define STR_ARR_LENGTH 64

// The maximum size a temporary string can be.
#define TEMP_STR_LENGTH 256

// The maximum length an identifier's name can be.
#define IDENTIFIER_NAME_LENGTH 16


// ~~~~~~~~~~ 3. Structure Definitions ~~~~~~~~~~ //

/*
 String Array Structure
 A structure for storing an array of strings.

 This is necessary to keep track of the size of string arrays because we cannot
 check the length of a string array if it is defined as `char** names` or passed
 as a function parameter.

 This struct allows us to contain the array AND the length in a single data type
 without data loss.
*/
typedef struct
{
	char* array[STR_ARR_LENGTH]; // An array of strings
	int length; // The number of elements in the array
	int padding; // This is just to avoid byte padding issues because of the 4 wasted bytes of memory after length.
} string_array;

/*
 Memory Line Structure
 The structure used for storing variables in memory for later retrieval.

 This is necessary for storing a dynamic number of variables in memory.
 This struct is never used in isolation: always in a pre-defined array
 such as `memory_cache[]`.
*/
typedef struct
{
	char* name; // Name of the variable
	double value; // Value of the variable
} memory_line;

/*
 A structure containing information about ml functions.

 @name = The name of the function.
 @lines = An array of strings. Each string is a line from the .ml program within this function.
 @local_memory = An array of memory_line's which serves as local memory for the function.
 @param_count = The number of parameters the function takes.
 @does_return = A non-negative number indicates this function returns a value.
*/
typedef struct
{
	char* name; // The name of the function.
	string_array lines; // This stores the lines which will be processed when the function is called.
	memory_line* local_memory[MEMORY_LENGTH]; // Stores local memory.
	int param_count; // The number of parameters this function takes.
	int does_return; // Whether or not this function returns a value.
} function;



// ~~~~~~~~~~ 4. Global Variable Definitions ~~~~~~~~~~ //


// Global variable containing an array of function structures.
// This is used to store functions defined by .ml programs.
function* functions_cache[MEMORY_LENGTH];

// Global variable containing an array of memory_line structures.
// This is used to store global variables in .ml programs.
memory_line* memory_cache[MEMORY_LENGTH];

// A bool which stores if an error has occurred.
// This is used to halt printing outputs at the end of the program.
bool error_occurred = false;

// Stores an array of lines which will be printed at the end of the
// program. This needs to be stored (rather than printed immediately)
// because we may not want to print them (if an error has occurred).
string_array* lines_to_print;



// ~~~~~~~~~~ 5. Function Definitions ~~~~~~~~~~ //


// ~~~ Processing Functions ~~~ //

double process_lines(string_array file_lines, memory_line* memory[MEMORY_LENGTH]);


// ~~~ Extraction/Calculation Functions ~~~ //

int extract_keywords(char* line, string_array* token_stream);
int extract_lines_from_fd(int fd, string_array* line_stream);
void extract_functions(string_array* file_lines);
void calc_expression(string_array keywords, int expr_start_pos, int expr_end_pos, char* stream, memory_line* memory[MEMORY_LENGTH]);
int parse_function_syntax(string_array* keywords, int start_pos);


// ~~~ Memory Functions ~~~ //

void ml_assign_variable(char* name, double value, memory_line* memory[MEMORY_LENGTH]);
int ml_check_variable(const char* name, memory_line* memory[MEMORY_LENGTH]);
double ml_retrieve_variable(const char* name, memory_line* memory[MEMORY_LENGTH]);
int ml_check_function(const char* name);
function* ml_retrieve_function(const char* name);
void ml_add_function(function* function_info);


// ~~~ Additional Functions ~~~ //

void dtos(double value, char* buffer);
int is_real_num(char* string);
void remove_character(char to_remove, char* stream);
void remove_strings_from_array(string_array* str_array, int start_pos, int end_pos, bool reverse);
double fmod(const double X, const double Y);
int print_error(char* string, bool should_exit);
int print_lines();



// ~~~~~~~~~~ 6. Function Implementations ~~~~~~~~~~ //

/*
 Main

 This is where the input file is opened, each line is extracted into an array, each function
 is extracted into an array, then where each line is processed and executed.
*/
int main(int argc, char* argv[])
{
	// Ensures that there are valid arguments
	if (argc < 2)
	{
		print_error("ERROR: No input parameters provided.\n", true);
	}

	// Retrieve file descriptor of the ML file
	const int ml_fd = open(argv[1], O_RDONLY);
	if (ml_fd == -1) { print_error("ERROR: Could not open input file!\n", true); }

	// This passes all arguments after argv[1] into global memory
	if (argc > 2)
	{
		for (int i = 2; i < argc; i++)
		{
			// Format the name of the argument
			char name[IDENTIFIER_NAME_LENGTH];
			sprintf(name, "arg%i", i - 2);

			// Get the value of the argument
			double value;
			const int scanf_status = sscanf(argv[i], "%lf", &value);
			if (scanf_status != 1)
			{
				print_error("ERROR: Input argument is not a real number!\n", true);
			}

			// Assign variable
			ml_assign_variable(name, value, memory_cache);
		}
	}

	// Creates a string_array which will contain each line from the file.
	string_array file_lines;
	file_lines.length = 0;

	// Allocate to lines_to_print
	lines_to_print = malloc(sizeof(string_array));
	lines_to_print->length = 0;

	// Extract all lines from the file into file_lines
	extract_lines_from_fd(ml_fd, &file_lines);

	// Now that lines have been extracted, we can safely close the file.
	close(ml_fd);

	// This loops over each line and looks for functions.
	// If it finds a function, the function and all its contents are removed from file_lines and cached into functions_cache.
	extract_functions(&file_lines);

	// Execute the functionality from any remaining lines.
	process_lines(file_lines, memory_cache);

	// Print outputs to screen
	print_lines();

	exit(EXIT_SUCCESS);
}

/*
 Extract Keywords
 
 This function will extract the keywords from a line.
 Takes two inputs: a line (char*), and a token_stream (char*[]).

 @line = For the input text from which keywords are extracted.
 @token_stream = A char ptr array and is where the keywords output is written to.
*/
int extract_keywords(char* line, string_array* token_stream)
{
	// Make a new line for temporary storage as "strtok" will modify the input stream.
	char temp_line[TEMP_STR_LENGTH];
	strcpy(temp_line, line);

	if (strlen(temp_line) == 0)
	{
		return -1;
	}

	// Get the first token using "strtok" and use a whitespace as a delimiter.
	// "strtok" will split an input string into tokens. Similar to the py function "split".
	char* token = strtok(temp_line, " ");

	// Initialise a counter in order to track placement into the token_stream.
	int token_count = 0;

	// Loop over the tokens extracted and place them into the token_stream.
	while (token != NULL)
	{
		// Allocate the right amount of memory to the array element
		// Also, its +1 to account for the null-byte.
		token_stream->array[token_count] = malloc(strlen(token) + 1);

		// Handle memory allocation failure
		if (token_stream->array[token_count] == NULL) {
			print_error("ERROR: Keyword Extraction Failed! Memory allocation error.\n", true);
		}

		// Copies the token string into the array.
		strcpy(token_stream->array[token_count], token);

		// Get next token and increment.
		token = strtok(NULL, " ");
		token_count++;
	}

	// Ensure the struct has the right array length.
	token_stream->length = token_count;

	return 0;
}

/*
 Extract Lines from File-Descriptor

 This function will read from a file in blocks of 256 bytes.
 In each block, it will loop over each character until it finds the end of a line.
 Once the end of the line has been found, the block is added to line_stream.

 @fd = The file descriptor lines are being extracted from.
 @line_stream = A string_array into which the lines are cached to.
 */
int extract_lines_from_fd(int fd, string_array* line_stream)
{
	/// Creates a temporary buffer for the file line to be stored into.
	char buffer[TEMP_STR_LENGTH];

	// Cache each line from the file into file_lines.
	int line_count = 0;
	char line[TEMP_STR_LENGTH];
	int line_len = 0;

	ssize_t bytes_read;

	// Read bytes from the file
	// While the number of bytes read is > 0, add those bytes to line_stream
	while ((bytes_read = read(fd, buffer, sizeof(buffer) - 1)) > 0)
	{
		int buf_pos = 0;

		// Loop over each character in the buffer
		while (buf_pos < bytes_read)
		{
			const char c = buffer[buf_pos++];

			// Upon finding the end of a line, add that line to the line_stream
			if (c == '\n' || c == '\r')
			{
				// Mark this as the end of the line buffer
				line[line_len] = '\0';

				if (strlen(line) == 0)
				{
					continue;
				}

				// Process the line
				line_stream->array[line_count] = malloc(line_len + 1);
				strcpy(line_stream->array[line_count], line);
				line_stream->length++;
				line_count++;
				line_len = 0;
			}
			else
			{
				// If this isn't the end of the line, add the current character the line buffer
				if (line_len < sizeof(line) - 1)
				{
					line[line_len++] = c;
				}
			}
		}
	}
	// Process the last line if it doesn't end with a newline
	if (line_len > 0)
	{
		line[line_len] = '\0';
		line_stream->array[line_count] = malloc(line_len + 1);
		strcpy(line_stream->array[line_count], line);
		line_stream->length++;
		line_count++;
	}

	return 0;
}

/*
 Extract Functions
 
 This removes functions from file_lines and puts them into the function_cache. 
 This needs to run first, before anything is processed, otherwise functions will
 be prematurely executed.

 This function is a separate pass for the sake of readability.

 @file_lines = The file lines from which lines with functions in them are extracted.
*/
void extract_functions(string_array* file_lines)
{
	// This is used to track global variable declarations.
	// We need to track them because functions cannot have parameters
	// with the same name as global variables.
	memory_line* temp_variables[MEMORY_LENGTH];

	// Copies current global memory into this temp memory
	// The only variables that can be passed through at this point are arguments.
	for (int memory_index = 0; memory_index < MEMORY_LENGTH; memory_index++)
	{
		// If this slot in the memory_cache is valid, copy the name into the temp memory
		if (memory_cache[memory_index] != NULL)
		{
			temp_variables[memory_index] = malloc(sizeof(memory_line));
			temp_variables[memory_index]->name = malloc(TEMP_STR_LENGTH);
			strcpy(temp_variables[memory_index]->name, memory_cache[memory_index]->name);
			continue;
		} 

		// If this slot in the memory_cache is invalid, set this as an empty slot in temp memory
		temp_variables[memory_index] = malloc(sizeof(memory_line));
		temp_variables[memory_index]->name = malloc(TEMP_STR_LENGTH);
		temp_variables[memory_index]->name = "";
	}

	// Iterates over every line
	for (int i = 0; i < file_lines->length; i++)
	{
		string_array keywords;

		int status = extract_keywords(file_lines->array[i], &keywords);
		if (status != 0)
		{
			// This means its an empty line
			continue;
		}

		// Iterates over every keyword in a line
		for (int j = 0; j < keywords.length; j++)
		{
			// IGNORE COMMENTS
			if(keywords.array[j][0] == '#')
			{
				break;
			}

			// VARIABLES
			// Track variable assignments, do not store them globally yet
			if (strcmp(keywords.array[j], "<-") == 0)
			{
				// incorrect syntax
				if (keywords.length - 1 < 0 || keywords.length + 1 > 63 || j + 1 == keywords.length || j == 0)
				{
					// we don't print an error for this just yet
					break;
				}
				
				// Check if the variable name has already been assigned to a variable or function.
				if (ml_check_variable(keywords.array[j - 1], temp_variables) != -1) { break; }
				if (ml_check_function(keywords.array[j - 1]) != -1) { print_error("ERROR: Variables cannot have the same name as a defined function!\n", false); break; }

				// Assign the result of the expression to a variable
				ml_assign_variable(keywords.array[j - 1], 0, temp_variables);
				break;
			}

			// FUNCTIONS
			if (strcmp(keywords.array[j], "function") == 0)
			{
				// If the function has invalid syntax, throw an error for what is wrong
				if (j != 0 || j + 1 == keywords.length || i + 1 == file_lines->length)
				{
					print_error("ERROR: Invalid function assignment syntax!\n", false);
					if (j != 0) { print_error("Function declarations must begin at keyword index 0.\n", false); }
					if (j + 1 == keywords.length) { print_error("Function missing a name!\n", false); }
					if (i + 1 == file_lines->length) { print_error("Function declared without contents!\n", false); }
				}

				// Check if the function has already been defined as a function or variable
				if(ml_check_function(keywords.array[j + 1]) != -1) {print_error("ERROR: Defining function with a duplicate name!\n", false); break;	}
				if (ml_check_variable(keywords.array[j + 1], temp_variables) != -1) {print_error("ERROR: Functions cannot have the same name as a defined variable!\n", false); break; }

				// allocation memory for function
				function* ml_function = malloc(sizeof(function));
				ml_function->lines.length = 0;


				for (int p = 0; p < MEMORY_LENGTH; p++)
				{
					ml_function->local_memory[p] = malloc(sizeof(memory_line));
					ml_function->local_memory[p]->name = malloc(TEMP_STR_LENGTH);
					ml_function->local_memory[p]->name = "";
				}

				// Give the function the correct name
				const char* function_name = keywords.array[j + 1];

				ml_function->name = malloc(strlen(function_name) + 1);
				strcpy(ml_function->name, function_name);

				// Give the function its local memory
				ml_function->param_count = 0;
				for (int k = 2; k < keywords.length; k++)
				{
					// If param name has already been defined in global variables, error
					if (ml_check_variable(keywords.array[k], temp_variables) != -1)
					{
						// This SHOULD exit immediately, because if this is not handled it will cause a seg fault.
						print_error("ERROR: Function parameters cannot have the same name as defined variables.\n", true);
						goto early_continue;
					}

					if (ml_check_variable(keywords.array[k], ml_function->local_memory) != -1)
					{
						// This SHOULD exit immediately, because if this is not handled it will cause a seg fault.
						print_error("ERROR: Function parameters cannot have duplicate parameters.\n", false);
						goto early_continue;
					}
					ml_assign_variable(keywords.array[k], 0.0, ml_function->local_memory);
					ml_function->param_count = ml_function->param_count + 1;
				}

				int lines_to_remove_count = 0;

				// A while loop that adds all of the function's lines to the function struct
				while (lines_to_remove_count == 0 || file_lines->array[i + lines_to_remove_count][0] == '	')
				{

					char* current_line = file_lines->array[i + lines_to_remove_count];

					if (lines_to_remove_count == 0)
					{
						lines_to_remove_count++;
						continue;
					}

					// Get the next line in the file
					//char* next_line = file_lines->array[i + 1 + lines_to_remove_count];
					remove_character('	', current_line);

					// Ensure the current line in the function has enough space to store the next.
					ml_function->lines.array[ml_function->lines.length] = malloc(sizeof(current_line) + 1);

					// Copy the next line into the function
					strcpy(ml_function->lines.array[ml_function->lines.length], current_line);

					// Increment the length and the number of lines we will remove
					ml_function->lines.length++;
					lines_to_remove_count++;
				}

				// Before we check if the function actually returns, initialise does_return as -1.
				ml_function->does_return = -1;

				// Iterate over each line and look for a return statement.
				// If there is one (with valid syntax), then we mark the function as return.
				for (int line_index = ml_function->lines.length - 1; line_index >= 0; line_index--)
				{
					string_array function_line_keywords;
					extract_keywords(ml_function->lines.array[line_index], &function_line_keywords);

					// loop over each keyword
					for (int keyword_index = 0; keyword_index < function_line_keywords.length; keyword_index++)
					{
						// Check if there is a return statement AND if it returns a value
						if (strcmp(function_line_keywords.array[keyword_index], "return") == 0 && keyword_index + 1 != function_line_keywords.length)
						{
							// A valid return statement is found, so set does-return as a non-negative int.
							ml_function->does_return = 1;

							// Ensure both inner/outer loops are broken
							goto early_exit;
						}
					}
					// Need this continue so that "early_exit" isn't run every loop.
					continue;

				early_exit:
					break;
				}
				// Remove the function's lines from file_lines
				remove_strings_from_array(file_lines, i, i + lines_to_remove_count, false);

				// Adds the function to function_cache
				ml_add_function(ml_function);

				// Need to DECREMENT `i` otherwise when we remove lines, we will accidentally skip a line.
				i--;
			}
		}
		early_continue:
			continue;
	}
	return;
}

/*
 Process Lines

 This is where the actual logic happens and what runs the ml code.
 This takes an input string_array and decides what to do with it.
 If it can't figure out what to do, it prints an error.

 @keywords = A string_array which lists every keyword in the line as well as the number of keywords.
 @memory = The memory
*/
double process_lines(string_array file_lines, memory_line* memory[MEMORY_LENGTH])
{
	//Loop over each line
	for (int line_index = 0; line_index < file_lines.length; line_index++)
	{
		string_array keywords;
		int status = extract_keywords(file_lines.array[line_index], &keywords);
		if (status != 0)
		{
			// This means its an empty line
			continue;
		}

		// Loop over each keyword in a line
		for (int i = 0; i < keywords.length; i++)
		{
			// COMMENTS
			if (keywords.array[i][0] == '#')
			{
				goto early_continue;
			}

			// VARIABLE ASSIGNMENTS
			if (strcmp(keywords.array[i], "<-") == 0)
			{
				// Error messages for incorrect syntax
				if (keywords.length - 1 < 0 || keywords.length + 1 > 63 || i + 1 == keywords.length || i == 0)
				{
					print_error("ERROR: Variable assignment failed! Incorrect syntax.\n", false);
					if (keywords.length - 1 < 0 || i == 0) { print_error("Assigning value to nothing!\n", false); }
					if (keywords.length + 1 > 63) { print_error("Assignment expression is too long!\n", false); }
					if (i + 1 == keywords.length) { print_error("Assigning nothing to variable!\n", false); }
				}


				// check for any expressions and handle them
				char c_result[TEMP_STR_LENGTH];
				calc_expression(keywords, i + 1, keywords.length, c_result, memory);

				double d_result;
				(void)sscanf(c_result, "%lf", &d_result);

				// Check if the variable has already been assigned.
				const int mem_index = ml_check_variable(keywords.array[i - 1], memory);
				if (mem_index != -1)
				{
					// If the variable exists, update the value.
					memory[mem_index]->value = d_result;
					goto early_continue;
				}

				// Assign the result of the expression to a variable
				ml_assign_variable(keywords.array[i - 1], d_result, memory);
				goto early_continue;
			}

			// PRINTING
			if (strcmp(keywords.array[i], "print") == 0)
			{
				if (i != 0)
				{
					print_error("ERROR: Print statement must be located at index 0 on a line.\n", false);
					goto early_continue;
				}

				if (keywords.length + 1 > 63 || i == keywords.length)
				{
					print_error("ERROR: Invalid print parameters!\n", false);
					goto early_continue;
				}

				// Calculate the expression
				char expression_result[TEMP_STR_LENGTH];
				calc_expression(keywords, 1, keywords.length, expression_result, memory);

				// Add the expression result to the lines_to_print array.
				// These lines will be printed at the end of the program (after validation).
				// If there are any errors, these won't be printed.
				lines_to_print->array[lines_to_print->length] = malloc(sizeof(expression_result));
				strcpy(lines_to_print->array[lines_to_print->length], expression_result);
				lines_to_print->length++;

				// Continue the outer loop
				goto early_continue;
			}

			// RETURN
			if (strcmp(keywords.array[i], "return") == 0)
			{
				if (i != 0)
				{
					print_error("ERROR: Return statements must be located at index 0 on a line!\n", false);
				}

				// If returning nothing, then just end it here
				if (i + 1 == keywords.length) { return 0.0; }

				// Check for any expressions and handle them
				char c_result[TEMP_STR_LENGTH];
				calc_expression(keywords, i + 1, keywords.length, c_result, memory);
				double d_result;
				(void)sscanf(c_result, "%lf", &d_result);

				return d_result;
			}

			// FUNCTIONS
			if (parse_function_syntax(&keywords, i) != -1)
			{
				// If a function was actually handled, then continue.
				goto early_continue;

				// This works because parse_function_syntax() will both check if it is a valid function AND
				// execute it in the same function.
			}
		}
		print_error("ERROR: Invalid statements!\n", false);
		fprintf(stderr, "! Error occurred at: `%s`\n", file_lines.array[line_index]);
		

		// Allows for each statement to continue the outer loop
	early_continue:
		continue;
	}
	return 1;
}

/*
 ML Assign Variable

 When the processor finds an assignment keyword, it will assign a value to a name.

 The value (double) and name (string) are stored inside a struct called "memory_line",
 and that is cached in an array of memory_lines.

 @name = The name of the variable being assigned.
 @value = The value of the variable being assigned.
 @memory = The memory cache (either global or local) that the variable is being assigned to.
*/
void ml_assign_variable(char* name, const double value, memory_line* memory[MEMORY_LENGTH])
{
	// Check if the name is valid
	if (strlen(name) == 0 || !isalpha(name[0]))
	{
		print_error("ERROR: Tried to assign variable with an invalid name.\n", false);
	}

	// Check if there are any empty slots in memory.
	// Store the first empty index.
	const int slot_index = ml_check_variable("", memory);
	if (slot_index == -1)
	{
		print_error("ERROR: Tried to assign variable. Out of memory!\n", true);
	}

	// Store the variable information in the empty slot.
	memory[slot_index]->name = malloc(sizeof(name) + 1);
	strcpy(memory[slot_index]->name, name);

	memory[slot_index]->value = value;
}

/*
 ML Check Variable
 
 Checks if a variable of a specified name exists in the input memory cache.
 If the variable exists, this function returns its slot_index in memory. -1 if not.

 @name = The name of the variable being searched for in the input memory cache.
 @memory = The memory cache being searched.
*/
int ml_check_variable(const char* name, memory_line* memory[MEMORY_LENGTH])
{
	// Loop over every variable in the input memory cache
	for (int i = 0; i < MEMORY_LENGTH; i++)
	{
		// If the current slot is empty, allocate memory to it
		if (memory[i] == NULL)
		{
			memory[i] = malloc(sizeof(memory_line)+1);
			if (memory[i] == NULL)
			{
				print_error("ERROR: Memory allocation failed!\n", true);
			}
			memory[i]->name = "";
		}

		// If the current slot's name matches the search name, return its slot index
		if (strcmp(memory[i]->name, name) == 0)
		{
			return i;
		}
	}

	// Return -1 to show the variable wasn't found
	return -1;
}

/*
 Ml Check Function

 Checks if a function of a specified name exists in the functions_cache.
 If the function exists, this function returns its slot_index in memory. -1 if not.

 @name = The name of the function being searched for.
*/
int ml_check_function(const char* name)
{
	// Loop over every function in functions_cache
	for (int i = 0; i < MEMORY_LENGTH; i++)
	{
		// If the current slot is empty, allocate memory to it
		if (functions_cache[i] == NULL) {
			functions_cache[i] = malloc(sizeof(function));
			if (functions_cache[i] == NULL) {
				print_error("ERROR: Function memory allocation failed!\n", true);
			}
			functions_cache[i]->name = "";
		}

		// If the current slot's name matches the search name, return its slot index
		if (strcmp(functions_cache[i]->name, name) == 0)
		{
			return i;
		}
	}

	// Return -1 to show the function wasn't found
	return -1;

}

/*
 ML Add Function
 
 This function will add a new ML function into the functions cache memory array.

 @function_info = The new function which is being added to the cache.
*/
void ml_add_function(function* function_info)
{
	// Find the first empty slot in memory
	const int slot_index = ml_check_function("");

	// If there is no empty slots, exit failure
	if (slot_index == -1)
	{
		print_error("ERROR: Invalid function assignment. Out of memory!\n", true);
	}

	// Assign all info to the slot in memory
	functions_cache[slot_index]->lines = function_info->lines;
	functions_cache[slot_index]->name = malloc(sizeof(function_info->name) + 1);
	strcpy(functions_cache[slot_index]->name, function_info->name);
	functions_cache[slot_index]->param_count = function_info->param_count;

	// Assign all global memory to local memory
	for (int j = 0; j < MEMORY_LENGTH; j++)
	{
		if (function_info->local_memory[j] == NULL) { break; }
		functions_cache[slot_index]->local_memory[j] = function_info->local_memory[j];
	}
}

/*
 ML Retrieve Function

 This function will retrieve a defined function from the functions cache.
 If the function doesn't exist, it will throw an error. To avoid an error, always
 use "check_function()" first.

 @name = The name of the function being retrieved.
*/
function* ml_retrieve_function(const char* name)
{
	// Find the slot index of the function parameter
	const int slot_index = ml_check_function(name);

	// If it could not find the index, throw an error.
	if (slot_index == -1)
	{
		print_error("ERROR: Tried to retrieve invalid function.\n", true);
	}

	return functions_cache[slot_index];
}

/*
 ML Retrieve Variable

 This function will retrieve a defined variable from the input memory cache.
 If the variable doesn't exist, it will throw an error. To avoid an error, always
 use "check_variable()" first.

 @name = The name of the variable being retrieved.
 @memory = The memory cache from which the variable is being retrieved.
*/
double ml_retrieve_variable(const char* name, memory_line* memory[MEMORY_LENGTH])
{
	// Find the slot in memory with the input name.
	const int slot_index = ml_check_variable(name, memory);
	if (slot_index == -1)
	{
		print_error("ERROR: Tried to retrieve variable with an invalid name.\n", true);
	}

	return memory[slot_index]->value;
}

/*
 Calculate Expression Function

 This function will recursively calculate the result of an expression.
 Variables will be retrieved, functions returned and math expressions calculated.

 @keywords = This is the array of keywords found in the current line. This includes ALL keywords, not just the expression.
 @expr_start_pos = This is the starting index of the expression in the keywords array.
 @expr_end_pos = This is the ending index of the expression in the keywords array.
 @stream = The character stream where the expression result will be stored.
 @memory = The memory cache from which variables will be retrieved.
*/
void calc_expression(string_array keywords, int expr_start_pos, int expr_end_pos, char* stream, memory_line* memory[MEMORY_LENGTH])
{
	// This iterates over every keyword and finds any variables.
	// This then replaces the variables with their values in the keywords array.
	// For example: "x + y" becomes "2.5 + 3.5".
	for (int i = expr_start_pos; i < expr_end_pos; i++)
	{
		// Replace every variable with its value

		// Check if it is a function
		int starting_length = keywords.length;
		if (parse_function_syntax(&keywords, i) != -1)
		{
			int difference = starting_length - keywords.length;
			expr_end_pos = expr_end_pos - difference;
			continue;
		}

		// Check if it is a number
		if(is_real_num(keywords.array[i]) != -1)
		{
			// Do nothing with it
			continue;
		}

		// Check if it is a variable
		if (ml_check_variable(keywords.array[i], memory) != -1)
		{
			// Replace its var call with its value
			char* retrieved_var_s = "";
			const double retrieved_var_d = ml_retrieve_variable(keywords.array[i], memory);
			retrieved_var_s = malloc(sizeof(retrieved_var_d) + 1);
			dtos(retrieved_var_d, retrieved_var_s);
			keywords.array[i] = malloc(sizeof(retrieved_var_d) + 1);
			strcpy(keywords.array[i], retrieved_var_s);

			continue;
		}

		// Check if it is an operator
		if (strlen(keywords.array[i]) == 1)
		{
			char* char_ptr = strchr("+-*/", keywords.array[i][0]);
			if (char_ptr != NULL)
			{
				continue;
			}
		}

		// If it is none of those, assume it is an undefined variable and set it to 0
		keywords.array[i] = "0";
	}

	// Outer loop to handle two passes (multiplication/division, then addition/subtraction)
	for (int pass_index = 0; pass_index < 2; pass_index++)
	{
		char* operator = pass_index == 0 ? "*/" : "+-"; // Select operator based on pass

		int count = 0;

		// Using a while loop to evaluate the expression
		while (expr_end_pos - expr_start_pos > 1 && count < keywords.length)
		{
			for (int i = expr_start_pos; i < expr_end_pos; i++)
			{
				// 1-character check ensures that we are only processing operators, not variable names or function calls
				if (strlen(keywords.array[i]) == 1 &&
					(keywords.array[i][0] == operator[0] || keywords.array[i][0] == operator[1]))
				{
					// Parsing the numbers before and after the operator
					double param1 = 0, param2 = 0;
					(void)sscanf(keywords.array[i - 1], "%lf", &param1);
					(void)sscanf(keywords.array[i + 1], "%lf", &param2);

					double result = 0;

					// Perform the operation based on the current operator
					switch (keywords.array[i][0])
					{
					case '*':
						result = param1 * param2;
						break;
					case '/':
						if (param2 == 0.0)
						{
							print_error("ERROR: Division by zero!\n", true);
						}
						result = param1 / param2;
						break;
					case '+':
						result = param1 + param2;
						break;
					case '-':
						result = param1 - param2;
						break;
					}

					// Storing the result back into the array at the position of param1
					snprintf(keywords.array[i - 1], TEMP_STR_LENGTH, "%.6f", result);

					// Shifting the remaining elements to remove the operator and param2
					for (int j = i; j < expr_end_pos - 2; j++)
					{
						keywords.array[j] = keywords.array[j + 2];
					}

					expr_end_pos -= 2; // Adjust expression end position
					i--; // Adjusting index to recheck the shifted array
				}
			}
			count++;
		}
	}

	if (expr_end_pos - expr_start_pos > 1)
	{
		print_error("ERROR: Could not resolve expression.\n", false);
	}

	// This will only execute once the expression is 1 string long (always a number by now)
	if (isdigit(keywords.array[expr_start_pos][0]))
	{
		double number = 0;
		(void)sscanf(keywords.array[expr_start_pos], "%lf", &number);

		char* retrieved_var_s = "";
		retrieved_var_s = malloc(sizeof(number) + 1);
		dtos(number, retrieved_var_s);
		strcpy(stream, retrieved_var_s);
		return;
	}

	return;
}

/*
 Float Modulo

 This function acts the same as fmod() from math.h without needing to include math.h.
 This is needed to determine if a value should be displayed as a double or integer.
*/
double fmod(const double X, const double Y)
{
	double quotient = X / Y;
	const int tmp = (int)quotient / 1;
	quotient = tmp;
	const double result = X - (Y * quotient);
	return result;
}

/*
 Double to String

 This is used for display purposes.
 It converts a double into a string and stores it in the input buffer.

 @value = The input double which will be converted.
 @buffer = The location where the output string is stored.
*/
void dtos(double value, char* buffer)
{
	// If the value could be an int, print it as an int (no decimal places)
	if (fmod(value, 1) == 0)
	{
		sprintf(buffer, "%i", (int)value);
		return;
	}

	// Otherwise print it as a double with 6 decimal places
	(void)sprintf(buffer, "%.6f", value);
}

/*
 Is Real Number

 This function takes an input string and checks if it is a real number.

 @string = The string which will be checked.
*/
int is_real_num(char* string)
{
	double number = 0;
	return sscanf(string, "%lf", &number) - 1;
}

/*
 Remove Character

 Removes all instances of a character from a string.
 In-place removal

 @to_remove = The character which will be removed from the stream.
 @stream = The string which is being edited.
*/
void remove_character(char to_remove, char* stream)
{
	int next_char_pos = 0;
	const int length = (int)strlen(stream);

	for (int index = 0; index < length; index++)
	{
		if (stream[index] != to_remove)
		{
			stream[next_char_pos] = stream[index];
			next_char_pos++;
		}
	}
	stream[next_char_pos] = '\0';
}

/*
 Remove Strings from Array
 
 This is used to remove all elements, within an input boundary, from an array of strings.

 @str_array = string_array from which strings are removed.
 @start_pos = The first index at which strings should be removed. This index is also removed.
 @end_pos = The last index at which strings should be removed. This index is NOT removed.
 @reverse = If true, the function will remove all strings OUTSIDE the boundary.
*/
void remove_strings_from_array(string_array* str_array, int start_pos, int end_pos, bool reverse)
{
	int next_string_pos = 0;
	for (int index = 0; index < str_array->length; index++)
	{
		// If not reversed, remove strings WITHIN bounds
		if (reverse == false)
		{
			if (index < start_pos || index >= end_pos)
			{
				str_array->array[next_string_pos] = str_array->array[index];
				next_string_pos++;
			}
		}
		// If reversed, remove strings OUTSIDE bounds
		else
		{
			if (index >= start_pos && index < end_pos)
			{
				str_array->array[next_string_pos] = str_array->array[index];
				next_string_pos++;
			}
		}
	}
	str_array->length = next_string_pos;
}

/*
 Parse Function Syntax

 This will parse the syntax of a function call.
 This will also execute the function call and find any returned values. The returned values are
 not actually returned by this function, but are instead placed into the original string_array
 replacing the function call.

 @keywords = the keywords passed in.
 @start_pos = the position in the keywords where the function call begins.
*/
int parse_function_syntax(string_array* keywords, int start_pos)
{
	string_array function_keywords;
	function_keywords.length = keywords->length;

	// Ensure that function_keywords is the same as keywords
	for (int i = 0; i < keywords->length; i++)
	{
		function_keywords.array[i] = keywords->array[i];
	}

	// If the name of the keyword we are checking doesn't start with a letter, safely assume it isn't a function.
	if (!isalpha(function_keywords.array[start_pos][0])) { return -1; }

	int end_pos = -1;
	bool found_open_bracket = false;

	// Loop over each keyword (to find the end pos of the function call)
	for (int keyword_index = start_pos; keyword_index < keywords->length; keyword_index++)
	{
		// Loop over each character (to find the first close bracket)
		const int keyword_length = (int)strlen(keywords->array[keyword_index]);
		for (int char_index = 0; char_index < keyword_length; char_index++)
		{
			// If it finds an open bracket > 1 keyword away, this means the starting keyword is NOT a function
			if (!found_open_bracket && keywords->array[keyword_index][char_index] == '(')
			{
				if (keyword_index > start_pos + 1)
				{
					return -1;
				}
			}
			// Find the first closing bracket, and set the end position as the next keyword.
			if (keywords->array[keyword_index][char_index] == ')')
			{
				end_pos = keyword_index + 1;
				goto early_break;
			}
		}
		continue;
	early_break:
		break;

	}

	// Checks if there is no closing bracket to the function call.
	if (end_pos == -1)
	{
		// This may not be a function call, so do not throw an error here.
		// Return failure anyway
		return -1;
	}

	// remove all other keywords from function_keywords outside the function call keywords
	remove_strings_from_array(&function_keywords, start_pos, end_pos, true);


	// This is to know how many strings we need to replace with the return statement later.
	int spaces_added = 0;

	// Replace every "(", ",", ")" character with a space. 
	// This will allow us to separate each keyword properly later. 
	for (int keyword_index = 0; keyword_index < function_keywords.length; keyword_index++)
	{
		const int keyword_length = (int)strlen(function_keywords.array[keyword_index]);
		for (int char_index = 0; char_index < keyword_length; char_index++)
		{
			const char character = function_keywords.array[keyword_index][char_index];
			if (character == '(' || character == ')' || character == ',')
			{
				function_keywords.array[keyword_index][char_index] = ' ';
				spaces_added++;
			}
		}
	}

	// Need to concatenate each keyword together into a single string.
	// This allows us to then extract keywords properly.
	char keyword_stream[TEMP_STR_LENGTH] = "";
	for (int i = 0; i < function_keywords.length; i++)
	{
		strcat(keyword_stream, function_keywords.array[i]);
	}
	keyword_stream[strlen(keyword_stream)] = '\0';

	// Now extract the parsed keywords.
	string_array parsed_keywords;
	extract_keywords(keyword_stream, &parsed_keywords);

	// Check if the function is actually real.
	if (ml_check_function(parsed_keywords.array[0]) != -1)
	{
		// retrieves the function
		function* found_function = ml_retrieve_function(parsed_keywords.array[0]);

		if (parsed_keywords.length - 1 != found_function->param_count)
		{
			print_error("ERROR: Invalid function parameters.\n", true);
		}



		// loop over the parameters and add them into local memory
		for (int i = 0; i < parsed_keywords.length - 1; i++) // need -1 to exclude the function name
		{
			// Convert the parameter from a string into a double
			double d_param;
			(void)sscanf(parsed_keywords.array[i + 1], "%lf", &d_param); // Need +1 to skip the function name

			// Go to the same place in local memory and update
			found_function->local_memory[i]->value = d_param;
		}

		// Add global variables into local memory.
		// Maybe this will be tested, maybe not. But whats the point of separating local and global memory if we don't
		// ever use scope?
		for (int i = 0; i < MEMORY_LENGTH; i++)
		{
			// Throw an error if allocated memory is exceeded.
			if (i + found_function->param_count >= MEMORY_LENGTH)
			{
				print_error("ERROR: Out of memory! Could not assign global variables to local memory.\n", true);
			}

			// If we reach an empty memory slot, break.
			if (memory_cache[i] == NULL)
			{
				break;
			}
			if (strcmp(memory_cache[i]->name, "") == 0)
			{
				break;
			}

			// Get global variable at index i
			char* name = malloc(sizeof(memory_cache[i]->name));
			strcpy(name, memory_cache[i]->name);
			double value = memory_cache[i]->value;

			// Allocate space for it in local memory
			found_function->local_memory[i + found_function->param_count]->name = malloc(sizeof(name));
			strcpy(found_function->local_memory[i + found_function->param_count]->name, name);

			// Copy it into local memory at offset position (accounting for params)
			found_function->local_memory[i + found_function->param_count]->value = value;
		}

		// process the keywords and store the returned result.
		const double returned_result = process_lines(found_function->lines, found_function->local_memory);

		// This updates the global memory.
		// If any global variables have been changed, this loop will update them. 
		for (int i = 0; i < MEMORY_LENGTH; i++)
		{
			if(memory_cache[i] == NULL || found_function->local_memory[i] == NULL)
			{
				break;
			}
			if (strcmp(found_function->local_memory[i]->name, memory_cache[i]->name) == 0)
			{
				memory_cache[i]->value = found_function->local_memory[i]->value;
			}
		}

		// check if found_function returns a value.
		if (found_function->does_return != -1)
		{
			// Turn returned_result into a string from a double
			char buffer[TEMP_STR_LENGTH];
			dtos(returned_result, buffer);

			// allocate enough space for the result, and copy it into the space
			keywords->array[start_pos] = NULL;
			keywords->array[start_pos] = malloc(sizeof(buffer));
			strcpy(keywords->array[start_pos], buffer);

			// Calculate how many keywords to replace in the original line
			const int keywords_to_delete = spaces_added - (spaces_added - end_pos);

			// remove the rest of the function call
			remove_strings_from_array(keywords, start_pos + 1, keywords_to_delete, false);
		}

		// Return a non-negative number to indicate it found and handled a function.
		return 0;
	}

	// If it hits this, then it isn't a function and we return a negative number.
	return -1;
}

/*
Print Error

This function allows us to decide whether we want to exit(EXIT_FAILURE) or not
and ensures that every error message begins with "!".

@string = The string that will be printed to screen.
*/
int print_error(char* string, bool should_exit)
{
	(void)fprintf(stderr, "! %s", string);
	if (should_exit)
	{
		exit(EXIT_FAILURE);
	}
	error_occurred = true;
	return 0;
}

/*
 Print Lines

 This will take an array of strings and print them to the screen.
 The strings are stored within lines_to_print, and strings are added to the array
 when an ML print function occurs.

 If an error has occurred at any point in the execution of the ML program, then
 the outputs will not be printed.
*/
int print_lines()
{
	// If an error has occurred, do NOT print yet.
	if(error_occurred)
	{
		exit(EXIT_FAILURE);
	}

	// If there are no lines to print, exit early.
	if(lines_to_print->length <= 0) { return 0; }

	// Loop over all lines and print them to stdout
	for (int i = 0; i < lines_to_print->length; i++)
	{
		(void)fprintf(stdout, "%s\n", lines_to_print->array[i]);
	}
	return 0;
}
