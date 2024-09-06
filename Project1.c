#include "Project1.h"

//  CITS2002	Project 1 2024
//  Student1:   23074422   Liam-Hearder
//  Student2:   23782402   Tanmay-Arggarwal
//  Platform:   Linux

#pragma warning(disable :5045) // NEED to remove this long before final draft. This disables a warning that linux might not have.

#define MEMORY_LENGTH 64

int main(int argc, char *argv[])
{
	// these two lines are just to get rid of a warning.
	if (argc < 2) { return 0; }
	if (argv[1] == NULL) { return 0; }

	// Opens the file and provides an error if it can't find it
	FILE *file = fopen("sample08.ml", "r");
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

	process_keywords(file_lines, memory_cache);

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
	char temp_line[512];
	strcpy(temp_line, line);

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

// This removes functions from file_lines and puts them into the function_cache. 
// This needs to run first, before anything is processed, otherwise functions will
// be prematurely executed.
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
				if (j != 0 || j + 1 == keywords.length || i + 1 == file_lines->length)
				{
					fprintf(stderr, "Invalid function assignment syntax: line %i\n", i);
					if (j != 0) { fprintf(stderr, "Function declarations must begin at keyword index 0\n"); }
					if (j + 1 == keywords.length) { fprintf(stderr, "Function missing a name!\n"); }
					if (i + 1 == file_lines->length) { fprintf(stderr, "Function declared without contents!"); }
					exit(EXIT_FAILURE);
				}

				// allocation memory for function
				function* ml_function = malloc(sizeof(function));
				ml_function->lines.length = 0;


				for (int p = 0; p < 64; p++)
				{
					ml_function->local_memory[p] = malloc(sizeof(memory_line));
					ml_function->local_memory[p]->name = malloc(8);
					ml_function->local_memory[p]->name = "";
				}

				// Give the function the correct name
				const char* function_name = keywords.array[j + 1];
				ml_function->name = malloc(strlen(function_name) + 1);
				if (ml_function->name == NULL)
				{
					fprintf(stderr, "ml_function->name: Memory allocation failed!\n");
					exit(EXIT_FAILURE);
				}
				strcpy(ml_function->name, function_name);

				// Give the function its local memory
				ml_function->param_count = 0;
				for (int k = 2; k < keywords.length; k++)
				{
					ml_assign_variable(keywords.array[k], 0.0, ml_function->local_memory);
					ml_function->param_count = ml_function->param_count + 1;
				}

				int lines_to_remove_count = 0;

				// A while loop that adds all of the function's lines to the function struct
				while (lines_to_remove_count == 0 || file_lines->array[i+lines_to_remove_count][0] == '	')
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
				for (int line_index = ml_function->lines.length-1; line_index >= 0; line_index--)
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
				remove_strings_from_array(file_lines, i, i + lines_to_remove_count);

				// Adds the function to function_cache
				ml_add_function(ml_function);

				// Need to DECREMENT `i` otherwise when we remove lines, we will accidentally skip a line.
				i--;
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
double process_keywords(string_array file_lines, memory_line* memory[64])
{
	//Loop over each line
	for (int line_index = 0; line_index < file_lines.length; line_index++)
	{
		string_array keywords;
		extract_keywords(file_lines.array[line_index], &keywords);

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
				if (keywords.length - 1 < 0 || keywords.length + 1 > 63 || i + 1 == keywords.length)
				{
					fprintf(stderr, "VARIABLE ASSIGNMENT ERROR: Incorrect syntax.\n");
					if (keywords.length - 1 < 0) { fprintf(stderr, "Assigning value to nothing!\n"); }
					if (keywords.length + 1 > 63) { fprintf(stderr, "Assignment expression is too long!\n"); }
					if (i + 1 == keywords.length) { fprintf(stderr, "Assigning nothing to variable!\n"); }
					exit(EXIT_FAILURE);
				}

				// check for any expressions and handle them
				char c_result[32];
				calc_expression(keywords, i + 1, keywords.length, c_result, memory);
				double d_result;
				sscanf(c_result, "%lf", &d_result);

				// Assign the result of the expression to a variable
				ml_assign_variable(keywords.array[i - 1], d_result, memory);
				goto early_continue;
			}

			// PRINTING
			if (strcmp(keywords.array[i], "print") == 0)
			{
				if (keywords.length + 1 > 63 || i == keywords.length)
				{
					printf("PRINT FUNCTION ERROR: INVALID PRINT PARAMS.\n");
					goto early_continue;
				}
				ml_print(keywords, memory);
				goto early_continue;
			}

			// RETURN
			if (strcmp(keywords.array[i], "return") == 0)
			{
				// If returning nothing, then just end it here
				if (i + 1 == keywords.length) { return 0.0; }

				// Check for any expressions and handle them
				char c_result[32];
				calc_expression(keywords, i + 1, keywords.length, c_result, memory);
				double d_result;
				sscanf(c_result, "%lf", &d_result);

				return d_result;
			}

			// FUNCTIONS
			if (parse_function_syntax(&keywords, i) != -1)
			{
				// If a function was actually handled, then continue.
				goto early_continue;
			}
		}

		fprintf(stderr, "Invalid statements! Error occurred at: `%s`\n", file_lines.array[line_index]);
		exit(EXIT_FAILURE);

		// Allows for each statement to continue the outer loop
		early_continue:
			continue;
	}
	return 1;
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
			memory[i]->name = malloc(sizeof(name) + 1);
			strcpy(memory[i]->name, name);

			memory[i]->value = value;
			return;
		}
	}
	printf("VARIABLE ASSIGNMENT INVALID: OUT OF MEMORY\n");
	printf("TRIED TO ASSIGN: %s\n", name);
}

// Checks if a variable of a specified name exists in the memory_cache.
// If the variable exists, this function returns true. False if not.
bool ml_check_variable(const char* name, memory_line* memory[64])
{
	for (int i = 0; i < 64; i++)
	{
		if (memory[i] == NULL) 
		{
			memory[i] = malloc(sizeof(memory_line));
			if (memory[i] == NULL) 
			{
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

void calc_expression(string_array keywords, int expr_start_pos, int expr_end_pos, char* stream, memory_line* memory[MEMORY_LENGTH]) // Two plus two is four Minus one that's three, quick maths skrra
    //Replacing variables with their values
    for (int i = expr_start_pos; i < expr_end_pos; i++) 
    {
        if (ml_check_variable(keywords.array[i], memory)) 
        {
            char* retrieved_var_s = malloc(32); // Allocate memory for variable string
            double retrieved_var_d = ml_retrieve_variable(keywords.array[i], memory);
            snprintf(retrieved_var_s, 32, "%.6f", retrieved_var_d); // Convert to string
            keywords.array[i] = realloc(keywords.array[i], strlen(retrieved_var_s) + 1);
            strcpy(keywords.array[i], retrieved_var_s);
            free(retrieved_var_s);
        }
    }

    // Outer loop to handle two passes (multiplication/division, then addition/subtraction)
    for (int pass_index = 0; pass_index < 2; pass_index++) 
    {
        char* operator = pass_index == 0 ? "*/" : "+-"; // Select operator based on pass

        int count = 0;
        // using while loop to evaluate the expression
        while (expr_end_pos - expr_start_pos > 1 && count < keywords.length) 
        {
            for (int i = expr_start_pos; i < expr_end_pos; i++) 
            {
                if (keywords.array[i][0] == operator[0] || keywords.array[i][0] == operator[1]) 
                {
                    // Parse the numbers before and after the operator
                    double param1 = 0, param2 = 0;
                    sscanf(keywords.array[i - 1], "%lf", &param1);
                    sscanf(keywords.array[i + 1], "%lf", &param2);

                    double result = 0;

                    // Performing the operation based on the current operator
                    switch (keywords.array[i][0]) 
                    {
                        case '*':
                            result = param1 * param2;
                            break;
                        case '/':
                            if (param2 == 0.0) 
                            {
                                fprintf(stderr, "Error: Division by zero.\n");
                                exit(EXIT_FAILURE);
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
                    snprintf(keywords.array[i - 1], 32, "%.6f", result);

                    // Shifting the remaining elements to remove the operator and param2
                    for (int j = i; j < expr_end_pos - 2; j++) 
                    {
                        keywords.array[j] = keywords.array[j + 2];
                    }

                    expr_end_pos -= 2; // Adjusting expression end position
                    i--; // Adjusting index to recheck the shifted array
                }
            }
            count++;
        }
    }
    //Copying the final result to the output stream
    strcpy(stream, keywords.array[expr_start_pos]);
}


void ml_add_function(function* function_info)
{
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
			functions_cache[i]->param_count = function_info->param_count;

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
	sprintf(buffer, "%.6f", value);
}

// Removes all instances of a character from a string.
// In-place removal
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

// This is just for debugging. An easy-to-call function that prints an array of strings.
void print_strings(string_array* strings)
{
	for (int i = 0; i < strings->length; i++)
	{
		printf("Print Each %i: %s\n", i, strings->array[i]);
	}
}

// This is used to remove certain elements from an array of strings.
// Another in-place removal
void remove_strings_from_array(string_array* str_array, int start_pos, int end_pos)
{
	int next_string_pos = 0;
	for (int index = 0; index < str_array->length; index++)
	{
		if (index < start_pos || index >= end_pos)
		{
			printf("KEEPING:  %s\n", str_array->array[index]);
			str_array->array[next_string_pos] = str_array->array[index];
			next_string_pos++;
		}
		else
		{
			printf("REMOVING: %s\n", str_array->array[index]);
		}
	}
	str_array->length = next_string_pos;
	printf("\n");
}

// This will parse the syntax of a function call
// @keywords = the keywords passed in.
// @start_pos = the position in the keywords where the function call begins.
int parse_function_syntax(string_array* keywords, int start_pos)
{
	/*
	//	Create a new string_array "function_keywords" to store the function call keywords.
	//	start_pos is where the function name is.
	//	Need to iterate over each character after start_pos and find the first ')' and remove all other keywords from function_keywords.
	//	Then need to add replace all '(', ',' and ')' chars with a space.
	//	Keep track of how many spaces have been added. Subtract the added spaces from the keyword count and that's how many keywords we replace later.
	//	Then extract keywords from the resultant string.
		Process the function, if it returns anything (using return as an attribute) then we get the return from process_keywords
		Replace a number of strings (calculated earlier) in the original keywords with the returned result.
	*/

	string_array function_keywords;
	function_keywords.length = keywords->length;

	// Ensure that function_keywords is the same as keywords
	for (int i = 0; i < keywords->length; i++)
	{
		function_keywords.array[i] = keywords->array[i];
	}

	int end_pos = -1;

	// Loop over each keyword (to find the end pos of the function call)
	for (int keyword_index = start_pos; keyword_index < keywords->length; keyword_index++)
	{
		// Loop over each character (to find the first close bracket)
		const int keyword_length = (int)strlen(keywords->array[keyword_index]);
		for (int char_index = 0; char_index < keyword_length; char_index++)
		{
			// Find the first closing bracket, and set the end position as the next keyword.
			if (keywords->array[keyword_index][char_index] == ')')
			{
				end_pos = keyword_index + 1 - start_pos;
				goto early_break;
			}
		}
		continue;
		early_break:
		printf("Startpos = %i\n", start_pos);
		printf("Endpos: %i\n", end_pos);
			break;

	}

	// Checks if there is no closing bracket to the function call.
	if (end_pos == -1)
	{
		// THERE IS NO CLOSING BRACKET
		// I am unsure if this should be an error, because it might not be a function call...
		// anyway return failure here
		return -1;
	}

	// remove all other keywords from function_keywords outside the function call keywords
	if(start_pos != 0)
	{
		remove_strings_from_array(&function_keywords, 0, start_pos);
	}
	remove_strings_from_array(&function_keywords, end_pos, function_keywords.length);
	print_strings(&function_keywords);

	// This is to know how many strings we need to replace with the return statement later.
	int spaces_added = 0;

	// Replace every "(", ",", ")" character with a space. 
	// This will allow us to separate each keyword properly later. 
	for (int keyword_index = 0; keyword_index < function_keywords.length; keyword_index++)
	{
		int keyword_length = strlen(function_keywords.array[keyword_index]);
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
	char keyword_stream[64] = "";
	for(int i = 0; i < function_keywords.length; i++)
	{
		strcat(keyword_stream, function_keywords.array[i]);
	}
	keyword_stream[strlen(keyword_stream)] = '\0';

	// Now extract the parsed keywords.
	string_array parsed_keywords;
	extract_keywords(keyword_stream, &parsed_keywords);

	// Check if the function is actually real.
	if (ml_check_function(parsed_keywords.array[0]))
	{
		// retrieves the function
		function* found_function = ml_retrieve_function(parsed_keywords.array[0]);

		if (parsed_keywords.length-1 != found_function->param_count)
		{
			/*printf("\n");
			print_strings(&parsed_keywords);
			printf("\n");*/
			
			fprintf(stderr, "ERROR: Invalid function parameters for function `%s`\n", found_function->name);
			exit(EXIT_FAILURE);
		}

		// loop over the parameters and add them into local memory
		for (int i = 0; i < parsed_keywords.length-1; i++) // need -1 to exclude the function name
		{

			// Convert the parameter from a string into a double
			double d_param;
			sscanf(parsed_keywords.array[i+1], "%lf", &d_param); // Need +1 to skip the function name

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
				printf("ERROR: Out of memory! Could not assign global variables to local memory.");
				exit(EXIT_FAILURE);
			}

			// If we reach an empty memory slot, break.
			if(memory_cache[i] == NULL)
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
		const double returned_result = process_keywords(found_function->lines, found_function->local_memory);

		// check if found_function returns a value.
		if (found_function->does_return != -1)
		{
			// This calculates how many keywords in the original line we need to replace with the returned result
			// We need to do a -1 at the end to account for the string we wish to keep (the result).
			int replace_count = parsed_keywords.length - spaces_added - 1;

			// Turn returned_result into a string from a double
			char buffer[16];
			dtos(returned_result, buffer);

			// allocate enough space for the result, and copy it into the space
			keywords->array[start_pos] = NULL;
			keywords->array[start_pos] = malloc(sizeof(buffer));
			strcpy(keywords->array[start_pos], buffer);

			// remove the rest of the function call
			remove_strings_from_array(keywords, start_pos + 1, replace_count+start_pos);
		}

		// Return a non-negative number to indicate it found and handled a function.
		return 1;
	}

	// If it hits this, then it isn't a function and we return a negative number.
	return -1;
}
