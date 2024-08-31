#include "Project1.h"



#pragma warning(disable :5045) // NEED to remove this long before final draft. This disables a warning that linux might not have.

int main(int argc, char *argv[])
{
	// these two lines are just to get rid of a warning.
	if (argc < 2) { return 0; }
	if (argv[1] == NULL) { return 0; }

	// Opens the file and provides an error if it can't find it
	FILE *sample = fopen("sample02.ml", "r");
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
		if (strcmp(keywords.array[i], "#") == 0) 
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

void ml_assign_variable(char* name, const double value)
{
	for (int i = 0; i < 64; i++)
	{
		if (strcmp(memory_cache[i]->name, "") == 0)
		{
			memory_cache[i]->name = name;
			memory_cache[i]->value = value;
			return;
		}
	}
	printf("VARIABLE ASSIGNMENT INVALID: OUT OF MEMORY\n");
}

void ml_print(string_array keywords)
{
	// Check if the first keyword is "print". If it isn't, then the syntax is invalid.
	if (strcmp(keywords.array[0], "print") != 0)
	{
		printf("PRINT FUNCTION ERROR : INVALID PRINT PARAMS.\n");
		return;
	}

	char* to_print[256];

	// Check if we need to do an expression for some reason.
	if (keywords.length > 2)
	{
		
	}
}

// This is just for debugging. An easy-to-call function that prints an array of strings.
void print_strings(string_array *strings)
{
	for (int i = 0; i < strings->length; i++)
	{
		printf("Print Each %i: %s\n", i, strings->array[i]);
	}
}

char* calc_expression(string_array keywords, int expr_start_pos, int expr_end_pos)
{
	for (int i = expr_start_pos; i < expr_end_pos; i++)
	{
		if (strlen(keywords.array[i]) == 1 && !isalpha(keywords.array[i][0]))
		{
			switch (keywords.array[i][0])
			{
			case '+':
				break;

			case '-':
				break;

			case '*':
				break;

			case '/':
				break;

			default: break;
			}
		}
	}
	
}
