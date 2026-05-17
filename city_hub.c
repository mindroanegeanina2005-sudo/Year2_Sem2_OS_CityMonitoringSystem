#define _POSIX_C_SOURCE 200809L
#include <sys/wait.h>
#include <stdio.h>
#include <signal.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdlib.h>

void handle_start_monitor() {
    int hub_mon_pid = fork();

    if (hub_mon_pid < 0) {
        perror("Fork failed");
        return;
    }

    if (hub_mon_pid == 0) {
        // inside hub_mon

        //Ignore SIGINT
        struct sigaction sa_ignore;
        sa_ignore.sa_handler = SIG_IGN;
        sa_ignore.sa_flags = 0;
        sigemptyset(&sa_ignore.sa_mask);
        sigaction(SIGINT, &sa_ignore, NULL);

        int pipefd[2];
        if (pipe(pipefd) == -1) {
            perror("Pipe generation failed");
            exit(1);
        }

        int monitor_pid = fork();
        if (monitor_pid < 0) {
            perror("Fork failed");
            exit(1);
        }

        if (monitor_pid == 0) {
            //hub_mon child
            
            //restoreh the SIGINT
            struct sigaction sa_default;
            sa_default.sa_handler = SIG_DFL;
            sa_default.sa_flags = 0;
            sigemptyset(&sa_default.sa_mask);
            sigaction(SIGINT, &sa_default, NULL);

            close(pipefd[0]); // Close unused read end

            // stdout of monitor to the pipe's write end
            dup2(pipefd[1], STDOUT_FILENO);
            close(pipefd[1]);

            // Execute monitor
            char *args[] = {"./monitor", NULL};
            execv(args[0], args);
            
            // If monitor fails
            perror("Execv failed");
            exit(1);
        }

       //back parent
        close(pipefd[1]); // Close unused write ends

        // ewad pipe
        FILE *pipe_stream = fdopen(pipefd[0], "r");
        if (pipe_stream == NULL) {
            close(pipefd[0]);
            exit(1);
        }

        char buffer[256];
        int error_detected = 0;

        // until EOF
        while (fgets(buffer, sizeof(buffer), pipe_stream) != NULL) {
            if (strncmp(buffer, "ERROR:", 6) == 0) {
                printf("\n[HUB_MON: ALERT] %s", buffer + 6);
                error_detected = 1;
            } else if (strncmp(buffer, "LOG:", 5) == 0) {
                printf("\n[HUB_MON: LOG] %s", buffer + 5);
            } else {
                printf("\n[HUB_MON] %s", buffer);
            }
        }

        fclose(pipe_stream);

        // Wait
        int status;
        waitpid(monitor_pid, &status, 0);

        if (error_detected) {
            printf("\n[HUB_MON: ERROR] Monitor was already active elsewhere.\n");
        } else {
            printf("\n[HUB_MON: NOTICE] Monitor child process has terminated.\n");
        }

        exit(0); // Exit hub_mon
    }

    printf("[Hub] Launched 'hub_mon=(PID: %d).\n", hub_mon_pid);
}


int main() {
    char input[512];

    printf("\n=== MENU ===\n");
    printf("Commands: 1.start_monitor\n 2.calculate_scores\n 3.exit\n");

    while (1) {
        printf("city_hub> ");

        if (fgets(input, sizeof(input), stdin) == NULL) {
            break;
        }
        //new line from input to remove
        input[strcspn(input, "\n")] = '\0';

        char *command = strtok(input, " ");
        if (!command) {
            continue;
        }

        // Menu
        if (strcmp(command, "start") == 0) {
            handle_start_monitor();
        } 
        else if (strcmp(command, "calculate") == 0) {
            printf("[WIP] calculate_scores.\n");
        }
           
        else if (strcmp(command, "exit") == 0) {
            printf("Exit city_hub.\n");
            break;
        } 
        else {
            printf("[Error]: Command '%s' not recognized.\n", command);
        }
        
        printf("\n");
    }

    return 0;
}