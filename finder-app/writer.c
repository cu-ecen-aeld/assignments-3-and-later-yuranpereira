#include <sys/types.h>
#include <sys/stat.h>

#include <stdio.h>
#include <fcntl.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <syslog.h>

int main(int argc, char **argv)
{
    char *path, *str;
    int len, fd;

    if (argc < 3) {
        printf("Two arguments must be provided\n");
        return 1;
    }
    openlog(NULL, 0, LOG_USER);
    
    path = argv[1];
    str = argv[2];
    len = strlen(str);

    /* Attempt to open file and log errors if it fails. */
    fd = open(path, O_WRONLY | O_CREAT | O_TRUNC, S_IRUSR | S_IWUSR);
    if (fd == -1) {
        syslog(LOG_ERR, "Could not create file:  %m");
        closelog();
        return 1;
    }
    
    syslog(LOG_DEBUG, "Writing %s to %s", str, path);
    write(fd, str, len);

    /* Close syslog and file*/
    closelog();
    close(fd);

    return 0;
}
