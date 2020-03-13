#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <unistd.h>
#include <time.h>
#include <dirent.h>
#include <errno.h>

#define RECOVERY_SYSTEM "/system/recovery"
#define LOG_FOLDER "/data/local/tmp/twrp_launcher_logging"
#define LAST_LOG "/data/local/tmp/twrp_launcher_logging/last_log"
#define SELINUX "/sys/fs/selinux/enforce"
#define BUSYBOX "/twrp_tmp/busybox"
#define TWRP "/sbin/recovery"

char *prio = NULL;

void selinux(int mode) {
      FILE *fptr;
      fptr = fopen(SELINUX,"w");
      fprintf(fptr,"%d",mode);
      fflush(fptr);
      fclose(fptr);
}

char* concat(const char *s1, const char *s2, const char *s3, const char *s4)
{
    char *result = malloc(strlen(s1) + strlen(s2) + strlen(s3) + strlen(s4) + 1);
    strcpy(result, s1);
    strcat(result, s2);
    strcat(result, s3);
    strcat(result, s4);
    return result;
}

void clear_log() {
    remove(LAST_LOG);
}

void mountrw() {
    system("mount -o remount,rw /");
    system("mount -o remount,rw /system");
}

int write_to_log(int fmt, char *msg, ...) {
    /*
     fmt = 1 --> I
     fmt = 2 --> W
     fmt = 3 --> E
    */

    time_t t = time(NULL);
    struct tm tm = *localtime(&t);

    if (access(LAST_LOG, F_OK) != -1) {}
    else {
         mkdir(LOG_FOLDER, 0700);
    }

    FILE* file = fopen(LAST_LOG,"a+");

    if (fmt == 1) {
        prio = "I";
    }
    if (fmt == 2) {
        prio = "W";
    }
    if (fmt == 3) {
        prio = "E";
    }
    if (prio == NULL) {
        prio = "U";
    }

    if (file != NULL)
    {
        fprintf(file, "%d-%02d-%02d %02d:%02d:%02d     %s: %s\n", tm.tm_year + 1900, tm.tm_mon + 1, tm.tm_mday, tm.tm_hour, tm.tm_min, tm.tm_sec, prio, msg);
        fflush(file);
        fclose(file);
    }
    return 0;
}

void del_if_ex(char *folder) {
    char* cmd = concat(BUSYBOX, " rm ", "-rf ", folder);
    //printf("%s", cmd);
    DIR* dir = opendir(RECOVERY_SYSTEM);
    if (dir) {
        closedir(dir);
        system(cmd);
    }
}

void copy(char *orig, char *dest, int bbx) {
    if (bbx == 1) {
      char* cmd = concat("/twrp_tmp/busybox cp -r ", orig, " ", dest);
      //printf("CMD IS: %s\n", cmd);
      system(cmd);
    }
    else if (bbx == 0) {
      char* cmd = concat("/system/bin/cp -r ", orig, " ", dest);
      //printf("CMD IS: %s\n", cmd);
      system(cmd);
    }
}

void check_root() {
    FILE *fp;
    char id[1035];

    fp = popen("id", "r");

    while (fgets(id, sizeof(id), fp) != NULL) {}

    pclose(fp);

    if(strstr(id, "root") == NULL) {
        printf("Cannot run without root shell: %s!\n", id);
        write_to_log(3, "Cannot run without root shell: %s\n", id);
        exit(1);
    }
}

int main(int argc, char *argv[])
{
          clear_log();
          check_root();

          selinux(0);

          printf("lol\n");

          write_to_log(1, "Starting the TWRP Launcher...");
          mountrw();

          write_to_log(1, "Checking files...");

          DIR* dir = opendir(RECOVERY_SYSTEM);
          if (dir) {
             /* Directory exists. */
             closedir(dir);
          } else if (ENOENT == errno) {
             write_to_log(3, "Cannot locate recovery folder in /system");
             exit(1);
          }

          write_to_log(3, "Preparing ramdisk...");

          write_to_log(2, "Preparing busybox...");
          del_if_ex("/twrp_tmp");
          mkdir("/twrp_tmp", 0755);
          copy("/system/recovery/busybox", "/twrp_tmp/", 0);
          system("chmod -R 775 /twrp_tmp/busybox");

          write_to_log(2, "Copying sbin folder...");
          del_if_ex("/sbin");
          mkdir("/sbin", 0755);
          copy("/system/recovery/sbin/*", "/sbin/", 0);
          system("/twrp_tmp/busybox chmod -R 755 /sbin/*");

          write_to_log(2, "Copying etc folder...");
          del_if_ex("/etc");
          mkdir("/etc", 0755);
          copy("/system/recovery/etc/*", "/etc/", 1);

          write_to_log(2, "Copying license folder...");
          del_if_ex("/license");
          mkdir("/license", 0755);
          copy("/system/recovery/license/*", "/license/", 1);

          write_to_log(2, "Copying res folder...");
          del_if_ex("/res");
          mkdir("/res", 0755);
          copy("/system/recovery/res/*", "/res/", 1);

          write_to_log(2, "Copying twres folder...");
          del_if_ex("/twres");
          mkdir("/twres", 0755);
          copy("/system/recovery/twres/*", "/twres/", 1);

          write_to_log(2, "Copying root files...");
          copy("/system/recovery/rootdir/*", "/", 1);

          write_to_log(2, "Stoping main services...");
          printf("Stoping main services...\n");
          system("/twrp_tmp/busybox killall -9 system_server");
          //busybox("killall", "-9", "zygote");
          system("/twrp_tmp/busybox killall -9 vold");

          write_to_log(2, "Starting TWRP...");
          printf("Starting TWRP...\n");
          execl("/sbin/recovery", "", NULL);

          return 0;
}
