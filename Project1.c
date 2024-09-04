#include "Project1.h"

//  CITS2002	Project 1 2024
//  Student1:   23074422   Liam-Hearder
//  Student2:   23782402   Tanmay-Arggarwal
//  Platform:   Linux

#pragma warning(disable :5045) // NEED to remove this long before final draft. This disables a warning that linux might not have.

int main(int argc, char *argv[])
{
	// these two lines are just to get rid of a warning.
	if (argc < 2) { return 0; }
	if (argv[1] == NULL) { return 0; }

	// Opens the file and provides an error if it can't find it
	FILE *file = fopen("sample05.ml", "r");
	if (file == NULL) {
		perror("Error opening file");
		return 0;
	}

	// Creates a temporary buffer for the file line to be stored into.
	char buffer[256];

	// Creates a string_array which will contain each line from the file.
	string_array file_lines;
	file_lines.length = 0;

	// Cache each line from the file into file_lines.
	int line_count = 0;
	while (fgets(buffer, 256, file))
	{
		// gets rid of the newline character
		buffer[strcspn(buffer, "\n")] = 0;

		// Allocates enough memory for the new line, copies it into file_lines
		file_lines.array[line_count] = malloc(sizeof(buffer)+1);
		strcpy(file_lines.array[line_count], buffer);

		file_lines.length++;

		line_count++;
	}

	// Function pass
	// This loops over each line and looks for functions.
	// If it finds a function, the function and all its contents are removed from file_lines and cached into a struct.
	extract_functions(&file_lines);

	// Process each line
	for (int i = 0; i < file_lines.length; i++)
	{
		string_array keywords;
		extract_keywords(file_lines.array[i], &keywords);

		process_keywords(keywords, memory_cache, i);
	}
	return 1;
}

/*
This function will extract the keywords from a line.
Takes two inputs: a line (char*), and a token_stream (char*[]).

@line = For the input text from which keywords are extracted.
@token_stream = A char ptr array and is where the keywords output is written to.
*/
int extract_keywords(char *line, string_array *token_stream)
{
	// Make a new line for temporary storage as "strtok" will modify the input stream.
	char temp_line[256];
	strcpy(temp_line, line);

	// Get the first token using "strtok" and use a whitespace as a delimiter.
	// "strtok" will split an input string into tokens. Similar to the py function "split".
	const char* token = strtok(temp_line, " ");

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
			fprintf(stderr, "Memory allocation failed!\n");
			return 0;
		}

		// Copies the token string into the array.
		strcpy(token_stream->array[token_count], token);

		// Get next token and increment.
		token = strtok(NULL, " ");
		token_count++;
	}

	// Ensure the struct has the right array length.
	token_stream->length = token_count;
	
	return 1;
}


void extract_functions(string_array* file_lines)
{
	// Iterates over every line
	for (int i = 0; i < file_lines->length; i++)
	{
		string_array keywords;
		extract_keywords(file_lines->array[i], &keywords);

		// Iterates over every keyword in a line
		for (int j = 0; j < keywords.length; j++)
		{
			// COMMENTS
			if (keywords.array[j][0] == '#')
			{
				break;
			}

			// FUNCTIONS
			if (strcmp(keywords.array[j], "function") == 0)
			{
				// If the function has invalid syntax, throw an error
				if (j != 0 || j+1 == keywords.length || i+1 == file_lines->length)
				{
					fprintf(stderr, "Invalid function assignment syntax!\n");
					exit(EXIT_FAILURE);
				}
				
				function* ml_function = malloc(sizeof(function));
				if (ml_function == NULL) 
				{
					fprintf(stderr, "ml_function: Memory allocation failed!\n");
					exit(EXIT_FAILURE);
				}
				ml_function->lines.length = 0;
				for (int p = 0; p < 64; p++)
				{
					ml_function->local_memory[p] = malloc(sizeof(memory_line)+1);
				}
				

				// Give the function the correct name
				const char* function_name = keywords.array[j + 1];
				ml_function->name = malloc(strlen(function_name)+1);
				if (ml_function->name == NULL) 
				{
					fprintf(stderr, "ml_function->name: Memory allocation failed!\n");
					exit(EXIT_FAILURE);
				}

				strcpy(ml_function->name, function_name);

				// Give the function its local memory
				for (int k = 2; k < keywords.length; k++ )
				{
					ml_assign_variable(keywords.array[k], 0.0, ml_function->local_memory);
				}

				int lines_to_remove_count = 0;
				

				// Iterates over the next few lines after a function has been found
				for (int k = i+1; k < file_lines->length; k++)
				{
					const int index = k - i - 1;
					if (file_lines->array[k][0] == '	')
					{
						const char* next_line = file_lines->array[k];
						ml_function->lines.array[index] = malloc(sizeof(next_line)+1);
						if (ml_function->lines.array[index] == NULL) 
						{
							fprintf(stderr, "ml_function->lines.array[index]: Memory allocation failed!\n");
							exit(EXIT_FAILURE);
						}

						strcpy(ml_function->lines.array[index], next_line);
						ml_function->lines.length++;
						lines_to_remove_count++;
					}
				}

				ml_add_function(ml_function);
				

				// Remove the function's lines from the file_lines to process
				for (int k = 0; k < lines_to_remove_count; k++)
				{
					if (k + lines_to_remove_count >= file_lines->length)
					{
						file_lines->array[k] = NULL;
						continue;
					}
					file_lines->array[k] = malloc(sizeof(file_lines->array[k + lines_to_remove_count])+1);
					if (file_lines->array[k] == NULL) 
					{
						fprintf(stderr, "file_lines->array[k]: Memory allocation failed!\n");
						exit(EXIT_FAILURE);
					}
					strcpy(file_lines->array[k], file_lines->array[k + lines_to_remove_count]);
				}
			}
		}
	}
	return;
}

/*
This is where the actual logic happens and what runs the ml code.
This takes an input string_array and decides what to do with it.
If it can't figure out what to do, it prints an error.

@keywords = A string_array which lists every keyword in the line as well as the number of keywords.
*/
int process_keywords(string_array keywords, memory_line* memory[64], int line_no)
{
	for (int i = 0; i < keywords.length; i++)
	{
		//printf("index: %i - word: %s\n", i, keywords.array[i]);
		// COMMENTS
		if (keywords.array[i][0] == '#')
		{
			return 1;
		}

		// VARIABLE ASSIGNMENTS
		if (strcmp(keywords.array[i], "<-") == 0)
		{
			if (keywords.length - 1 < 0 || keywords.length + 1 > 63 || i == 0)
			{
				printf("VARIABLE ASSIGNMENT ERROR: ACCESS VIOLATION.\n");
				return 0;
			}
			printf("ASSIGNMENT FROM PROCESS: ");
			// NEED TO IMPLEMENT EXPRESSIONS HERE
			ml_assign_variable(keywords.array[i - 1], strtod(keywords.array[i + 1], &keywords.array[keywords.length -1]), memory);
			return 1;
		}

		// PRINTING
		if (strcmp(keywords.array[i], "print") == 0)
		{
			if (keywords.length + 1 > 63 || i == keywords.length)
			{
				printf("PRINT FUNCTION ERROR: INVALID PRINT PARAMS.\n");
				return 0;
			}
			ml_print(keywords, memory);
			return 1;
		}

		// Functions
		if (ml_check_function(keywords.array[i]))
		{
			
			// retrieves the function
			function* found_function = ml_retrieve_function(keywords.array[i]);

			// Assigns function parameters to local memory
			const int param_count = keywords.length - 1;

			// if it needs parameters
			if (param_count > 0)
			{
				// Loop over found parameters and add them to memory
				for (int j = i; j < param_count+i; j++)
				{
					// If the memory is invalid (it doesn't think there are supposed to be params) then break.
					if (strcmp(found_function->local_memory[j]->name, "") == 0) { break; }

					char c_param[8];
					strcpy(c_param, keywords.array[j]);
					remove_character('(', c_param);
					remove_character(')', c_param);
					remove_character(',', c_param);

					double d_param = 0;
					sscanf(c_param, "%lf", &d_param);

					found_function->local_memory[j]->value = d_param;
				}
			}

			// Loop over each line in the function, extract keywords and process them.
			for (int j = 0; j < found_function->lines.length; j++)
			{
				string_array function_keywords;
				extract_keywords(found_function->lines.array[i], &function_keywords);

				process_keywords(function_keywords, found_function->local_memory, j+100);
			}
			return 1;
		}
	}
	printf("SYNTAX ERROR: INVALID STATEMENTS ON LINE %i\n", line_no);
	/*
	for (int i = 0; i < keywords.length; i++)
	{
		printf("%s\n", keywords.array[i]);
	}
	printf("---\n");
	*/

	return 0;
}

// When the processor finds an assignment keyword, then it will assign a value to a name.
// The value (double) and name (string) are stored inside a struct called "memory_line",
// and that is cached in an array of memory_lines. 
void ml_assign_variable(char* name, const double value, memory_line* memory[64])
{
	
	for (int i = 0; i < 64; i++)
	{
		if (memory[i] == NULL) {
			memory[i] = malloc(sizeof(memory_line));
			if (memory[i] == NULL) {
				fprintf(stderr, "memory_cache: Memory allocation failed!\n");
				exit(EXIT_FAILURE);
			}
			memory[i]->name = _strdup("");
		}

		if (strcmp(memory[i]->name, "") == 0)
		{
			strcpy(memory[i]->name, name);
			memory[i]->value = value;
			return;
		}
	}
	printf("VARIABLE ASSIGNMENT INVALID: OUT OF MEMORY\n");
}

// Checks if a variable of a specified name exists in the memory_cache.
// If the variable exists, this function returns true. False if not.
bool ml_check_variable(const char* name, memory_line* memory[64])
{
	for (int i = 0; i < 64; i++)
	{
		if (memory[i] == NULL) {
			memory[i] = malloc(sizeof(memory_line));
			if (memory[i] == NULL) {
				fprintf(stderr, "Memory allocation failed!\n");
				exit(EXIT_FAILURE);
			}
			memory[i]->name = _strdup("");
		}

		if (strcmp(memory[i]->name, name) == 0)
		{
			return true;
		}
	}
	return false;
}

bool ml_check_function(const char* name)
{
	
	for (int i = 0; i < 64; i++)
	{
		if (functions_cache[i] == NULL) {
			functions_cache[i] = malloc(sizeof(memory_line));
			if (functions_cache[i] == NULL) {
				fprintf(stderr, "ml_check_function: Memory allocation failed!\n");
				exit(EXIT_FAILURE);
			}
			functions_cache[i]->name = _strdup("");
		}

		if (strcmp(functions_cache[i]->name, name) == 0)
		{
			return true;
		}
	}
	return false;
	
}

// This function retrieves a ml_function that was previously assigned by ML code.
// When ML code has a keyword that isn't recognized, we assume it's a variable or a function that needs to be retrieved.
// "ml_check_function" MUST be run first.
function* ml_retrieve_function(const char* name)
{
	// Iterate over all memory until it finds the right one
	for (int i = 0; i < 64; i++)
	{
		// Check name of var to see if it matches
		if (strcmp(functions_cache[i]->name, name) == 0)
		{
			// Returns the value as a ptr
			return functions_cache[i];
		}
	}

	// If it reaches this, then the function wasn't found in memory.
	// Meaning it either was misused by the C11 code or the function was never assigned by the ML code.
	// This should NOT be reached in any circumstance, as "ml_check_function" MUST be run first.
	printf("FUNCTION RETRIEVAL INVALID: FUNCTION (%s) DOESN'T EXIST\n", name);
	return NULL;
}

// This function retrieves a variable that was previously assigned by ML code.
// When ML code has a keyword that isn't recognized, we assume it's a variable or a function that needs to be retrieved.
// "ml_check_variable" MUST be run first.
double ml_retrieve_variable(const char* name, memory_line* memory[64])
{
	// Iterate over all memory until it finds the right one
	for (int i = 0; i < 64; i++)
	{
		// Check name of var to see if it matches
		if (strcmp(memory[i]->name, name) == 0)
		{
			// Returns the value as a ptr
			return memory[i]->value;
		}
	}

	// If it reaches this, then the variable wasn't found in memory.
	// Meaning it either was misused by the C11 code or the variable was never assigned by the ML code.
	// This should NOT be reached in any circumstance, as "ml_check_variable" MUST be run first.
	printf("VARIABLE RETRIEVAL INVALID: VARIABLE (%s) DOESN'T EXIST\n", name);
	return 0;
}

void ml_print(string_array keywords, memory_line* memory[64])
{
	// Check if the first keyword is "print". If it isn't, then the syntax is invalid.
	if (strcmp(keywords.array[0], "print") != 0)
	{
		printf("PRINT FUNCTION ERROR : INVALID PRINT PARAMS.\n");
		return;
	}

	// Calculate the expression

	char expression_result[32];
	calc_expression(keywords, 1, keywords.length, expression_result, memory);
	printf("%s\n", expression_result);
	
}

void calc_expression(string_array keywords, int expr_start_pos, int expr_end_pos, char* stream, memory_line* memory[64])
{
	// This iterates over every keyword and finds any variables.
	// This then replaces the variables with their values in the keywords array.
	// For example: "x + y" becomes "2.5 + 3.5".
	for (int i = expr_start_pos; i < expr_end_pos; i++)
	{
		// Replace every variable with its value
		if (ml_check_variable(keywords.array[i], memory))
		{
			char* retrieved_var_s = "";
			const double retrieved_var_d = ml_retrieve_variable(keywords.array[i], memory);
			retrieved_var_s = malloc(sizeof(retrieved_var_d) + 1);
			dtos(retrieved_var_d, retrieved_var_s);
			keywords.array[i] = malloc(sizeof(retrieved_var_d) + 1);
			strcpy(keywords.array[i], retrieved_var_s);
		}
	}

	// This loops over the keywords while there is more than 1 keyword in the expression to calculate.
	// Added "count" to ensure the while loop can't go infinitely if there is a syntax error in the expression.
	int count = 0;
	while (expr_end_pos - expr_start_pos > 1 && count < keywords.length)
	{
		
		for (int i = expr_start_pos; i < expr_end_pos; i++)
		{
			// Make sure its a math symbol
			if(strlen(keywords.array[i]) != 1 || isdigit(keywords.array[i][0]) || isalpha(keywords.array[i][0])) {continue;}

			// Get the math term we are using
			const char term = keywords.array[i][0];

			// if the term isn't one of these, then don't do the following expressions
			if (term != '*' && term != '/' && term != '+' && term != '-') { continue; }

			// REWRITE

			// Exit early if the syntax is invalid
			if (i < 2 || i == keywords.length - 1)
			{
				fprintf(stderr, "Invalid math expression syntax!\n");
				exit(EXIT_FAILURE);
			}

			// Convert from strings to doubles, multiply the numbers together and get the result.
			double param1 = 0;
			double param2 = 0;
			sscanf(keywords.array[i - 1], "%lf", &param1);
			sscanf(keywords.array[i + 1], "%lf", &param2);

			double expr_result = 0;

			switch(term)
			{
			case '*':
				expr_result = param1 * param2;
				break;
			case '/':
				if(param1 == 0.0 || param2 == 0.0)
				{
					fprintf(stderr, "Divide by zero error.\n");
					exit(EXIT_FAILURE);
				}
				expr_result = param1 / param2;
				break;
			case '+':
				expr_result = param1 + param2;
				break;
			case '-':
				expr_result = param1 - param2;
				break;
			default:
				// It shouldn't be able to reach this.
				break;
			}

			// Get the first number in the expression and allocate memory for the new value replacing it
			keywords.array[i - 1] = malloc(sizeof(expr_result) + 1);
			sprintf(keywords.array[i - 1], "%lf", expr_result);

			for (int j = i; j < expr_end_pos; j++)
			{
				if (j + 2 >= keywords.length)
				{
					keywords.array[j] = NULL;
					continue;
				}
				keywords.array[j] = malloc(sizeof(keywords.array[j + 2] + 1));
				strcpy(keywords.array[j], keywords.array[j + 2]);
			}

			// Now that the keywords array is 2 keywords shorter, we need to decrement the expression's end-position by 2.
			expr_end_pos = expr_end_pos - 2;
			keywords.length = keywords.length - 2;

			// This will break the for loop and restart the while loop
			break;
		}
		count++;
	}

	// These functions will only execute once the expression is 1 string long (either a variable or number)

	// This checks if the keyword is a number
	if (isdigit(keywords.array[expr_start_pos][0]))
	{
		double number = 0;
		sscanf(keywords.array[expr_start_pos], "%lf", &number);

		char* retrieved_var_s = "";
		retrieved_var_s = malloc(sizeof(number) + 1);
		dtos(number, retrieved_var_s);
		strcpy(stream, retrieved_var_s);
		return;
	}

	// This will then check if it is a variable
	if(ml_check_variable(keywords.array[expr_start_pos], memory))
	{
		char* retrieved_var_s = "";
		const double retrieved_var_d = ml_retrieve_variable(keywords.array[expr_start_pos], memory);
		retrieved_var_s = malloc(sizeof(retrieved_var_d) + 1);
		dtos(retrieved_var_d, retrieved_var_s);
		strcpy(stream, retrieved_var_s);
		return;
	}
	return;
}

void ml_add_function(function* function_info)
{
	// Ideas

	// Do a function-pass over the file_lines before processing anything else.
	// Look for functions and remove their lines from the file. Add those lines to a "function struct".
	// The "function struct" has two elements, a string_array and memory_cache.
	// The string_array is to store the lines for processing.
	// The memory_cache is for local variables passed through.
	// When the function name is called, find the function in struct array, pass through the local variables,
	// and execute the code line by line. If it reaches a "return" statement, end the processing.

	// returns the index where the processing should continue from in the file_lines array
	for (int i = 0; i < 64; i++)
	{
		
		// Check to make sure none of the elements are NULL, as we cannot check if NULL structs are empty.
		if (functions_cache[i] == NULL) {
			functions_cache[i] = malloc(sizeof(function));
			if (functions_cache[i] == NULL) {
				fprintf(stderr, "functions_cache: Memory allocation failed!\n");
				exit(EXIT_FAILURE);
			}
			functions_cache[i]->name = _strdup("");
		}

		// If an element in the function_cache is empty, add the details into it.
		if (strcmp(functions_cache[i]->name, "") == 0)
		{
			functions_cache[i]->lines = function_info->lines;
			functions_cache[i]->name = malloc(strlen(function_info->name) + 1);
			strcpy(functions_cache[i]->name, function_info->name);

			for (int j = 0; j < 64; j++)
			{
				if (function_info->local_memory[j] == NULL) { break; }
				functions_cache[i]->local_memory[j] = function_info->local_memory[j];
			}
			return;
		}
	}
}

// double to string
void dtos(double value, char* buffer)
{
	// If the value could be an int, print it as an int (no decimal places)
	if(fmod(value, 1) == 0)
	{
		sprintf(buffer, "%i", (int)value);
		return;
	}

	// Otherwise print it as a double with 6 decimal places
	sprintf(buffer, "%f", value);
}

// Removes all instances of a character from a string.
void remove_character(char to_remove, char* stream)
{
	int j, k;
	const int length = strlen(stream);

	for (j = 0, k = 0; j < length; j++)
	{
		if (stream[j] != to_remove)
		{
			stream[k++] = stream[j];
		}
	}
	stream[k] = '\0';
}

// This is just for debugging. An easy-to-call function that prints an array of strings.
void print_strings(string_array* strings)
{
	for (int i = 0; i < strings->length; i++)
	{
		printf("Print Each %i: %s\n", i, strings->array[i]);
	}
}
