#include <stdlib.h>
#include <string.h>
#include <unistd.h>     // getuid, getgid, getgroups
#include <sys/types.h>
#include <sys/stat.h>
#include <errno.h>
#include <limits.h>
#include <sys/syscall.h>

// Global list of environment variables
extern char **environ;

#ifndef PATH_MAX
#define PATH_MAX 4096
#endif


// Clone of getenv()
char *get_env(const char *__name){
    size_t len = strlen(__name);
    for(char **env = environ; *env != NULL; env++){
        if(strncmp(*env, __name,len)==0 && (*env)[len] == '='){
            return strdup(*env + len + 1);
        }
    }
    return NULL;
}

// =========== Clone of access() function and some of other functions it requires ====================

static inline uid_t get_uid(void){
    return (uid_t) syscall(SYS_getuid);
}

static inline gid_t get_gid(void){
    return (gid_t) syscall(SYS_getgid);
}

static int check_mode_bit(mode_t st_mode, uid_t st_uid, gid_t st_gid, uid_t ruid, gid_t rgid,
    gid_t *groups, int ngroups, mode_t owner_bit, mode_t group_bit, mode_t other_bit){
        
        // Special case for root
        if(ruid == 0){
            if(owner_bit == (mode_t)0 && group_bit == (mode_t)0 && other_bit == (mode_t)0){ // If no tests are passed, root can have access
                return 1;
            }
            if((owner_bit & (S_IXUSR | S_IXGRP | S_IXOTH)) != 0){ // If we are checking for the execution privilege
                if (st_mode & (S_IXUSR|S_IXGRP|S_IXOTH)){  // If any group has execution privileges, root also has access
                    return 1;
                }
                else{
                    return 0;
                }
            }
            else{
                return 1; // R/W is allowed for root in this (simplified) model
            }
        }

        if(ruid == st_uid){ // If user is the file's owner, compare owner's bit
            return (st_mode & owner_bit) ? 1 : 0;
        }

        if(rgid == st_gid){ // If user among the file's group, compare group bit
            return (st_mode & group_bit) ? 1 : 0;
        }

        // check for supplementary groups
        for(int i = 0 ; i < ngroups ; i++){
            if(groups[i] == st_gid){
                return (st_mode & group_bit) ? 1 : 0;
            }
        }

        //else check for "other" category privileges
        return (st_mode & other_bit) ? 1 : 0;


}

static int check_path_prefix_search(const char *path, uid_t ruid, gid_t rgid, gid_t *groups, int ngroups){
    size_t n = strlen(path);

    if(n == 0){
        errno = ENOENT;
        return -1;
    }

    char buf[PATH_MAX+1];
    if(n > PATH_MAX){
        errno = ENAMETOOLONG; 
        return -1;
    }

    strncpy(buf, path, sizeof(buf));
    buf[sizeof(buf)-1] = '\0';
    
    // Check if each file along the path is directory and is searchable (execute/search permission)

    size_t i = 0;

    // If absolute path, check root "/" first
    if(buf[0] == '/'){
        struct stat st;
        if(stat("/", &st) != 0 ){
            return -1;
        }

        // Check if called can search the root directory
        if(!check_mode_bit(st.st_mode, st.st_uid, st.st_gid, ruid, rgid, groups, ngroups, S_IXUSR, S_IXGRP, S_IXOTH)){
            errno = EACCES;
            return -1;
        }
        while(buf[i] == '/') i++; // Skip over repeated '/'
    }
    else{
        struct stat st;
        if(stat(".", &st) != 0){
            return -1;
        }
        if(!check_mode_bit(st.st_mode, st.st_uid, st.st_gid, ruid, rgid, groups, ngroups, S_IXUSR, S_IXGRP, S_IXOTH)){
            errno = EACCES;
            return -1;
        }
        i = 0;
    }

        // Iterate remaining path components and check each prefix that ends with '/'
    for(; i < n; i++){
        if(buf[i] == '/'){
            // Termporarily terminate string at the slash to form a prefix 
            buf[i] = '\0';
            if(buf[0] == '\0'){ // If prefix is empty restore and continue
                buf[i] = '/';
                continue;
            }
            struct stat st;
            if(stat(buf, &st) != 0){
                return -1;
            }
            if(!S_ISDIR(st.st_mode)){
                errno = ENOTDIR;
                return -1;
            }
            // Check search (execute) bit for this directory
            if(!check_mode_bit(st.st_mode, st.st_uid, st.st_gid, ruid, rgid, groups, ngroups, S_IXUSR, S_IXGRP, S_IXOTH)){
                errno = EACCES;
                return -1;
            }
            // Restore the '/' and skip repeated slashes
            buf[i] = '/';
            while(buf[i] == '/' && i < n) i++;
            --i; // for loop will do ++i
        }
    }
    return 0;
}

int access_file(const char *path, int type){
    if(!path){
        errno = EINVAL;
        return -1;
    }

    uid_t ruid = get_uid();
    gid_t rgid = get_gid();

    // Get supplementary groups, then allocate and fetch them
    int ng = getgroups(0, NULL);
    gid_t *groups = NULL;
    if(ng > 0){
        groups = malloc(sizeof(gid_t) * ng);
        if(!groups){
            errno = ENOMEM;
            return -1;
        }
        if(getgroups(ng, groups) < 0){
            free(groups);
            return -1;
        }
    }
    else{
        ng = 0;
    }

    // Ensure we can traverse the directory components (search permission)
    if(check_path_prefix_search(path, ruid, rgid, groups, ng) != 0){
        free(groups);
        // errno set by check_path_prefix_search()
        return -1;
    }

    struct stat st;
    if(stat(path, &st) != 0){
        free(groups);
        // errno set by stat()
        return -1;
    }

    // if type == F_OK (existence) and stat didn't return error, file already exists
    if( type == F_OK){
        free(groups);
        return 0;
    }

    // Check other permissions if requested 
    if(type & R_OK){
        int ok = check_mode_bit(st.st_mode, st.st_uid, st.st_gid, ruid, rgid, groups, ng, S_IRUSR, S_IRGRP, S_IROTH);
        if(!ok){
            free(groups);
            errno = EACCES;
            return -1;
        }
    }
    if(type & W_OK){
        int ok = check_mode_bit(st.st_mode, st.st_uid, st.st_gid, ruid, rgid, groups, ng, S_IWUSR, S_IWGRP, S_IWOTH);
        if(!ok){
            free(groups);
            errno = EACCES;
            return -1;
        }
    }
    if(type & X_OK){
        int ok = check_mode_bit(st.st_mode, st.st_uid, st.st_gid, ruid, rgid, groups, ng, S_IXUSR, S_IXGRP, S_IXOTH);
        if(!ok){
            free(groups);
            errno = EACCES;
            return -1;
        }
    }

    free(groups);
    return 0;
}
