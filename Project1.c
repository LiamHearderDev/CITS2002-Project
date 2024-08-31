#include "Project1.h"



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

		//char* arr[64];
		string_array keywords;
		extract_keywords(buffer, &keywords);

		process_keywords(keywords);
		//print_strings(&keywords);
	}

	return 1;
}

// This function will extract the keywords from a line.
// Takes two inputs: a line (char*), and a token_stream (char*[]).
// Param "line" is for the input text from which keywords are extracted.
// Param "token_stream" is a char ptr array and is where the keywords output is written to.
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

int process_keywords(string_array keywords)
{
	const int keyword_count = keywords.length;
	for (int i = 0; i < keywords.length; i++)
	{
		//printf("KEYWORD: %s\n", keywords[i]);
		if (strcmp(keywords.array[i], "#") == 0) { printf("COMMENT\n"); return 0; }

		if (strcmp(keywords.array[i], "<-") == 0)
		{
			if (keyword_count - 1 < 0 || keyword_count + 1 > 63 || i == 0)
			{
				printf("VARIABLE ASSIGNMENT INVALID: ACCESS VIOLATION."); return 0;
			}
			assign_variable(keywords.array[i - 1], 0);
			//assign_variable(keywords.array[i - 1], strtod(keywords.array[i + 1], &keywords.array[keyword_count-1]));
		}
	}
	return 1;
}

void assign_variable(char* name, const double value)
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
}

// This is just for debugging. An easy to call function that prints an array of strings.
void print_strings(string_array *strings)
{
	for (int i = 0; i < strings->length; i++)
	{
		printf("Print Each %i: %s\n", i, strings->array[i]);
	}
}
