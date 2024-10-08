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

void parse_command(char *buffer, char **command, int *numtokens, char **outfile) {
    int inside_quotes = 0;  
    char *start = buffer;   
    *numtokens = 0;
    *outfile = NULL; // Initialize outfile to NULL to check if redirection is required

    while (*buffer) {
        if (*buffer == '>') {  // Handle output redirection
            *buffer = '\0';  // Terminate the previous token (if any)
            buffer++;
            while (*buffer == ' ') buffer++;  // Skip any spaces after `>`
            *outfile = buffer;  // Output file name starts here
            while (*buffer && *buffer != ' ' && *buffer != '\n') buffer++;  // Find end of file name
            *buffer = '\0';  // Terminate the output file name
            buffer++;  // Move past '\0'
            start = buffer;  // Update start to current buffer position
            break;
        }
        if (*buffer == '"') {  
            inside_quotes = !inside_quotes;  
            if (inside_quotes) {
                start = buffer + 1;  
            } else {
                *buffer = '\0';  
                if (start != buffer) {
                    command[(*numtokens)++] = start;  // Store token only if not empty
                }
                start = buffer + 1;  
            }
        } else if ((*buffer == ' ' || *buffer == '\n') && !inside_quotes) {
            if (start != buffer) {  
                *buffer = '\0';     
                command[(*numtokens)++] = start;  // Store token only if not empty
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

void execute_command(char **command, char *outfile) {
    pid_t pid = fork();  
    if (pid == 0) {  // Child process
        if (outfile != NULL) {  // If output redirection is required
            int fd = open(outfile, O_CREAT | O_WRONLY | O_TRUNC, 0644);
            if (fd < 0) {
                perror("Error opening output file");
                exit(1);
            }
            // Redirect stdout to the file
            dup2(fd, STDOUT_FILENO);
            close(fd);
        }
        if (command[0][0] == '/') {
            // Absolute path
            execv(command[0], command);
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
    char *outfile;
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
        parse_command(buffer, command, &numtokens, &outfile);

        // Execute the tokenized command
        if (numtokens > 0) {
            execute_command(command, outfile);
        }
    }

    close(infile);  // Close the input file
    return 0;
}