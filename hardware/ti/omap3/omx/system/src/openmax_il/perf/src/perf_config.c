
/*
 * Copyright (C) Texas Instruments - http://www.ti.com/
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 */
#include "perf_config.h"
#include "perf_common.h"
#include <ctype.h>

#ifdef ANDROID
/* Log for Android system*/
#include <utils/Log.h>
#endif

/* pre-declare helper functions */
static int  assign_string(char **psMember, char const *sValue);
static int  assign_string_if_matches(char const *line, char const *argument,
                                     char **psMember);
static int  assign_long(unsigned long *piMember, char const *sValue);
static int  assign_long_if_matches(char const *line, char const *argument,
                                   unsigned long *piMember);
static void read_line(PERF_Config *sConfig, char const *line, char const *tag);
static char const *get_value_if_matches(char const *line, char const *argument);

/*-----------------------------------------------------------------------------
  CONFIGURATION METHODS FUNCTIONS
-----------------------------------------------------------------------------*/
/* default configuration */
void PERF_Config_Init(PERF_Config *sConfig)
{
    /* initialize default configuration */

    sConfig->mask           = 0;

    /* logging interface */
    sConfig->trace_file     = NULL;
    sConfig->delayed_open   = 0;
    sConfig->buffer_size    = 65536;

    /* debug interface */
    sConfig->debug          = FALSE;
    sConfig->detailed_debug = FALSE;
    sConfig->csv            = 1;
    sConfig->log_file       = NULL;

    /* replay interface */
    sConfig->replay_file    = strdup("STDOUT");

    /* real-time interface */
    sConfig->realtime       = 0;
    sConfig->rt_granularity = 1;
    sConfig->rt_summary     = 1;
    sConfig->rt_debug       = 0;
    sConfig->rt_detailed    = 0;
    sConfig->rt_file        = strdup("STDERR");
}

/* release configuration memory */
void PERF_Config_Release(PERF_Config *sConfig)
{
    /* release all allocated members */
    if (sConfig->trace_file)
    {
        free(sConfig->trace_file);
        sConfig->trace_file = NULL;
    }
    if (sConfig->log_file)
    {
        free(sConfig->log_file);
        sConfig->log_file = NULL;
    }
    if (sConfig->replay_file)
    {
        free(sConfig->replay_file);
        sConfig->replay_file = NULL;
    }
    if (sConfig->rt_file)
    {
        free(sConfig->rt_file);
        sConfig->rt_file = NULL;
    }
}

/** Method  read_line
 * 
 *  Arg1    pointer to configuration
 * 
 *  Arg2    configuration line (trimmed of trailing white
 *  spaces)
 * 
 *  Arg3    tag - restrict matches to this tag or no-tag
 * 
 *  Effects compares configuration lines to assignments to each
 *  configuration variable.  If tags are specified before the
 *  configuration variable, they are ignored unless they match
 *  the supplied tag. If it matches, the assignment is performed
 *  to the variable. Otherwise, an error message is printed.
 *  */

static
void read_line(PERF_Config *cfg, char const *line, char const *tag)
{
    char const *ptr;

    /* skip leading spaces */
    while (*line && isspace(*line)) line++;

    /* ignore comment lines and empty lines */
    if (!*line || *line == '#') return;

    /* check to see if there is a tag prefix */    

    /* find first white-space or . in the line */
    for (ptr = line; *ptr && !isspace(*ptr) && *ptr != '.' && *ptr != '='; ptr++);

    if (*ptr == '.')
    {
        /* ignore lines where the tag does not match */
        if (!tag || strncmp(line, tag, ptr - line)) return;

        /* otherwise, skip the tag for the match */
        line = ptr + 1;
    }

    /* check for known member names */

    if (!(assign_long_if_matches(line, "mask",          &cfg->mask) ||
          /* logging configuration */
          assign_string_if_matches(line, "trace_file",  &cfg->trace_file) ||
          assign_long_if_matches(line, "delayed_open",  &cfg->delayed_open) || 
          assign_long_if_matches(line, "buffer_size",   &cfg->buffer_size) ||
          /* debug configuration */
          assign_string_if_matches(line, "log_file",    &cfg->log_file) ||
          assign_long_if_matches(line, "debug",         &cfg->debug) ||
          assign_long_if_matches(line, "detailed_debug",&cfg->detailed_debug) || 
          assign_long_if_matches(line, "csv",           &cfg->csv) ||
          /* replay configuration */
          assign_string_if_matches(line, "replay_file", &cfg->replay_file) ||
          /* real-time configuration */
          assign_long_if_matches(line, "realtime",       &cfg->realtime) ||
          assign_long_if_matches(line, "rt_granularity", &cfg->rt_granularity) ||
          assign_long_if_matches(line, "rt_debug",       &cfg->rt_debug) ||
          assign_long_if_matches(line, "rt_detailed",    &cfg->rt_detailed) ||
          assign_long_if_matches(line, "rt_summary",     &cfg->rt_summary) ||
          assign_string_if_matches(line, "rt_file",      &cfg->rt_file)
          ))

    {
        fprintf(stderr,
                "warning: incorrect line in configuration file:\n%s\n", line);
    }
}

/*
    Effects: reads each line of the perf.ini file and processes configuration
    assignments in linear order.  Maximum line length is enforced, and all
    lines longer than this are ignored.  Also, all lines must end in new-line
    If ulID is specified, lines starting with the fourCC ULID. will also be
    read.  Lines starting with # will be ignored.
*/
void PERF_Config_Read(PERF_Config *sConfig, char const *tag)
{
    FILE *config_file = NULL;
    char line[PERF_CONFIG_LINELENGTH];
    int ignore = FALSE;

    if (sConfig)
    {
        /* open config file */
        config_file = fopen(PERF_CONFIG_FILE, "rt");
        if (config_file)
        {
            /* read each line */
            while (fgets(line, PERF_CONFIG_LINELENGTH, config_file))
            {
                if (/* strlen(line) == PERF_CONFIG_LINELENGTH && */
                    *line && line[strlen(line)-1] != '\n')
                {
                    /* ignore lines that reach the max length */
                    ignore = TRUE;
                }
                else if (!ignore)
                {
                    /* remove new-line and trailing spaces from end of line */
                    while (*line && isspace(line [strlen(line)-1]))
                    {
                        line[strlen(line)-1] = 0;
                    }

                    /* process un-ignored lines */
                    read_line(sConfig, line, tag);
                }
                else
                {
                    /* no longer ignore lines after they are completely read */
                    ignore = FALSE;
                }
            }

            /* done */
            fclose(config_file);
        }
    }
}

/*-----------------------------------------------------------------------------
  HELPER FUNCTIONS
-----------------------------------------------------------------------------*/

/** Method  get_value_if_matches
 * 
 *  Arg1    configuration line
 * 
 *  Arg2    configuration variable name
 * 
 *  Effects if the configuration line is <variable name> =
 *  <value>, it returns the <value>. Otherwise, it returns NULL
 * 
 *  */
static
char const *get_value_if_matches(char const *line,
                                 char const *argument)
{
    /* skip leading spaces */
    while (*line && isspace(*line)) line++;

    /* return NULL if argument name does not match */
    if (strncasecmp(line, argument, strlen(argument))) return (NULL);
    line += strlen(argument);

    /* there must be an = after argument name */

    /* skip trailing spaces before = */
    while (*line && isspace(*line)) line++;

    /* return NULL if = not found */
    if (*line != '=') return (NULL);
    line++;

    /* skip leading spaces before value */
    while (*line && isspace(*line)) line++;

    /* if reached the end of line, return NULL; otherwise, return value */
    return(*line ? line : NULL);
}

/** Method  assign_string
 * 
 *  Arg1    pointer to string configuration member
 * 
 *  Arg2    configuration value
 * 
 *  Effects Assigns the value to the configuration member.
 *  */

static
int assign_string(char **psMember, char const *sValue)
{
    /* delete any prior value */
    if (*psMember)
    {
        free(*psMember);
        *psMember = NULL;
    }

    /* set new value unless it is NULL */
    if (strcasecmp(sValue, "NULL"))
    {
        *psMember = strdup(sValue);
    }

    return (1);
}

/** Method  assign_long
 * 
 *  Arg1    pointer to configuration member
 * 
 *  Arg2    configuration value (string)
 * 
 *  Effects Assigns the integer value of the string to the
 *  configuration member.  If value starts with '$' or '0x', it
 *  interprets the remaining digits as a hexadecimal number.  If
 *  value starts with -, or with a digit, it is interpreted as a
 *  decimal (can be signed or unsigned).  Otherwise, if it
 *  matches 'enabled', 'on' or 'true', the member is assigned 1.
 *  In all other cases, it is assigned 0.
 *  */

static
int assign_long(unsigned long *piMember, char const *sValue)
{
    /* set new value */

    /* hexadecimal value */
    if (!strncasecmp(sValue, "0x", 2)) sscanf(sValue + 2, "%lx", piMember);
    else if (*sValue == '$') sscanf(sValue + 1, "%lx", piMember);

    /* decimal value */
    else if (*sValue == '-') sscanf(sValue, "%ld", piMember);
    else if (isdigit(*sValue)) sscanf(sValue, "%lu", piMember);

    /* boolean value */
    else *piMember = (!strcasecmp(sValue, "enabled") ||
                      !strcasecmp(sValue, "on") ||
                      !strcasecmp(sValue, "true"));

    return (1);
}

/** Method  assign_string_if_matches
 * 
 *  Arg1    configuration line
 * 
 *  Arg2    configuration variable name
 * 
 *  Arg3    pointer to string configuration member
 *
 *  Effects if the configuration line is <variable name> =
 *  <value>, it assigns the value to the configuration
 *  member.
 *
 *  Returns 1, if assignment occured. 0 otherwise.
 *  */

static
int assign_string_if_matches(char const *line, char const *argument,
                             char **target)
{
    char const *value = get_value_if_matches(line, argument);
    return (value ? (assign_string(target, value), 1) : 0);
}

/** Method  assign_long_if_matches
 * 
 *  Arg1    configuration line
 * 
 *  Arg2    configuration variable name
 * 
 *  Arg3    pointer to string configuration member
 *
 *  Effects if the configuration line is <variable name> =
 *  <value>, it assigns the integer value of the string to the
 *  configuration member in the following manner:  If value
 *  starts with '$' or '0x', it interprets the remaining digits
 *  as a hexadecimal number. If value starts with -, or with a
 *  digit, it is interpreted as a decimal (can be signed or
 *  unsigned). Otherwise, if it matches 'enabled', 'on' or
 *  'true', the member is assigned 1. In all other cases, it is
 *  assigned 0.
 *
 *  Returns 1, if assignment occured. 0 otherwise (if config
 *  line was not an assignment to variable name).
 *  */

static
int assign_long_if_matches(char const *line, char const *argument,
                           unsigned long *target)
{
    char const *value = get_value_if_matches(line, argument);
    return (value ? (assign_long(target, value), 1) : 0);
}


