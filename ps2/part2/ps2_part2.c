//Ahmad Malik
//ECE357  Fall 2022
//Probelm Set 2
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>
#include <string.h>
#include <sys/types.h>
#include <dirent.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <pwd.h>
#include <grp.h>
#include <errno.h>
#include <stdbool.h>
#include <ctype.h>

int printNode(char*path, struct stat Inode, bool userflag, uid_t userID, bool mtimeflag, int mtimeVal);
int walkPath(char *path, bool userflag,  uid_t userID, bool mtimeflag, int mtimeVal);

int main(int argc, char * argv[]) {

    bool userflag = false;
    bool mtimeflag = false;
    int mtimeVal = 0;
    uid_t userID = 0;

    int option;
	while((option = getopt(argc, argv, "u:m:")) != -1){
        if(option == 'u'){
			userflag = true;
            if(isdigit(*optarg)){
                userID = atoi(optarg);
            }
            else{
                struct passwd *getpwuid = getpwnam(optarg);
                if (getpwuid == NULL){
                    errno = 0;
                    fprintf(stderr, "INVALID USERNAME: %s\n", strerror(errno));
                    return -1;
                }
                userID = getpwuid->pw_uid;
            }
        }
		else if(option == 'm'){
            mtimeflag = true;
				mtimeVal = atoi(optarg);
		}
        else{
            printf("%s\n", optopt);
		}
	}
    char *path = argv[argc - 1];
	printf("\n\n\t Inode#   Block#    Permissions   Link#    UserName    GroupName    Size(bytes)      Date     Time      Path   \n");
	printf("\t________|________|______________|_______|____________|___________|______________|___________|________|______________________________\n");
    walkPath(path, userflag, userID, mtimeflag, mtimeVal);
    return 0;
}

int walkPath(char *path, bool userflag,  uid_t userID, bool mtimeflag, int mtimeVal){
	struct stat Inode;
	if (lstat(path,&Inode) == -1){
		printf("Unable to Access File  %s\n", strerror(errno));
		return -1;
	}
    if(Inode.st_uid == userID && userflag == true){
		printNode(path, Inode, userflag, userID, mtimeflag,mtimeVal);
	}
    if(userflag == false){
		printNode(path, Inode, userflag, userID, mtimeflag,mtimeVal);
	}
	if ((Inode.st_mode & S_IFMT) != S_IFDIR){
		return 0;
	}
	DIR *dir = opendir(path);
	if (dir == NULL){
		fprintf(stderr, "Unable to Open directory: %s \n", path, strerror(errno));
		return -1;
	}
	struct dirent *directory;
	while((directory = readdir(dir)) != NULL){
		char newPath[1024];
		if ((strcmp(directory ->d_name, "..") != 0) && (strcmp(directory ->d_name, ".") != 0)){
			strcpy(newPath, path);
			strcat(newPath, "/");
			strcat(newPath, directory->d_name);
			walkPath(newPath, userflag, userID, mtimeflag,mtimeVal);
		}
	}
	if (closedir(dir) == -1){
		fprintf(stderr, "Unable to Close directory: %s", strerror(errno));
		return -1;
	}
	return 0;
}

int printNode(char*path, struct stat Inode, bool userflag, uid_t userID, bool mtimeflag, int mtimeVal){
	uid_t uid = Inode.st_uid;
	gid_t gid = Inode.st_gid;
    struct passwd *pwd = getpwuid(uid);
	struct group *gpd = getgrgid(gid);
    int buf = 1024;
	char str[11];
	char userName[buf];
    char groupName[buf];
	strncpy(userName, pwd->pw_name, buf);
	strncpy(groupName, gpd->gr_name, buf);
    strcpy(str, "----------" );

    if (S_ISREG(Inode.st_mode))  str[0] = '-';
    if (S_ISDIR(Inode.st_mode))  str[0] = 'd';
    if (S_ISCHR(Inode.st_mode))  str[0] = 'c';
    if (S_ISBLK(Inode.st_mode))  str[0] = 'b';
    if (S_ISLNK(Inode.st_mode))  str[0] = 'l';
    if (S_ISFIFO(Inode.st_mode))  str[0] = 'p';
    if (S_ISSOCK(Inode.st_mode))  str[0] = 's';
    if (Inode.st_mode & S_IRUSR ) str[1] = 'r';
    if (Inode.st_mode & S_IWUSR ) str[2] = 'w';
    if (Inode.st_mode & S_IXUSR ) str[3] = 'x';
    if (Inode.st_mode & S_IRGRP ) str[4] = 'r';
    if (Inode.st_mode & S_IWGRP ) str[5] = 'w';
    if (Inode.st_mode & S_IXGRP ) str[6] = 'x';
    if (Inode.st_mode & S_IROTH ) str[7] = 'r';
    if (Inode.st_mode & S_IWOTH ) str[8] = 'w';
    if (Inode.st_mode & S_IXOTH ) str[9] = 'x';

	ino_t inodeNum = Inode.st_ino;
	blkcnt_t blocks = Inode.st_blocks/2;
	nlink_t linkNum = Inode.st_nlink;
	off_t fileSize = Inode.st_size;
	char dateTime[20];
	strftime(dateTime, 20, "%y-%m-%d   %H:%M", localtime(&(Inode.st_mtime)));

	char symPath[buf];
	if(S_ISLNK(Inode.st_mode)){
		int len = readlink(path, symPath, buf);
		symPath[len] = '\0';
		strcat(path, " -> ");
		strcat(path, symPath);
	}
	if (mtimeflag == true){
		if (mtimeVal <= time(NULL) - Inode.st_mtime){
			printf(" %15d  %4d      %10s  %5d   %10s %10s%15d       %10s    %s \n", inodeNum, blocks, str, linkNum, userName, groupName, fileSize, dateTime, path);
		}
	}
	if (mtimeflag == false){
			printf(" %15d  %4d      %10s  %5d   %10s %10s%15d       %10s    %s \n", inodeNum, blocks, str, linkNum, userName, groupName, fileSize, dateTime, path);
	}
	return 0;
}
