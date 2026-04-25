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


///NTOE TO ADD: only managers can create distrincts??

/// --------------
/// HELPERS
/// --------------
void get_permissions_string(mode_t mode, char *str) {
    // Owner
    str[0] = (mode & S_IRUSR) ? 'r' : '-';
    str[1] = (mode & S_IWUSR) ? 'w' : '-';
    str[2] = (mode & S_IXUSR) ? 'x' : '-';
    // Group
    str[3] = (mode & S_IRGRP) ? 'r' : '-';
    str[4] = (mode & S_IWGRP) ? 'w' : '-';
    str[5] = (mode & S_IXGRP) ? 'x' : '-';
    // Others
    str[6] = (mode & S_IROTH) ? 'r' : '-';
    str[7] = (mode & S_IWOTH) ? 'w' : '-';
    str[8] = (mode & S_IXOTH) ? 'x' : '-';
    
    str[9] = '\0';
}


/// --------------
/// ADDING REPORTS 
/// --------------
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


// both manager and inspector can use add
void add(char *district_id, char *name, char *role){

    struct stat st;
    char path[256]; //for report path
    char txt[256]; //for logged path
    char conf[256]; //for configuration path

    // Checks if dir/district exists
    if (stat(district_id, &st) == 0) {

        // Check dir permissions
        if ((st.st_mode & 0777) != (S_IRWXU | S_IRGRP | S_IXGRP)){
            printf("[ERROR] Invalid permissions on district directory\n");
            return;
        }

        // If dir exists we continue with that one, ig not we create one
    }
    else {
        // Dir does not exists -> create
        if (strcmp(role, "manager") == 0) {
            // creates dir
            if (mkdir(district_id, 0750) == -1) {
                perror("[ERROR] mkdir failed");
                return;
            }
            chmod(district_id, 0750);
        }else{
            printf("[ERROR] Only managers can create new districts.\n");
            return;
        }

    }

    /// REPORT
     //------------------
    snprintf(path, sizeof(path), "%s/reports.dat", district_id);

    int fd = open(path, O_CREAT | O_RDWR, 0664);
    if (fd == -1) {
        perror("[ERROR] reports.dat creation failed");
        return;
    }
    chmod(path, 0664);

    // Check permissions
    if (stat(path, &st) == 0) {
        if ((st.st_mode & 0777) != (S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH)) {
            printf("[ERROR] reports.dat has invalid permissions\n");
            close(fd);
            return;
        }
    }

    Report r;
    int lastID = get_last_report_id(fd); 
    r.reportID = lastID + 1;

    strcpy(r.inspector_name, name);
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

    ///LOGGED TEXT FILE
    //------------------
    snprintf(txt, sizeof(txt), "%s/logged_district", district_id);
    int ft = open(txt, O_CREAT | O_WRONLY | O_APPEND, 0644);
    if (ft != -1) {
        chmod(txt, 0644); 
        
        if (fstat(ft, &st) == 0 && (st.st_mode & 0777) == (S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH)) {
           
            char log[256];
            char *time_str = ctime(&r.timestamp);
            time_str[24] = '\0';

            int log_len = snprintf(log, sizeof(log), "-> %s %s %s add\n", time_str, name, role);
            write(ft, log, log_len);
        }
        close(ft);
    }
    /// CONFIGURATION 
    //------------------
    snprintf(conf, sizeof(conf), "%s/district.cfg", district_id);

    int fco = open(conf, O_CREAT | O_RDWR | O_EXCL, 0640);
    // Checks if file is new or not
    if (fco == -1) {

        close(fco);
    }else {
        chmod(conf, 0640);
        
        char *threshold = "3\n";
        if (write(fco, threshold, strlen(threshold)) == -1) {
            perror("[ERROR] Failed to write default threshold");
        }
        close(fco);
    }   

    // Check permissions
    if (stat(conf, &st) == 0) {
        if ((st.st_mode & 0777) != (S_IRUSR | S_IWUSR | S_IRGRP)) {
            printf("[ERROR] district.cfg has invalid permissions\n");
            return;
        }
    }

}

/// --------------
/// LISTING
/// --------------

void list(char *district_id, char *name, char *role){
    struct stat st;
    char path[256];

    snprintf(path, sizeof(path), "%s/reports.dat", district_id);

    if (stat(path, &st) == -1) {
        perror("[ERROR] stat failed");
        return;
    }

    // Perimssions
    char perm[10];
    get_permissions_string(st.st_mode, perm);

    printf("\n=== reports.dat INFO ===\n");
    printf("Permissions  : %s\n", perm);
    printf("Size         : %ld bytes\n", st.st_size);
    printf("Last modified: %s", ctime(&st.st_mtime)); //takes time from stat
  
    // Open read
    int fd = open(path, O_RDONLY);
    if (fd == -1) {
        perror("[ERROR] cannot open reports.dat");
        return;
    }

    Report r;
    
    printf("\n=== REPORTS ===\n");

    // Writing a report
    while (read(fd, &r, sizeof(Report)) == sizeof(Report)) {
        printf("\n");
        printf("ID              : %d\n", r.reportID);
        printf("Inspector       : %s\n", r.inspector_name);
        printf("Lat: %.3f     |Long: %.3f\n", r.GPS_lat, r.GPS_long);
        printf("Issue           : %s\n", r.issue);
        printf("Severity        : %d\n", r.severity);
        printf("Time            : %s", ctime(&r.timestamp));
        printf("Desc            : %s\n", r.desc);
    }

    close(fd);

    //WRITING LOG
    char txt[256];
    snprintf(txt, sizeof(txt), "%s/logged_district", district_id);
    int ft = open(txt, O_WRONLY | O_APPEND);
    if (ft == -1) {
        perror("[ERROR] logged_district adding log failed");
        return;
    }
    char log[256];
    time_t timestamp = time(NULL);
    char *time_str = ctime(&timestamp);
    time_str[24] = '\0';

    int log_len=snprintf(log, sizeof(log), "-> %s %s %s list\n", time_str, name, role);

    if (write(ft, log, log_len) != log_len) {
        perror("[ERROR] write failed on logs");
        close(ft);
        return;
    }

    close(ft);

}

int main(int argc, char* argv[]){

    if (argc < 6) {
        fprintf(stderr, "Error: Missing required arguments.\n");
        return 1;
    }

    // forse the fhe command order.
    if (strcmp(argv[1], "--role") != 0 || strcmp(argv[3], "--user") != 0) {
        fprintf(stderr, "Usage: %s --role [role] --user [user] --[command] [args...]\n", argv[0]);
        return 1;
    }


    char* role=argv[2]; //manager or inspector
    char* user=argv[4]; //name
    char* command=argv[5]; //--add or --list etc

    char *args[50];
    int arg_count = 0;

    for (int i = 6; i < argc; i++) {
        args[arg_count++] = argv[i];
    }

    // I think I can do it with an aux funtion + case but for now it works.
    if (strcmp(command, "--add") == 0) {
        add(args[0], user, role);
    }
    else if (strcmp(command, "--list") == 0) {
        list(args[0], user, role);
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

