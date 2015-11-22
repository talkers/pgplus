/*
 * Playground+ - fix.h
 * Code to fix all those nasty warnings
 * ---------------------------------------------------------------------------
*/

#ifndef SOLARIS
struct rlimit;
struct timval;
struct itimerval;

#ifndef REDHAT5
extern int      sigpause(int);
#endif

#ifndef BSDISH
   #if !defined(linux)
      extern int      setrlimit(int, struct rlimit *);
   #endif /* LINUX */
#endif

#ifndef REDHAT5
extern int      getitimer(int, struct itimerval *);
extern int      getrlimit(int, struct rlimit *);
#endif

#ifndef BSDISH
   #if !defined(linux)
      extern int      setitimer(int, struct itimerval *, struct itimerval *);
   #endif /* LINUX */
   extern void     lower_case(char *);
   #if !defined(linux)
      extern int      strcasecmp(char *, char *);
   #endif /* LINUX */
#endif /* BSDISH */

extern void     lower_case(char *);
#endif
