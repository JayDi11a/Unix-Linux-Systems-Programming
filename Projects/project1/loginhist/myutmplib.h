/* utmplib.h  - header file with decls of functions in utmplib.c */

int utmp_open(char *);
struct utmp *utmp_next();
int utmp_close();
