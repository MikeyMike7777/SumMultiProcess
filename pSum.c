/** 
 * file:  pSum.c 
 * author:  Michael Mathews 
 * course: CSI 3336 
 * assignment: Project 8 
 * due date:  4/17/2023 
 * 
 * date modified: 4/17/2023 
 *      - file created 
 * 
 * spawns a specified number of child processes (less than 100) and adds up
 * values in the child processes. then, the parent process finds the total sum
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>

#define READ_END 0
#define WRITE_END 1

#define MAX_CHILDREN 100

int main(int argc, char *argv[]) {
    // check command line arguments
    if (argc != 3) {
        fprintf(stderr, "Usage: %s <filename> <number of child processes>\n", argv[0]);
        return 1;
    }

    // open the file for reading
    FILE *file = fopen(argv[1], "rb");
    if (file == NULL) {
        perror("Failed to open file");
        return 1;
    }

    // get the file size
    fseek(file, 0L, SEEK_END);
    long fileSize = ftell(file);
    rewind(file);
    // read the numbers from the file into an array
    int *numbers = malloc(fileSize / 4);
    fread(numbers, fileSize, 1, file);
    fclose(file);

    // calculate how many numbers each child should sum
    int numChildren = atoi(argv[2]);
    int numNumbersPerChild = fileSize / 4 / numChildren;
    int numExtraNumbers = fileSize / 4 % numChildren;
    // create pipe for communication with child processes
    int pipefd[2];
    if (pipe(pipefd) == -1) {
        perror("Failed to create pipe");
        return 1;
    }

    // spawn child processes
    pid_t *childPIDs = malloc(numChildren * sizeof(pid_t));
    int i;
    for (i = 0; i < numChildren; i++) {
        pid_t pid = fork();
        if (pid == -1) {
            perror("Failed to fork child process");
            return 1;
        } else if (pid == 0) {
            // child process
            close(pipefd[READ_END]); // close unused read end of pipe
            int start = i * numNumbersPerChild;
            int end = start + numNumbersPerChild;
            if (i == numChildren - 1) {
                // last child process
                end += numExtraNumbers;
            }
            int sum = 0, j;
            for (j = start; j < end; j++) {
                sum += numbers[j];
            }
            write(pipefd[WRITE_END], &sum, sizeof(sum));
            close(pipefd[WRITE_END]); // close write end of pipe
            exit(0);
        } else {
            // parent process
            childPIDs[i] = pid;
        }
    }

    close(pipefd[WRITE_END]); // close unused write end of pipe
    // wait for all child processes to finish and collect their results
    int totalSum = 0, childSum;
    for (i = 0; i < numChildren; i++) {
        read(pipefd[READ_END], &childSum, sizeof(childSum));
        totalSum += childSum;
        waitpid(childPIDs[i], NULL, 0);
    }
    close(pipefd[READ_END]); // close read end of pipe

    free(childPIDs);

    // print the result
    printf("Total sum: %d\n", totalSum);

    return 0;
}
