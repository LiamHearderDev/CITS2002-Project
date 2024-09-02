#include "Project1.h"

#pragma warning(disable :5045) // NEED to remove this long before final draft. This disables a warning that linux might not have.

int main(int argc, char *argv[])
{
	// these two lines are just to get rid of a warning.
	if (argc < 2) { return 0; }
	if (argv[1] == NULL) { return 0; }

	// Opens the file and provides an error if it can't find it
	FILE *sample = fopen("sample04.ml", "r");
	if (sample == NULL) {
		perror("Error opening file");
		return 0;
	}

	// Creates a buffer for reading each line in the file
	char buffer[512];

	// Iterates through each line in the file
	while (fgets(buffer, 256, sample))
	{
		// gets rid of the newline character
		buffer[strcspn(buffer, "\n")] = 0;

		string_array keywords;
		extract_keywords(buffer, &keywords);

		// runs the ML code.
		process_keywords(keywords);
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

/*
This is where the actual logic happens and what runs the ml code.
This takes an input string_array and decides what to do with it.
If it can't figure out what to do, it prints an error.

@keywords = A string_array which lists every keyword in the line as well as the number of keywords.
*/
int process_keywords(string_array keywords)
{
	const int keyword_count = keywords.length;
	for (int i = 0; i < keywords.length; i++)
	{
		// COMMENTS
		if (keywords.array[i][0] == '#')
		{
			return 1;
		}

		// VARIABLE ASSIGNMENTS
		if (strcmp(keywords.array[i], "<-") == 0)
		{
			if (keyword_count - 1 < 0 || keyword_count + 1 > 63 || i == 0)
			{
				printf("VARIABLE ASSIGNMENT ERROR: ACCESS VIOLATION.\n");
				return 0;
			}
			
			ml_assign_variable(keywords.array[i - 1], strtod(keywords.array[i + 1], &keywords.array[keyword_count-1]));
			return 1;
		}

		// PRINTING
		if(strcmp(keywords.array[i], "print") == 0)
		{
			if (keyword_count + 1 > 63 || i == keywords.length)
			{
				printf("PRINT FUNCTION ERROR: INVALID PRINT PARAMS.\n");
				return 0;
			}
			ml_print(keywords);
			return 1;
		}
	}
	printf("SYNTAX ERROR: INVALID STATEMENTS.");
	return 0;
}

// When the processor finds an assignment keyword, then it will assign a value to a name.
// The value (double) and name (string) are stored inside a struct called "memory_line",
// and that is cached in an array of memory_lines. 
void ml_assign_variable(char* name, const double value)
{
	for (int i = 0; i < 64; i++)
	{
		if (memory_cache[i] == NULL) {
			memory_cache[i] = malloc(sizeof(memory_line));
			if (memory_cache[i] == NULL) {
				fprintf(stderr, "Memory allocation failed!\n");
				exit(EXIT_FAILURE);
			}
			memory_cache[i]->name = _strdup("");
		}
		if (strcmp(memory_cache[i]->name, "") == 0)
		{
			memory_cache[i]->name = name;
			memory_cache[i]->value = value;
			return;
		}
	}
	printf("VARIABLE ASSIGNMENT INVALID: OUT OF MEMORY\n");
}

// Checks if a variable of a specified name exists in the memory_cache.
// If the variable exists, this function returns true. False if not.
bool ml_check_variable(const char* name)
{
	for (int i = 0; i < 64; i++)
	{
		if (memory_cache[i] == NULL) {
			memory_cache[i] = malloc(sizeof(memory_line));
			if (memory_cache[i] == NULL) {
				fprintf(stderr, "Memory allocation failed!\n");
				exit(EXIT_FAILURE);
			}
			memory_cache[i]->name = _strdup("");
		}

		if (strcmp(memory_cache[i]->name, name) == 0)
		{
			return true;
		}
	}
	return false;
}

// This function retrieves a variable that was previously assigned by ML code.
// When ML code has a keyword that isn't recognized, we assume it's a variable that needs to be retrieved.
// "ml_check_variable" MUST be run first.
double ml_retrieve_variable(const char* name)
{
	// Iterate over all memory until it finds the right one
	for (int i = 0; i < 64; i++)
	{
		// Check name of var to see if it matches
		if (strcmp(memory_cache[i]->name, name) == 0)
		{
			// Returns the value as a ptr
			return memory_cache[i]->value;
		}
	}

	// If it reaches this, then the variable wasn't found in memory.
	// Meaning it either was misused by the C11 code or the variable was never assigned by the ML code.
	// This should NOT be reached in any circumstance, as "ml_check_variable" MUST be run first.
	printf("VARIABLE RETRIEVAL INVALID: VARIABLE (%s) DOESN'T EXIST\n", name);
	return 0;
}

void ml_print(string_array keywords)
{
	// Check if the first keyword is "print". If it isn't, then the syntax is invalid.
	if (strcmp(keywords.array[0], "print") != 0)
	{
		printf("PRINT FUNCTION ERROR : INVALID PRINT PARAMS.\n");
		return;
	}

	// Array of strings that will be printed.
	// This is to store any strings that need to be printed, such as from expressions if its multiple things
	char* to_print[32];

	// Calculate the expression

	char expression_result[32];
	calc_expression(keywords, 1, keywords.length, expression_result);
	printf("%s\n", expression_result);
	
}

// This is just for debugging. An easy-to-call function that prints an array of strings.
void print_strings(string_array *strings)
{
	for (int i = 0; i < strings->length; i++)
	{
		printf("Print Each %i: %s\n", i, strings->array[i]);
	}
}

void calc_expression(string_array keywords, int expr_start_pos, int expr_end_pos, char* stream)
{
	// Plan for math expressions:
	//		Probably need to do it in line with brackets -> mult/div -> add/sub
	//		First, scan for multiplication or division. Replace them in keywords with result.
	//		Second, do the same for addition or subtraction.


	// This iterates over every keyword and finds any variables.
	// This then replaces the variables with their values in the keywords array.
	// For example: "x + y" becomes "2.5 + 3.5".
	for (int i = expr_start_pos; i < expr_end_pos; i++)
	{
		// Replace every variable with its value
		if (ml_check_variable(keywords.array[i]))
		{
			char* retrieved_var_s = "";
			const double retrieved_var_d = ml_retrieve_variable(keywords.array[i]);
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
			if(strlen(keywords.array[i]) == 1 && !isdigit(keywords.array[i][0]) && !isalpha(keywords.array[i][0]))
			{
				switch (keywords.array[i][0])
				{

				// MULTIPLICATION
				case '*':

					// Exit early if the syntax is invalid
					if(i < 2 || i == keywords.length-1)
					{
						fprintf(stderr, "Invalid multiplication syntax!\n");
						exit(EXIT_FAILURE);
					}

					// Convert from strings to doubles, multiply the numbers together and get the result.
					double mult_param1 = 0;
					double mult_param2 = 0;
					sscanf(keywords.array[i - 1], "%lf", &mult_param1);
					sscanf(keywords.array[i + 1], "%lf", &mult_param2);
					const double mult_expr_result = mult_param1 * mult_param2;

					// Get the first number in the expression and allocate memory for the new value replacing it
					keywords.array[i - 1] = malloc(sizeof(mult_expr_result)+1);
					sprintf(keywords.array[i - 1], "%lf", mult_expr_result);

					for (int j = i; j < expr_end_pos; j++)
					{
						if(j+2 >= keywords.length)
						{
							keywords.array[j] = NULL;
							free(keywords.array[j]);
							continue;
						}
						keywords.array[j] = malloc(sizeof(keywords.array[j + 2] + 1));
						strcpy(keywords.array[j], keywords.array[j + 2]);
					}

					// Now that the keywords array is 2 keywords shorter, we need to decrement the expression's end-position by 2.
					expr_end_pos = expr_end_pos - 2;
					keywords.length = keywords.length - 2;
					break;

				// DIVISION
				case '/':

					// Exit early if the syntax is invalid
					if (i < 2 || i == keywords.length - 1)
					{
						fprintf(stderr, "Invalid division syntax!\n");
						exit(EXIT_FAILURE);
					}

					// Convert from strings to doubles, divide param1 by param2 and get the result.
					double div_param1 = 0;
					double div_param2 = 0;
					sscanf(keywords.array[i - 1], "%lf", &div_param1);
					sscanf(keywords.array[i + 1], "%lf", &div_param2);

					// Ensure it doesn't divide by zero.
					if(div_param1 == 0 || div_param2 == 0)
					{
						fprintf(stderr, "Divide by zero error.\n");
						exit(EXIT_FAILURE);
					}

					const double div_expr_result = div_param1 / div_param2;

					// Get the first number in the expression and allocate memory for the new value replacing it
					keywords.array[i - 1] = malloc(sizeof(div_expr_result) + 1);
					sprintf(keywords.array[i - 1], "%lf", div_expr_result);

					for (int j = i; j < expr_end_pos; j++)
					{
						if (j + 2 >= keywords.length)
						{
							keywords.array[j] = NULL;
							free(keywords.array[j]);
							continue;
						}
						keywords.array[j] = malloc(sizeof(keywords.array[j + 2] + 1));
						strcpy(keywords.array[j], keywords.array[j + 2]);
					}

					// Now that the keywords array is 2 keywords shorter, we need to decrement the expression's end-position by 2.
					expr_end_pos = expr_end_pos - 2;
					keywords.length = keywords.length - 2;
					break;

				// ADDITION
				case '+':
					// Exit early if the syntax is invalid
					if (i < 2 || i == keywords.length - 1)
					{
						fprintf(stderr, "Invalid addition syntax!\n");
						exit(EXIT_FAILURE);
					}

					// Convert from strings to doubles, divide param1 by param2 and get the result.
					double add_param1 = 0;
					double add_param2 = 0;
					sscanf(keywords.array[i - 1], "%lf", &add_param1);
					sscanf(keywords.array[i + 1], "%lf", &add_param2);
					const double add_expr_result = add_param1 + add_param2;

					// Get the first number in the expression and allocate memory for the new value replacing it
					keywords.array[i - 1] = malloc(sizeof(add_expr_result) + 1);
					sprintf(keywords.array[i - 1], "%lf", add_expr_result);

					for (int j = i; j < expr_end_pos; j++)
					{
						if (j + 2 >= keywords.length)
						{
							keywords.array[j] = NULL;
							free(keywords.array[j]);
							continue;
						}
						keywords.array[j] = malloc(sizeof(keywords.array[j + 2] + 1));
						strcpy(keywords.array[j], keywords.array[j + 2]);
					}

					// Now that the keywords array is 2 keywords shorter, we need to decrement the expression's end-position by 2.
					expr_end_pos = expr_end_pos - 2;
					keywords.length = keywords.length - 2;
					break;

				// SUBTRACTION
				case '-':
					// Exit early if the syntax is invalid
					if (i < 2 || i == keywords.length - 1)
					{
						fprintf(stderr, "Invalid subtraction syntax!\n");
						exit(EXIT_FAILURE);
					}

					// Convert from strings to doubles, divide param1 by param2 and get the result.
					double sub_param1 = 0;
					double sub_param2 = 0;
					sscanf(keywords.array[i - 1], "%lf", &sub_param1);
					sscanf(keywords.array[i + 1], "%lf", &sub_param2);
					const double sub_expr_result = sub_param1 - sub_param2;

					// Get the first number in the expression and allocate memory for the new value replacing it
					keywords.array[i - 1] = malloc(sizeof(sub_expr_result) + 1);
					sprintf(keywords.array[i - 1], "%lf", sub_expr_result);

					for (int j = i; j < expr_end_pos; j++)
					{
						if (j + 2 >= keywords.length)
						{
							keywords.array[j] = NULL;
							free(keywords.array[j]);
							continue;
						}
						keywords.array[j] = malloc(sizeof(keywords.array[j + 2] + 1));
						strcpy(keywords.array[j], keywords.array[j + 2]);
					}

					// Now that the keywords array is 2 keywords shorter, we need to decrement the expression's end-position by 2.
					expr_end_pos = expr_end_pos - 2;
					keywords.length = keywords.length - 2;
					break;

				default:
					break;
				}
				// This will break the for loop and restart the while loop
				break;
			}
		}

		count++;
	}

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
	if(ml_check_variable(keywords.array[expr_start_pos]))
	{
		char* retrieved_var_s = "";
		const double retrieved_var_d = ml_retrieve_variable(keywords.array[expr_start_pos]);
		retrieved_var_s = malloc(sizeof(retrieved_var_d) + 1);
		dtos(retrieved_var_d, retrieved_var_s);
		strcpy(stream, retrieved_var_s);
		return;
	}
	
	return;
}

void ml_add_function(char* name, int line_no)
{
	// I'm thinking we store an array of structs which stores function info.
	// We need to store the line number at which the function starts,
	// so we can ignore the rest of the function and not process it until we need to.
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
