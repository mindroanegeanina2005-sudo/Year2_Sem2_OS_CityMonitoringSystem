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

// Writing a report.
void print_report_details(Report *r) {
    printf("\nID              : %d\n", r->reportID);
    printf("Inspector       : %s\n", r->inspector_name);
    printf("Lat: %.3f     |Long: %.3f\n", r->GPS_lat, r->GPS_long);
    printf("Issue           : %s\n", r->issue);
    printf("Severity        : %d\n", r->severity);
    printf("Time            : %s", ctime(&r->timestamp));
    printf("Desc            : %s\n", r->desc);
    printf("-------------------------------\n");
}

// Writing a log -> since i have to use it for all actions
void write_log(char *district_id,char *name, char *role, char *text){
    char txt[256];
    struct stat st;
    snprintf(txt, sizeof(txt), "%s/logged_district", district_id);


    int ft = open(txt, O_WRONLY | O_APPEND);
    if (ft == -1) {
        perror("[ERROR] open log failed");
        return;
    }

    if (stat(txt, &st) == -1) {
        perror("[ERROR] stat failed");
        close(ft);
        return;
    }

    // Check permissions
    if (strcmp(role, "manager") == 0) {
        //if write or execute, owner
         if (!(st.st_mode & S_IRUSR) || !(st.st_mode & S_IWUSR)) {
            printf("[ERROR] Manager cannot read/write logs\n");
            close(ft);
            return;
        }
    } else {
        // close(ft);
        // return;
        if (!(st.st_mode & S_IRGRP)) {
            printf("[ERROR] Inspector cannot read logs\n");
            close(ft);
            return;
        }
    }

    time_t t = time(NULL);
    char *time_str = ctime(&t);
    time_str[24] = '\0';

    char log[256];
    int len = snprintf(log, sizeof(log), "%s %s %s %s\n",
                                    time_str, name, role, text);

    write(ft, log, len);
    close(ft);
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
        if (strcmp(role, "manager") == 0) {
            //if write or execute, owner
            if (!(st.st_mode & S_IRUSR) || !(st.st_mode & S_IWUSR) || !(st.st_mode & S_IXUSR)) {
                printf("[ERROR] Manager cannot use district directory\n");
                return;
            }
        } else {
            if (!(st.st_mode & S_IRGRP) || !(st.st_mode & S_IXGRP)) {
                printf("[ERROR] Inspector cannot use district directory\n");
                return;
            }
        }
        // If dir exists we continue with that one, ig not we create one
    }
    else {
        // Dir does not exists -> create
        if (strcmp(role, "manager") != 0) {
            printf("[ERROR] Only managers can create new districts.\n");
            return;
        }
        // creates dir
        if (mkdir(district_id, 0750) == -1) {
            perror("[ERROR] mkdir failed");
            return;
        }
        chmod(district_id, 0750);
       
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
    // Checks only write.
    if (stat(path, &st) == 0) {
    if (strcmp(role, "manager") == 0) {
        if (!(st.st_mode & S_IWUSR)) {
            printf("[ERROR] Manager cannot write reports.dat\n");
            close(fd);
            return;
        }
    } else {
        if (!(st.st_mode & S_IWGRP)) {
            printf("[ERROR] Inspector cannot write reports.dat\n");
            close(fd);
            return;
        }
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
    if (ft == -1) {
        perror("[ERROR] log file creation failed");
        return;
    }
    close(ft);

    chmod(txt, 0644);

    // used funtion to write the report
    write_log(district_id, name, role, "add");

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
        if (strcmp(role, "manager") == 0) {
            if (!(st.st_mode & S_IRUSR) || !(st.st_mode & S_IWUSR)) {
                printf("[ERROR] Manager cannot read and write district.cfg\n");
                return;
            }
        } else {
            if (!(st.st_mode & S_IRGRP)) {
                printf("[ERROR] Inspector cannot read district.cfg\n");
                return;
            }
        }
    }

}

/// --------------
/// LISTING + VIEW
/// --------------

void list(char *district_id, char *name, char *role){
    struct stat st;
    char path[256];

    snprintf(path, sizeof(path), "%s/reports.dat", district_id);

    // Check if file exists
    if (stat(path, &st) == -1) {
        perror("[ERROR] reports.dat does not exist");
        return;
    }
    // Check permissions
    if ((st.st_mode & 0777) != (S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH)) {
        perror("[ERROR] reports.dat has invalid permissions");
        return;
    }

    // get info
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
        print_report_details(&r); // this just prints a report
    }

    close(fd);

    //WRITING LOG
    write_log(district_id, name, role, "list");

}

void view(char *district_id, char* name, char *role, char* report_id_char){
    struct stat st;
    char path[256];

    snprintf(path, sizeof(path), "%s/reports.dat", district_id);

    // Check if file exists
    if (stat(path, &st) == -1) {
        perror("[ERROR] reports.dat does not exist");
        return;
    }
    // Check permissions
    if ((st.st_mode & 0777) != (S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH)) {
        perror("[ERROR] reports.dat has invalid permissions");
        return;
    }


    int fd = open(path, O_RDONLY);
    if (fd == -1) {
        perror("[ERROR] cannot open reports.dat");
        return;
    }

    Report r;

    int report_id=atoi(report_id_char);
    int offset = (report_id-1) * sizeof(Report);

    // Jump to possition
    if (lseek(fd, offset, SEEK_SET) == -1) {
        perror("[ERROR] Seek failed");
        close(fd);
        return;
    }
    if (read(fd, &r, sizeof(Report)) == sizeof(Report)){
        printf("\n=== Report View %d ===\n",report_id);
        print_report_details(&r);
    }else{
        printf("Did not find report at possition\n");
    }

    close(fd);

    //WRITING LOG
    write_log(district_id, name, role, "view");

}

/// --------------
/// Update Threshold
/// --------------

void update_threshold(char *district_id, char *name, char *role, char* value_char){
    struct stat st;
    char path[256];

    snprintf(path, sizeof(path), "%s/district.cfg", district_id);

    if (strcmp(role, "manager") != 0) {
        printf("Manager role required to update thresholds.\n");
        return;
    }

    if (stat(path, &st) == -1) {
        perror("Error accessing district.cfg");
        return;
    }
    if (!(st.st_mode & S_IRUSR) || !(st.st_mode & S_IWUSR)) {
        printf("[ERROR] Manager does not have read/write access to district.cfg\n");
        return;
    }

    int fd = open(path, O_WRONLY | O_TRUNC);
    if (fd == -1) {
        perror("[ERROR] Cannot open district.cfg");
        return;
    }

    char buffer[64];
    int threshold = atoi(value_char);
    snprintf(buffer, sizeof(buffer), "%d\n", threshold);

    if (write(fd, buffer, strlen(buffer)) == -1) {
        perror("[ERROR] Failed to write threshold");
        close(fd);
        return;
    }

    close(fd);

    write_log(district_id, name, role, "update threshold");

}

/// --------------
/// REMOVE REPORTS
/// --------------

void remove_report(char *district_id, char* name, char *role, char* report_id_char){
    struct stat st;
    char path[256];

    snprintf(path, sizeof(path), "%s/reports.dat", district_id);

    if (strcmp(role, "manager") != 0) {
        printf("Manager role required to update thresholds.\n");
        return;
    }

    if (stat(path, &st) == -1) {
        perror("[ERROR] stat failed");
        return;
    }

    if (!(st.st_mode & S_IRUSR) || !(st.st_mode & S_IWUSR)) {
        printf("[ERROR]Manager does not have read/write permissions on");
        return;
    }

    // after checking perimssions and such we open the file
    int fd = open(path, O_RDWR);
    if (fd == -1) {
        perror("[ERROR] Failed to open reports.dat");
        return;
    }

    int report_id = atoi(report_id_char);
    int total_records = st.st_size / sizeof(Report);

    //early check if the record exists
    if (report_id <= 0 || report_id > total_records) {
        printf("[INFO] Invalid report ID\n");
        close(fd);
        return;
    }

    // Jump
    int offset = (report_id - 1) * sizeof(Report);
    if (lseek(fd, offset, SEEK_SET) == -1) {
        perror("[ERROR] seek failed");
        close(fd);
        return;
    }


    // Shift
    for (int i = report_id - 1; i < total_records - 1; i++) {

        Report next;

        if (lseek(fd, (i + 1) * sizeof(Report), SEEK_SET) == -1) {
            perror("[ERROR] seek failed");
            close(fd);
            return;
        }

        if (read(fd, &next, sizeof(Report)) != sizeof(Report)) {
            perror("[ERROR] read failed");
            close(fd);
            return;
        }

        //fix ID for the new position
        next.reportID = i + 1;

        if (lseek(fd, i * sizeof(Report), SEEK_SET) == -1) {
            perror("[ERROR] seek failed");
            close(fd);
            return;
        }

        if (write(fd, &next, sizeof(Report)) != sizeof(Report)) {
            perror("[ERROR] write failed");
            close(fd);
            return;
        }
    }


    if (ftruncate(fd, (total_records - 1) * sizeof(Report)) == -1) {
        perror("[ERROR] ftruncate failed");
    }

    close(fd);
}

int main(int argc, char* argv[]){

    if (argc < 6) {
        fprintf(stderr, "Error: Missing required arguments.\n");
        return 1;
    }

    // forse the fhe command order.
    if (strcmp(argv[1], "--role") != 0 || strcmp(argv[3], "--user") != 0) {
        fprintf(stderr, "Usage: %s --role [role] --user [user] --[command] ...\n", argv[0]);
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
        view(args[0],user,role,args[1]);
    }
    else if (strcmp(command, "--remove_report") == 0) {
        remove_report(args[0],user,role,args[1]);
    }
    else if (strcmp(command, "--update_threshold") == 0) {
        update_threshold(args[0],user,role,args[1]);
    }
    else if (strcmp(command, "--filter") == 0) {
        printf("Filter command\n");
    }
    else {
        printf("Unknown command\n");
    }

   
    return 0;

}

