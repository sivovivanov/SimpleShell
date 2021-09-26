/*******************************************************************
* Assessment Title: Simple Shell Exercise, Assessed Coursework Exercise #4
* 
* Number of Submitted C Files: 2
*
* Date 21.03.2018
*
*
* Author: Julia Tennant, Reg no: 201616633
* Author: Steven Brown, Reg no: 201608874
* Author: Slav Ivanov, Reg no:	201645797
*
*
* Personal Statement: I confirm that this submission is all our own work.
*			
*			Julia Tennant
*			Steven Brown
*			Slav Ivanov
*
*
*
*******************************************************************/

#include<stdio.h>
#include<string.h>
#include<stdlib.h>
#include<unistd.h>
#include<sys/types.h>
#include<sys/wait.h>
#include<ctype.h>
#include<stdbool.h>

// Constants used throughout program
#define INPUT_LENGTH 513
#define TOKENS 10
#define DELIMITERS 9
#define HISTORY_LIMIT 20
#define ALIASES_LIMIT 10

// Global variables
// Used to hold the original path when the program starts, is then restored in exiting() function.
char *original_path;
char* history[HISTORY_LIMIT][INPUT_LENGTH];
// Indicates at which position the next command in history is.
int next_cmd = 0;
// Structure used for storing aliases.
struct aliases_storage
{
    char alias[INPUT_LENGTH];
    char command[INPUT_LENGTH];
};
// Array of aliases names holding all 10 of the alises.
struct aliases_storage aliasArray[ALIASES_LIMIT];

// Function declaration
void exiting();
void readInput();
void tokenise(char user_input[], int hist_list_present, int alias_present);
void addToHist(char* strArray[], int alias_present);
void readTokens(char* strArray[]);
void printHistory(char* strArray[]);
void exclamationMarks(char* strArray[]);
void convertHistory(int desired_row);
void saveHistory();
void loadHistory();
void createAliases();
bool findCycle(char alias[], char command[], char* strArray[], int freespace, int recursive);
void addAlias(char* strArray[]);
void removeAlias(char* strArray[]);
void saveAliases();
void loadAliases();
int forking(char* strArray[]);

int main()
{
	original_path = getenv("PATH");
	chdir(getenv("HOME"));
	
	createAliases();
	loadAliases();
	loadHistory();
	
	while(1)
	{
		readInput();
	}
	return 0;
}

// Function exiting(), exits the program, saving the aliases and history and restoring the path.
// It is called whenever "exit" is entered or CTRL + D is pressed.
void exiting()
{
	setenv("PATH",original_path,1);
	printf("\n%s\nExiting...\n", getenv("PATH"));
	saveHistory();
	saveAliases();
	exit(0);
}

// Takes in user input
// Called constantly in the while loop in the main. Stores input in an array of characters.
// Passes the input to tokenise() with history_present and alias_present set to 0.
void readInput()
{
	//User input
	char user_input[INPUT_LENGTH];
	printf("> ");
	
	// Check if CTRL + D is pressed
	if (fgets(user_input, INPUT_LENGTH - 1, stdin) == NULL)
	{
    		exiting();
   	} 
    
	else
	{
		tokenise(user_input, 0, 0);
	}
}

// Splits input into seperate tokens and stores each in an array.
// Adds tokens to history.
// When history_present and alias_present are set to 0 it then passes the array of tokens to readTokens().
void tokenise(char user_input[], int hist_list_present, int alias_present) 
{
	int i = 0;
	char* strArray[TOKENS] = {" "};
	const char delim[DELIMITERS] = {' ', '\t', '>', '<', '&', ';', '\n', '|'};
	char *token = strtok(user_input, delim);
	
	// Puts tokens in new array
	while(token != NULL)
    {
		strArray[i] = malloc(strlen(token) + 1);
		strcpy(strArray[i], token);
		token = strtok(NULL, delim);
		i++;
 	}
	addToHist(strArray, alias_present);
	
	// Passes array with tokens to be executed
	if(hist_list_present == 0)
	{
		readTokens(strArray);
	}
}

// Adds commands to history.
// Takes in tokenised input and adds it to the global history array.
void addToHist(char* strArray[], int alias_present)
{
	int counter = 0;
	int skiploop = 0;
	
	// Don't save aliases of !! and !-1 (prevents infinite loop)
	if(alias_present == 1 && ((strcmp(strArray[0], "!!") == 0 || strcmp(strArray[0], "!-1") == 0 || strchr(strArray[0], '!') != NULL)))
	{
		next_cmd--;
		return;
	}
	
	// Copies strArray to history[][]
 	// First in, last out:
 	// Fills rows 0 - 19, then starts again at 0
 	for(int i = 0; strArray[i] != NULL; i++)
 	{
 		// counter gets the position of the next argument,
 		// used for filling NULLs in once we've copied the command into history
 		counter++;
 		
 		// Don't save !commands, emptiness nor the command an alias executes
 		if((*strArray[i] == '!' && strcmp(strArray[0], "alias") != 0)|| strArray[i] == " " || alias_present == 1)
		{
			next_cmd--;
			skiploop = 1;
			break;
		}
		
 		if(next_cmd == HISTORY_LIMIT)
 		{
 			// Going back to row 0, now it's starting to overwrite the history with the newer commands entered
 			next_cmd = 0;
 		}
 		// Adds command to history
		history[next_cmd][i] = strArray[i];
	}
	
	// Fill rest of arguments with null to prevent arguments passing onto later commands
	for(int i = counter; skiploop == 0 && i < TOKENS; i++)
	{
		history[next_cmd][i] = NULL;
	}
	
	// Position/row of where the next command should be stored in history[][]
	next_cmd++;
}

// Reads tokens and detects if any specific kind of input is entered.  
// Looks out for non-internal commands.
// Otherwise passes strArray[] to forking() function.
void readTokens(char* strArray[])
{
	for(int i = 0; i < ALIASES_LIMIT; i++)
	{
		if(strcmp(strArray[0], aliasArray[i].alias) == 0)
		{
			// Checking for alias a !2, a
			if(strchr(aliasArray[i].command, '!') != NULL)
			{
				char temp[3];
				if(strchr(aliasArray[i].command, '-') != NULL)
				{
					strcpy(temp, strchr(aliasArray[i].command, '!') + 2);
				}
				
				else
				{
					strcpy(temp, strchr(aliasArray[i].command, '!') + 1);
				}
				int temp2 = atoi(temp);
				
				if(history[temp2-1][0] == NULL)
				{
					printf("No command here.\n");
					return;
				}
			
				if(strcmp(strArray[0], history[temp2-1][0]) == 0 && temp2 == next_cmd)
				{
					printf("This would create a cycle.\n");
					return;
				}
				
				else
				{
					tokenise(aliasArray[i].command, 0, 1);
					return;
				}
				
			}
			
			else
			{
				// Saves the command arguments that get lost during tokenising
				char temp[INPUT_LENGTH];
				strcpy(temp, aliasArray[i].command);
			
				// If user types arguments after the alias name
				for(int j = 1; j < TOKENS && strArray[j] != NULL; j++)
				{
					strcat(aliasArray[i].command, strArray[j]);
					strcat(aliasArray[i].command, " ");
				}

				tokenise(aliasArray[i].command, 0, 1);
			
				// Restoring the "lost" arguments
				strcpy(aliasArray[i].command, temp);
				strcat(aliasArray[i].command, " ");
				return;
			}
			

		}
	}
	
	if(strcmp(strArray[0], "exit") == 0)
	{
		if(strArray[1] != NULL)
		{
			printf("Invalid argument entered.\n");
		}
		else
		{	
			printf("Exiting...\n");	
			exiting();
		}
	}
	
	else if(strcmp(strArray[0], "getpath") == 0)
	{
		if(strArray[1] != NULL)
		{
			printf("Invalid argument entered.\n");
		}
		else
		printf("%s\n", getenv("PATH"));
	}
	
	else if(strcmp(strArray[0], "setpath") == 0)
	{
		if(strArray[1] == NULL)
		{
			printf("Path not entered.\n");
		}
		
		else if(strArray[2] != NULL)
		{
			printf("Invalid path entered. Use less arguments.\n");
		}
		 
		else
		{
			setenv("PATH", strArray[1], 1);
			printf("Path set to: %s\n", strArray[1]);
		}
	}
	
	else if(strcmp(strArray[0], "cd") == 0)
	{
		if(strArray[1] == NULL)
		{
			chdir(getenv("HOME"));
		}
		else
		{			
			if(strArray[2] != NULL)
			{
				printf("Invalid directory entered. Use less arguments.\n");
			}
			
			else if(chdir(strArray[1]) != 0)
			{
				fprintf(stderr, "%s - ", strArray[1]);
				perror("");
			}
		}
	}
	
	else if(strchr(strArray[0], '!') != NULL)
	{
		exclamationMarks(strArray);
	}
	
	else if(strcmp(strArray[0], "history") == 0)
	{
		printHistory(strArray);
	}
	
	else if (strcmp(strArray[0], "alias") == 0)
	{
		addAlias(strArray);
	}
	
	else if(strcmp(strArray[0], "unalias") == 0)
	{
		removeAlias(strArray);
	}
	
	else if(strcmp(strArray[0], " ") != 0)
	{
		forking(strArray);
	}
}

// Prints the contents of history whenever just "history" is passed to readTokens()
void printHistory(char* strArray[])
{
	if(strArray[1] != NULL)
	{
		printf("This command doesn't take any arguments.\n");
	}
	
	else
	{
		int command_number = 1;
		
		// If user has done < HISTORY_LIMIT number of commands
		if(history[HISTORY_LIMIT - 1][0] == NULL)
		{
			for(int i = 0; i < HISTORY_LIMIT && history[i][0] != NULL; i++)
			{
				printf("%d. ",command_number);
				for(int j = 0; j < TOKENS && history[i][j] != NULL; j++)
				{
					printf("%s ",history[i][j]);
				}
				printf("\n");
				command_number++;
			}
		}
		
		else
		{
			// Print from the first command executed -> end of history[][]
			for(int i = next_cmd; i < HISTORY_LIMIT; i++)
			{
				printf("%d. ",command_number);
				for(int j = 0; j < TOKENS && history[i][j] != NULL; j++)
				{
					printf("%s ",history[i][j]);
				}
				printf("\n");
				command_number++;
			}
		
			// Print the rest
			for(int i = 0; i < next_cmd; i++)
			{
				printf("%d. ",command_number);
				for(int j = 0; j < TOKENS && history[i][j] != NULL; j++)
				{
					printf("%s ",history[i][j]);
				}
				printf("\n");
				command_number++;
			}
		}
	}
}

// This function is called whenever one of the three exclamation mark commands is needed. 
// If the user typed in !!, !<no.> or !!<-no>
void exclamationMarks(char* strArray[])
{
	if(strArray[1] != NULL)
	{
		printf("This command doesn't take arguments.\n");
	}
	else if(strlen(strArray[0]) > 4)
	{
		printf("Input for \"!\" is too big.\n");
	}
	else
	{
		char desired_row_temp[3];
		int desired_row;	
		// Copies the number entered after ! into temp array
		strcpy(desired_row_temp, strchr(strArray[0], '!') + 1);
		
		// !!
		if(desired_row_temp[0] == '!')
		{
			if(desired_row_temp[1] != '\0')
			{
				printf("Nothing should be entered after \"!!\".\n");
				return;
			}
			
			desired_row = next_cmd - 1;
			if(desired_row == -1)
			{
				desired_row = HISTORY_LIMIT - 1;
			}
			
			if(history[desired_row][0] == NULL)
			{
				printf("No previous commands executed.\n");
				return;
			}
		}
		
		// !-<no.>
		else if(desired_row_temp[0] == '-')
		{
			if(!isdigit(desired_row_temp[1]) || (!isdigit(desired_row_temp[2]) && desired_row_temp[2] != '\0'))
			{
				printf("Only enter a number after \"!-\".\n");
				return;
			}
			
			// Removing the -ve before converting char -> int
			desired_row_temp[0] = '0';
			desired_row = atoi(desired_row_temp);
			
			if(desired_row < 1 || desired_row > HISTORY_LIMIT)
			{
				printf("Cannot view history that far back.\n");
				return;
			}
			
			// Changing desired row to the row they actually need
			desired_row = next_cmd - atoi(desired_row_temp);
			if(desired_row < 0)
			{
				desired_row = HISTORY_LIMIT + desired_row;
			}
			
			if(history[desired_row][0] == NULL)
			{
				printf("Trying to view non-existent command.\n");
				return;
			}
		}
		
		// !<no.>
		else
		{
			if(!isdigit(desired_row_temp[0]) || (isdigit(desired_row_temp[0]) && !isdigit(desired_row_temp[1])) && desired_row_temp[1] != '\0' || (isdigit(desired_row_temp[1]) && !isdigit(desired_row_temp[2]) && desired_row_temp[2] != '\0'))
			{
				printf("Invalid request - only enter a number, - or ! after \"!\".\n");
				return;
			}
			// Converts temp array to integer
			desired_row = atoi(desired_row_temp);
			
			if(desired_row == 0)
			{
				printf("Invalid request - no command 0.\n");
				return;
			}
			
			if(desired_row >= HISTORY_LIMIT + 1)
			{
				printf("Cannot view history that far back.\n");
				return;
			}
			
			if(history[desired_row - 1][0] == NULL)
			{
				printf("Trying to view non-existent command.\n");
				return;
			}
			
			// fakelist stores the order commands where executed in
			// Each position in the array stores the actual position/row of where that command is in history[][]
			int fakelist[HISTORY_LIMIT];
			
			// If user has done < HISTORY_LIMIT number of commands
			if(history[HISTORY_LIMIT - 1][0] == NULL)
			{
				for(int i = 0; i < HISTORY_LIMIT; i++)
				{
					fakelist[i] = i;
				}
			}
			
			else
			{
				int j = 0;
				for(int i = next_cmd; i < HISTORY_LIMIT; i++)
				{
					fakelist[j] = i;
					j++;
				}
			
				for(int i = 0; i < next_cmd; i++)
				{
					fakelist[j] = i;
					j++;
				}
			}
			// Finally getting the actual row we need from history[][]!!!
			desired_row = fakelist[desired_row - 1];
		}
		// Converts the command + arguments we want into suitable format before executing
		convertHistory(desired_row);
	}
}

void convertHistory(int desired_row)
{
	char* desired_cmd[TOKENS] = {" "};
	for(int i = 0; i < TOKENS; i++)
	{
		desired_cmd[i] = history[desired_row][i];
	}
	readTokens(desired_cmd);
}

// Saves history to text file being held in the home directory
void saveHistory()
{
	chdir(getenv("HOME"));
	FILE *f = fopen(".hist_list", "w");
	
	// If user has done < HISTORY_LIMIT number of commands
	if(history[HISTORY_LIMIT - 1][0] == NULL)
	{
		for(int i = 0; i < HISTORY_LIMIT && history[i][0] != NULL; i++)
		{
			for(int j = 0; j < TOKENS && history[i][j] != NULL; j++)
			{
				fprintf(f, "%s ", history[i][j]);
			}
			fputs("\n", f);
		}
	}
	else
	{
		// Print from the first command executed -> end of history[][]
		for(int i = next_cmd; i < HISTORY_LIMIT; i++)
		{
			for(int j = 0; j < TOKENS && history[i][j] != NULL; j++)
			{
				fprintf(f, "%s ",history[i][j]);
			}
			fputs("\n", f);
		}

		// Print the rest
		for(int i = 0; i < next_cmd; i++)
		{
			for(int j = 0; j < TOKENS && history[i][j] != NULL; j++)
			{
				fprintf(f, "%s ",history[i][j]);
			}
			fputs("\n", f);
		}
	}
	fclose(f);
}

// Loads history into a character array.
// Then that character array is passed to tokenise, with history_present being set to 1.
// That makes sure the commands get added to history, but not immediatelly executed after that.
void loadHistory()
{	
	// txt_in stores one line from .hist_list
	char txt_in[INPUT_LENGTH] = {' '};
	
	// Load text file with instructions
	FILE *f = fopen(".hist_list", "r");
	if(f)
	{
		while(1)
		{
			// Checks if .hist_list is empty
			if(fgets(txt_in, INPUT_LENGTH - 1, f) == NULL)
			{
				break;
			}
			
			// Feed the contents of .hist_list to be processed(tokenised)
			else
			{
				tokenise(txt_in, 1, 0);
			}
		}	
		fclose(f);
	}	
}

// Initialises alias structures at each index in the array. Called before the main while loop.
void createAliases()
{
	for(int i = 0; i < ALIASES_LIMIT; i++)
	{
		struct aliases_storage aliasArray[i];
	}
}

// Recursive function looking for a cycle when defining aliases.
// There is no cycle if the recursive calls "&&"-up to a true.
bool findCycle(char alias[], char command[], char* strArray[],  int freespace, int recursive)
{
	int i = 0;
	char* temp[TOKENS] = {" "};
	const char delim[DELIMITERS] = {' ','\n'};
	char *token = strtok(aliasArray[freespace].command, delim);
	
	// Puts tokens in new array
	while(token != NULL)
	{
		temp[i] = malloc(strlen(token) + 1);
		strcpy(temp[i], token);
		token = strtok(NULL, delim);
		i++;
	}

	if(strcmp(temp[0], alias) == 0)
	{
		return true && findCycle(aliasArray[freespace].alias, aliasArray[freespace].command, strArray, freespace - 1, recursive);
	}
	
	else if(strcmp(alias, strArray[2]) == 0)
	{
		return false;
	}
	
	else
	{
		for(i = 0; i < ALIASES_LIMIT; i++)
		{
			// Check if alias = an aliased command
			if(strcmp(alias, aliasArray[i].command) == 0 || recursive == 1)
			{	
				// If alias (x) = an aliased command (y), check if the command (y) = an existing alias (z)
				for(int j = 0; j < ALIASES_LIMIT; j++)
				{
					if(strcmp(command, aliasArray[j].alias) == 0)
					{
						// findCycle(z, z's command, strArray, resetting freespace, recursive set to 1)
						if(!(findCycle(aliasArray[j].alias, aliasArray[j].command, strArray, ALIASES_LIMIT, 1)))
						{
							return false;
						}
					}
				}
			}
		}
		return true;
	}	
}

// Adds an alies to the initialized alias array. Checks if just "alias" is entered
void addAlias(char* strArray[])
{
	if(strArray[2] == NULL)
	{
		if(strArray[1] != NULL)
		{
			printf("Please enter at least two arguments for alias.\n");
			
		}
				
		// Print all the aliases if users gives no arguments
		else
		{
			if (strcmp(aliasArray[0].alias, "") == 0)
			{
					printf("No aliases to show \n");
			}
			else
			{
				int i = 0;
			
				printf("Aliases --- Commands\n");
				while(strcmp(aliasArray[i].alias, "") != 0)
				{
					printf("%d. %s --- %s\n", i+1, aliasArray[i].alias, aliasArray[i].command);
					i++;
				}
			}
		}
	}
	
	else if(strcmp(aliasArray[ALIASES_LIMIT-1].alias, "") == 0)
	{
		// Finding space to add an alias
		int freespace = 0;
		while(strcmp(aliasArray[freespace].alias, "") != 0)
		{
			if(strcmp(strArray[1], aliasArray[freespace].alias) == 0)
			{
				printf("Alias already exists with that name.\n");
				return;
			}
			freespace++;
		}
		
		if(findCycle(strArray[1], strArray[2], strArray, freespace - 1, 0))
		{
			// Adding the alias, command and arguments
			strcpy(aliasArray[freespace].alias, strArray[1]);
			for(int i = 2; i < TOKENS && strArray[i] != NULL; i++)
			{
				strcat(aliasArray[freespace].command, strArray[i]);
				strcat(aliasArray[freespace].command, " ");
			}	
		}
		else
		{
			printf("This would create a cycle.\n");
		}
	}
	
	else
	{
		printf("Aliases full.\n");
	}
}

// Removes an alies specified by the alias name.
// Clears both the alias as well as the command.
void removeAlias(char* strArray[])
{
	if(strArray[1] == NULL)
	{
		printf("Please enter a valid number of arguments for unalias.\n");
	}
	else
	{
		int shift = 0;
		for(int i = 0; i < ALIASES_LIMIT; i++)
		{
			if(shift == 0 && strcmp(aliasArray[0].alias, "") == 0)
			{
				printf("There are no aliases to remove.\n");
				break;
			}
			
			// Removing the alias
			if(strcmp(strArray[1], aliasArray[i].alias) == 0)
			{
				printf("Alias removed: %s\n", aliasArray[i].alias);
				strcpy(aliasArray[i].alias, "");
				strcpy(aliasArray[i].command, "");
				shift = 1;
			}
			
			// Shifting everything up
			if(shift == 1)
			{
				if(i+1 == ALIASES_LIMIT)
				{
					strcpy(aliasArray[i].alias, "");
					strcpy(aliasArray[i].command, "");
				}
				else
				{
					strcpy(aliasArray[i].alias, aliasArray[i+1].alias);
					strcpy(aliasArray[i].command, aliasArray[i+1].command);
				}
			}
		}
		
		if(shift == 0)
		{
			printf("There is no such alias to remove.\n");
		}
	}
}

// Save aliases to text file in home directory
void saveAliases()
{
	chdir(getenv("HOME"));
	FILE *f = fopen(".aliases", "w");
	
	for(int i = 0; i < ALIASES_LIMIT && strcmp(aliasArray[i].alias, "") != 0; i++)
	{
		fprintf(f, "%s ", aliasArray[i].alias);
		fputs(" ", f);
		fprintf(f, "%s ", aliasArray[i].command);
		fputs("\n", f);
	}	
	fclose(f);
}

// Load aliases from text file in home directory (used on startup)
void loadAliases()
{
	char txt_in[INPUT_LENGTH] = {' '};
	char alias[INPUT_LENGTH] = "alias ";
	
	FILE *f = fopen(".aliases", "r");
	if(f)
	{
		while(1)
		{
			if(fgets(txt_in, INPUT_LENGTH - 1, f) == NULL)
			{
				break;
			}
			else
			{
				strcat(alias, txt_in);
				tokenise(alias, 0, 1);
				strcpy(alias, "alias ");
			}
		}	
		fclose(f);
	}
}

// Execute programs
int forking(char* strArray[])
{
		pid_t pid;
		
		//Fork a child process
		pid = fork();
		
		//Error occured
		if(pid < 0)
		{
			fprintf(stderr, "Fork Failed.");
			exit(-1);
		}
		
		//Child process
		else if (pid == 0)
		{
			if((execvp(strArray[0], strArray)) == -1)
			{
				fprintf(stderr, "%s - ", strArray[0]);
				perror("");
			}
			exit(0);
		}

		//Parent process		
		else
		{	
			//Parent waits for child to complete
			wait(NULL);
			printf("Child complete.\n");
		}
}