#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <sys/wait.h>
#include <stdlib.h>

// Function to read a single line from a file
int read_line(int infile, char *buffer, int maxlen) {
    int readlen = 0;
    char ch;
    while (readlen < maxlen - 1) {
        int bytes_read = read(infile, &ch, 1);
        if (bytes_read == 0) break;  // End of file
        if (bytes_read < 0) {
            perror("Error reading the file");
            return -1;
        }
        buffer[readlen++] = ch;
        if (ch == '\n') break;  
    }
    buffer[readlen] = '\0';  
    return readlen;
}

void parse_command(char *buffer, char **command, int *numtokens) {
    int inside_quotes = 0;  
    char *start = buffer;   
    *numtokens = 0;         
    
    while (*buffer) {
        if (*buffer == '\"') {  
            inside_quotes = !inside_quotes;  
            if (inside_quotes) {
                start = buffer + 1;  
            } else {
                *buffer = '\0';  
                command[(*numtokens)++] = start;  
                start = buffer + 1;  
            }
        } else if ((*buffer == ' ' || *buffer == '\n') && !inside_quotes) {
            if (start != buffer) {  
                *buffer = '\0';     
                command[(*numtokens)++] = start;  // Store token
            }
            start = buffer + 1;  // Move to the next token
        }
        buffer++;
    }
    
    if (start != buffer && !inside_quotes) {
        command[(*numtokens)++] = start;
    }
    
    command[*numtokens] = NULL;  // Null-terminate the array
}


void execute_command(char **command) {
   

    pid_t pid = fork();  
    if (pid == 0) {
        if (command[0][0] == '/') {
            // Absolute path
            execv(command[0], command);  d
            perror("Error executing command");  // If execv fails
            exit(1);
        } else if (strchr(command[0], '/') != NULL) {
            // Relative path
            execv(command[0], command);  
            perror("Error executing command");  // If execv fails
            exit(1);
        } else {
            execvp(command[0], command); 
            perror("Error finding command");  // If execvp fails
            exit(1);
        }
    } else if (pid > 0) {
        wait(NULL);  
    } else {
        perror("Fork failed");  
    }
}

int main(int argc, char *argv[]) {
    if (argc != 2) {
        printf("Usage: %s <input file>\n", argv[0]);
        return -1;
    }

    // Open the input file
    int infile = open(argv[1], O_RDONLY);
    if (infile < 0) {
        perror("Error opening input file");
        return -2;
    }

    char buffer[1024];
    char *command[64];  
    int readlen;

    while (1) {
        readlen = read_line(infile, buffer, 1024);
        if (readlen < 0) {
            perror("Error reading input file");
            return -3;
        }
        if (readlen == 0) {
            break;  
        }

        int numtokens = 0;
        parse_command(buffer, command, &numtokens);

        // Execute the tokenized command
        if (numtokens > 0) {
            execute_command(command);
        }
    }

    close(infile);  // Close the input file
    return 0;
}
