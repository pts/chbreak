chbreak: breaking out of chroot using chdir(".."), and mitigation techniques
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
chbreak is a C program which can break out of a chroot(2) environment using
the chdir("..") technique from 1999, which even works on Linux 4.18. The
repository also contains some other tools to set up a hardened chroot
environment for which the chdir("..") technique doesn't work to break out.

See also the comments near the beginning of the .c files for some relevant
links.

__END__
