// thanks to https://stackoverflow.com/questions/10404448/getopt-h-compiling-linux-c-code-in-windows#17195644
// since windows doesn't have a built in function called getopt - linux function

#pragma once

#include <string.h>
#include <stdio.h>

extern int  opterr = 1,             /* if error message should be printed */
            optind = 1,             /* index into parent argv vector */
            optopt = 0,             /* character checked for validity */
            optreset = 0;               /* reset getopt */
extern char* optarg = (char*)"";                /* argument associated with option */

#define BADCH   (int)'?'
#define BADARG  (int)':'
#define EMSG    (char *)""

// to parse command line options. Included in unistd.h, and getopt.h for linux machines
int getopt(int nargc, char* const nargv[], const char* ostr)
{
    static char* place = EMSG;              /* option letter processing */
    const char* oli;                        /* option letter list index */

    if (optreset || !*place) {              /* update scanning pointer */
        optreset = 0;
        if (optind >= nargc || *(place = nargv[optind]) != '-') {
            place = EMSG;
            return (-1);
        }
        if (place[1] && *++place == '-') {      /* found "--" */
            ++optind;
            place = EMSG;
            return (-1);
        }
    }                                       /* option letter okay? */
    if ((optopt = (int)*place++) == (int)':' ||
        !(oli = strchr(ostr, optopt))) {
        /*
        * if the user didn't specify '-' as an option,
        * assume it means -1.
        */
        if (optopt == (int)'-')
            return (-1);
        if (!*place)
            ++optind;
        if (opterr && *ostr != ':')
            (void)printf("illegal option -- %c\n", optopt);
        return (BADCH);
    }
    if (*++oli != ':') {                    /* don't need argument */
        optarg = NULL;
        if (!*place)
            ++optind;
    }
    else {                                  /* need an argument */
        if (*place)                     /* no white space */
            optarg = place;
        else if (nargc <= ++optind) {   /* no arg */
            place = EMSG;
            if (*ostr == ':')
                return (BADARG);
            if (opterr)
                (void)printf("option requires an argument -- %c\n", optopt);
            return (BADCH);
        }
        else                            /* white space */
            optarg = nargv[optind];
        place = EMSG;
        ++optind;
    }
    return (optopt);                        /* dump back option letter */
}