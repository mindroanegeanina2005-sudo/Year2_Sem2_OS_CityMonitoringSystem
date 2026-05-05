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
        write(STDOUT_FILENO,"Received signal SIGINT.\n",30);
        unlink(".monitor_pid");
        exit(0);
    }
    else if (sig == SIGUSR1){
        char msg[50]= "\nReport added.\n";
        write(STDOUT_FILENO, msg, strlen(msg));
    }
    
}

int main(){
    struct sigaction sa;

    sa.sa_handler = handler;
    sa.sa_flags = 0;
    sigemptyset(&sa.sa_mask);
    sigaction(SIGINT, &sa, NULL);
    sigaction(SIGUSR1, &sa, NULL);

    int fd = open(".monitor_pid", O_WRONLY | O_CREAT | O_TRUNC, 0644);
    if (fd == -1) {
        perror("[ERROR] open hidden file");
        return 1;
    }

    char buffer[32];
    int len = snprintf(buffer, sizeof(buffer), "%d\n", getpid());
    write(fd, buffer, len);
    close(fd);

    write(STDOUT_FILENO, "Monitor running. Press Ctrl+C to stop.\n", 40);

    // Wait until signal
    while (1) {
        pause();
    }

    return 0;
}