#define _POSIX_C_SOURCE 200809L

#include <stdio.h>
#include <signal.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdlib.h> //for exit()

// SIGINT = Interrupt - Interrupt from keyboard (CTRL+C).
//kill -INT $(cat .monitor_pid)   -sends SIGINT if running in the backround


void handler(int sig) {
    if (sig == SIGINT) {
        char msg[50]= "LOG: Received signal SIGINT.\n";
        write(STDOUT_FILENO,msg,strlen(msg));
        unlink(".monitor_pid");
        exit(0);
    }
    else if (sig == SIGUSR1){
        char msg[50]= "LOG: Report added.\n";
        write(STDOUT_FILENO, msg, strlen(msg));
    }
    
}

int main(){
    //Phase 3, check if monitor running
    int fd_check = open(".monitor_pid", O_RDONLY);
    if (fd_check != -1) {
        char pid_buf[32];
        int n = read(fd_check, pid_buf, sizeof(pid_buf) - 1);
        close(fd_check);
        
        if (n > 0) {
            pid_buf[n] = '\0';
            int existing_pid = atoi(pid_buf);
            
            //check if porcess exists
            if (existing_pid > 0 && kill(existing_pid, 0) == 0) {
                char err_msg[64];
                int len = snprintf(err_msg, sizeof(err_msg), "ERROR: Monitor already running with PID: %d\n", existing_pid);
                write(STDOUT_FILENO, err_msg, len);
                return 1; 
            }
        }
    }

    struct sigaction sa;

    sa.sa_handler = handler;
    sa.sa_flags = 0;
    sigemptyset(&sa.sa_mask);
    sigaction(SIGINT, &sa, NULL);
    sigaction(SIGUSR1, &sa, NULL);

    int fd = open(".monitor_pid", O_WRONLY | O_CREAT | O_TRUNC, 0644);
    if (fd == -1) {
        perror("ERROR: open hidden file");
        return 1;
    }

    char buffer[32];
    int len = snprintf(buffer, sizeof(buffer), "%d\n", getpid());
    write(fd, buffer, len);
    close(fd);

    char msg[64]= "LOG: Monitor running. Press Ctrl+C to stop.\n";
    write(STDOUT_FILENO, msg, strlen(msg));

    // Wait until signal
    while (1) {
        pause();
    }

    return 0;
}