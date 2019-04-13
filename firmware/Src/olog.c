//-------------------------------------------------------------------------
//  olog.c
//-------------------------------------------------------------------------

#include <stdarg.h>
#include "olog.h"

static const char BEGIN_FATAL[] = "\033[91mFATAL: ";
static const char BEGIN_ERROR[] =  "\033[31mERROR: ";
static const char BEGIN_WARNING[] = "\033[33mWARNING: ";
static const char BEGIN_INFO[] = "\033[32mINFO: ";
static const char BEGIN_DEBUG[] =  "DEBUG: ";
static const char END_LOG[] = "\n\033[0m";

void olog_log(int level, const char *fmt, ...)
{
    if (level == OLOG_FATAL){
        olog_write(BEGIN_FATAL, sizeof(BEGIN_FATAL) - 1);
    }else if (level == OLOG_ERROR){
        olog_write(BEGIN_ERROR, sizeof(BEGIN_ERROR) - 1);
    }else if (level == OLOG_WARNING){
        olog_write(BEGIN_WARNING, sizeof(BEGIN_WARNING) - 1);
    }else if (level == OLOG_INFO){
        olog_write(BEGIN_INFO, sizeof(BEGIN_INFO) - 1);
    }else if (level == OLOG_DEBUG){
        olog_write(BEGIN_DEBUG, sizeof(BEGIN_DEBUG) - 1);
    }
 
    va_list args;
    va_start(args, fmt);
    olog_vprintf(fmt, &args);
    va_end(args);

    olog_write(END_LOG, sizeof(END_LOG) - 1);
}
