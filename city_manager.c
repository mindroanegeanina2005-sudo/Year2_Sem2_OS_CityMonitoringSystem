#include <time.h>
#include <stdio.h>
#include <string.h>

typedef struct Report{
    int reportID;
    char inspector_name[50];
    double GPS_lat;
    double GPS_long;
    char issue[100];
    int severity;
    time_t timestamp;
    char desc[200];
}Report;

int main(int argc, char* argv[]){

    char* role=argv[2]; //manager or inspector
    char* user=argv[4]; //name
    char* command=argv[5]; //--add or --list etc

    char args[256] = ""; //after command

    for (int i = 6; i < argc; i++) {
        strcat(args, argv[i]);
        if (i < argc - 1)
            strcat(args, " ");
    }



    printf("ROLE: %s\n", role);
    printf("USER: %s\n", user);
    printf("COMMAND: %s\n", command);
    printf("ARGS: %s\n", args);
    return 0;

}

git config --global user.email "mindroane.geanina2005@gmail.com"
git config --global user.name "Geanina"