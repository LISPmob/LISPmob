/*
 * lispd_log.c
 *
 * This file is part of LISP Mobile Node Implementation.
 * Write log messages
 * 
 * Copyright (C) 2011 Cisco Systems, Inc, 2011. All rights reserved.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 *
 * Please send any bug reports or fixes you make to the email address(es):
 *    LISP-MN developers <devel@lispmob.org>
 *
 * Written or modified by:
 *    David Meyer       <dmm@cisco.com>
 *    Preethi Natarajan <prenatar@cisco.com>
 *    Alberto Rodriguez Natal <arnatal@ac.ucp.edu>
 *
 */

#include "lispd_log.h"
#include "lispd_external.h"
#include <syslog.h>
#include <stdarg.h>
#include <time.h>

FILE *fp = NULL;

static inline void lispd_log(
        int         log_level,
        char        *log_name,
        const char  *format,
        va_list     args);


void lispd_log_msg1(int lisp_log_level, const char *format, ...)
{
    va_list args;
    char *log_name; /* To store the log level in string format for printf output */
    int log_level;


    va_start (args, format);

    switch (lisp_log_level){
    case LISP_LOG_CRIT:
        log_name = "CRIT";
        log_level = LOG_CRIT;
        lispd_log(log_level, log_name, format, args);
        break;
    case LISP_LOG_ERR:
        log_name = "ERR";
        log_level = LOG_ERR;
        lispd_log(log_level, log_name, format, args);
        break;
    case LISP_LOG_WARNING:
        log_name = "WARNING";
        log_level = LOG_WARNING;
        lispd_log(log_level, log_name, format, args);
        break;
    case LISP_LOG_INFO:
        log_name = "INFO";
        log_level = LOG_INFO;
        lispd_log(log_level, log_name, format, args);
        break;
    case LISP_LOG_DEBUG_1:
        if (debug_level > 0){
            log_name = "DEBUG";
            log_level = LOG_DEBUG;
            lispd_log(log_level, log_name, format, args);
        }
        break;
    case LISP_LOG_DEBUG_2:
        if (debug_level > 1){
            log_name = "DEBUG-2";
            log_level = LOG_DEBUG;
            lispd_log(log_level, log_name, format, args);
        }
        break;
    case LISP_LOG_DEBUG_3:
        if (debug_level > 2){
            log_name = "DEBUG-3";
            log_level = LOG_DEBUG;
            lispd_log(log_level, log_name, format, args);
        }
        break;
    default:
        log_name = "LOG";
        log_level = LOG_INFO;
        lispd_log(log_level, log_name, format, args);
        break;
    }
#ifdef ANDROID
    //if (lisp_log_level != LISP_LOG_DEBUG_3)
    	__android_log_vprint(ANDROID_LOG_INFO, "LISPmob-C ==>", format,args);
#endif
    va_end (args);
}

static inline void lispd_log(
        int         log_level,
        char        *log_name,
        const char  *format,
        va_list     args)
{
	time_t t = time(NULL);
	struct tm tm = *localtime(&t);

#ifdef ANDROID
    if (fp != NULL){
        fprintf(fp,"[%d/%d/%d %d:%d:%d] %s: ",
        		tm.tm_year + 1900, tm.tm_mon + 1, tm.tm_mday, tm.tm_hour, tm.tm_min, tm.tm_sec, log_name);
        vfprintf(fp,format,args);
        fprintf(fp,"\n");
        fflush(fp);
    }else{
        vsyslog(log_level,format,args);
    }
#else
	if (daemonize){
	    if (fp != NULL){
	    	fprintf(fp,"[%d/%d/%d %d:%d:%d] %s: ",
	    			tm.tm_year + 1900, tm.tm_mon + 1, tm.tm_mday, tm.tm_hour, tm.tm_min, tm.tm_sec, log_name);
	    	vfprintf(fp,format,args);
	    	fprintf(fp,"\n");
	        fflush(fp);
	    }else{
	        vsyslog(log_level,format,args);
	    }
	}else{
        printf("[%d/%d/%d %d:%d:%d] %s: ",
        		tm.tm_year + 1900, tm.tm_mon + 1, tm.tm_mday, tm.tm_hour, tm.tm_min, tm.tm_sec, log_name);
	    vfprintf(stdout,format,args);
		printf("\n");
	}
#endif
}

void open_log_file(char *log_file)
{
    if (log_file == NULL){
        return;
    }
    /* Overwrite log file in each start */
	fp = freopen(log_file, "w", stderr);
	if (fp == NULL){
	    printf("ERR: Couldn't open log file: %s. Using  syslog\n", log_file);
	    lispd_log_msg1(LISP_LOG_ERR,"Couldn't open log file: %s. Using  syslog", log_file);
	}
}

void close_log_file()
{
	fclose (fp);
}

/*
 * True if log_level is enough to print results
 */

/* True if log_level is enough to print results */
inline int is_loggable(int log_level)
{
    if (log_level < LISP_LOG_DEBUG_1)
        return (TRUE);
    else if (log_level <= LISP_LOG_INFO + debug_level)
        return (TRUE);
    return (FALSE);
}
/*
 * Editor modelines
 *
 * vi: set shiftwidth=4 tabstop=4 expandtab:
 * :indentSize=4:tabSize=4:noTabs=true:
 */
