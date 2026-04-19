#include <time.h>

#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

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





int get_last_report_id(int fd){
    Report temp;
    int lastID = 0;

    lseek(fd, 0, SEEK_SET);

    while (read(fd, &temp, sizeof(Report)) == sizeof(Report)) {
        if (temp.reportID > lastID)
            lastID = temp.reportID;
    }

    return lastID;
}

// both manager and inspector can use
void add(char *district_id, char *inspector_name){

    struct stat st;
    char path[256];

  
    // Checks if dir/district exists
    if (stat(district_id, &st) == 0) {

        // Check dir permissions
        if ((st.st_mode & 0777) != 0750) {
            printf("[ERROR] Invalid permissions on district directory\n");
            return;
        }

        //if dir exists we continue with that one, ig not we create one
    }
    else {
        if (mkdir(district_id, 0750) == -1) {
            perror("[ERROR] mkdir failed");
            return;
        }
    }

    /// REPORT
    snprintf(path, sizeof(path), "%s/reports.dat", district_id);

    int fd = open(path, O_CREAT | O_RDWR, 0664);
    if (fd == -1) {
        perror("[ERROR] reports.dat creation failed");
        return;
    }
    
    

    chmod(path, 0664);

    // Check permissions
    if (stat(path, &st) == 0) {
        if ((st.st_mode & 0666) != 0664) {
            printf("[ERROR] reports.dat has invalid permissions\n");
            return;
        }
    }


    Report r;
    int lastID = get_last_report_id(fd); 
    r.reportID = lastID + 1;

    strcpy(r.inspector_name, inspector_name);
    r.timestamp = time(NULL);

    printf("Enter latitude: ");
    scanf("%lf", &r.GPS_lat);

    printf("Enter longitude: ");
    scanf("%lf", &r.GPS_long);

    printf("Enter issue (road/lighting/etc): ");
    scanf("%s", r.issue);

    printf("Enter severity (1-3): ");
    scanf("%d", &r.severity);
    getchar(); // consume newline

    printf("Enter description: ");
    fgets(r.desc, sizeof(r.desc), stdin);
    r.desc[strcspn(r.desc, "\n")] = 0;

    //find end
    lseek(fd, 0, SEEK_END);

    if (write(fd, &r, sizeof(Report)) != sizeof(Report)) {
        perror("[ERROR] write failed");
        close(fd);
        return;
    }

    close(fd);
    printf("[INFO] Report added successfully with ID %d\n", r.reportID);
    
}




\

void list(char *district_id)
{
    struct stat st;
    char path[256];

    snprintf(path, sizeof(path), "%s/reports.dat", district_id);

    if (stat(path, &st) == -1) {
        perror("[ERROR] stat failed");
        return;
    }

    // Perimssions
    char perm[10];

    perm[0] = (st.st_mode & S_IRUSR) ? 'r' : '-';
    perm[1] = (st.st_mode & S_IWUSR) ? 'w' : '-';
    perm[2] = (st.st_mode & S_IXUSR) ? 'x' : '-';

    perm[3] = (st.st_mode & S_IRGRP) ? 'r' : '-';
    perm[4] = (st.st_mode & S_IWGRP) ? 'w' : '-';
    perm[5] = (st.st_mode & S_IXGRP) ? 'x' : '-';

    perm[6] = (st.st_mode & S_IROTH) ? 'r' : '-';
    perm[7] = (st.st_mode & S_IWOTH) ? 'w' : '-';
    perm[8] = (st.st_mode & S_IXOTH) ? 'x' : '-';

    perm[9] = '\0';



    printf("\n=== reports.dat INFO ===\n");
    printf("Permissions : %s\n", perm);
    printf("Size        : %ld bytes\n", st.st_size);
    printf("Last modified: %s", ctime(&st.st_mtime));

    // Open read
    int fd = open(path, O_RDONLY);
    if (fd == -1) {
        perror("[ERROR] cannot open reports.dat");
        return;
    }

    Report r;

    printf("\n=== REPORTS ===\n");

    while (read(fd, &r, sizeof(Report)) == sizeof(Report)) {
        printf("\n");
        printf("ID: %d\n", r.reportID);
        printf("Inspector: %s\n", r.inspector_name);
        printf("Lat: %.6f | Long: %.6f\n", r.GPS_lat, r.GPS_long);
        printf("Issue: %s\n", r.issue);
        printf("Severity: %d\n", r.severity);
        printf("Time: %s", ctime(&r.timestamp));
        printf("Desc: %s\n", r.desc);
    }

    close(fd);
}

int main(int argc, char* argv[]){

    char* role=argv[2]; //manager or inspector
    char* user=argv[4]; //name
    char* command=argv[5]; //--add or --list etc

    char *args[50];
    int arg_count = 0;

    for (int i = 6; i < argc; i++) {
        args[arg_count++] = argv[i];
    }

    //I think I can do it with an aux funtion + case but for now it works.
    if (strcmp(command, "--add") == 0) {
        add(args[0],user);
    }
    else if (strcmp(command, "--list") == 0) {
        list(args[0]);
    }
    else if (strcmp(command, "--view") == 0) {
        printf("Remove report command\n");
    }
    else if (strcmp(command, "--remove_report") == 0) {
        printf("Remove report command\n");
    }
    else if (strcmp(command, "--update_threshold") == 0) {
        printf("Remove report command\n");
    }
    else if (strcmp(command, "--filter") == 0) {
        printf("Filter command\n");
    }
    else {
        printf("Unknown command\n");
    }

   
    return 0;

}

