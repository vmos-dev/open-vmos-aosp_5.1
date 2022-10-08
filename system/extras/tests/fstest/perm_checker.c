/*
 * Copyright (C) 2008 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

// A simple file permissions checker. See associated README.

#define _GNU_SOURCE

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <ctype.h>
#include <sys/types.h>
#include <dirent.h>
#include <errno.h>

#include <sys/stat.h>
#include <unistd.h>
#include <time.h>

#include <pwd.h>
#include <grp.h>

#include <linux/kdev_t.h>

#define DEFAULT_CONFIG_FILE "/data/local/perm_checker.conf"

#define PERMS(M) (M & ~S_IFMT)
#define MAX_NAME_LEN 4096
#define MAX_UID_LEN 256
#define MAX_GID_LEN MAX_UID_LEN

static char *config_file;
static char *executable_file;

enum perm_rule_type {EXACT_FILE = 0, EXACT_DIR, WILDCARD, RECURSIVE,
    NUM_PR_TYPES};

struct perm_rule {
    char *rule_text;
    int rule_line;
    char *spec;
    mode_t min_mode;
    mode_t max_mode;
    uid_t min_uid;
    uid_t max_uid;
    gid_t min_gid;
    gid_t max_gid;
    enum perm_rule_type type;
    struct perm_rule *next;
};

typedef struct perm_rule perm_rule_t;

static perm_rule_t *rules[NUM_PR_TYPES];

static uid_t str2uid(char *str, int line_num)
{
    struct passwd *pw;

    if (isdigit(str[0]))
        return (uid_t) atol(str);

    if (!(pw = getpwnam(str))) {
        printf("# ERROR # Invalid uid '%s' reading line %d\n", str, line_num);
        exit(255);
    }
    return pw->pw_uid;
}

static gid_t str2gid(char *str, int line_num)
{
    struct group *gr;

    if (isdigit(str[0]))
        return (uid_t) atol(str);

    if (!(gr = getgrnam(str))) {
        printf("# ERROR # Invalid gid '%s' reading line %d\n", str, line_num);
        exit(255);
    }
    return gr->gr_gid;
}

static void add_rule(int line_num, char *spec,
                     unsigned long min_mode, unsigned long max_mode,
                     char *min_uid_buf, char *max_uid_buf,
                     char *min_gid_buf, char *max_gid_buf) {

    char rule_text_buf[MAX_NAME_LEN + 2*MAX_UID_LEN + 2*MAX_GID_LEN + 9];
    perm_rule_t *pr = malloc(sizeof(perm_rule_t));
    if (!pr) {
        printf("Out of memory.\n");
        exit(255);
    }
    if (snprintf(rule_text_buf, sizeof(rule_text_buf),
                 "%s %lo %lo %s %s %s %s", spec, min_mode, max_mode,
                 min_uid_buf, max_uid_buf, min_gid_buf, max_gid_buf)
                 >= (long int) sizeof(rule_text_buf)) {
        // This should never happen, but just in case...
        printf("# ERROR # Maximum length limits exceeded on line %d\n",
               line_num);
        exit(255);
    }
    pr->rule_text = strndup(rule_text_buf, sizeof(rule_text_buf));
    pr->rule_line = line_num;
    if (strstr(spec, "/...")) {
        pr->spec = strndup(spec, strlen(spec) - 3);
        pr->type = RECURSIVE;
    } else if (spec[strlen(spec) - 1] == '*') {
        pr->spec = strndup(spec, strlen(spec) - 1);
        pr->type = WILDCARD;
    } else if (spec[strlen(spec) - 1] == '/') {
        pr->spec = strdup(spec);
        pr->type = EXACT_DIR;
    } else {
        pr->spec = strdup(spec);
        pr->type = EXACT_FILE;
    }
    if ((pr->spec == NULL) || (pr->rule_text == NULL)) {
        printf("Out of memory.\n");
        exit(255);
    }
    pr->min_mode = min_mode;
    pr->max_mode = max_mode;
    pr->min_uid = str2uid(min_uid_buf, line_num);
    pr->max_uid = str2uid(max_uid_buf, line_num);
    pr->min_gid = str2gid(min_gid_buf, line_num);
    pr->max_gid = str2gid(max_gid_buf, line_num);

    // Add the rule to the appropriate set
    pr->next = rules[pr->type];
    rules[pr->type] = pr;
#if 0  // Useful for debugging
    printf("rule #%d: type = %d spec = %s min_mode = %o max_mode = %o "
           "min_uid = %d max_uid = %d min_gid = %d max_gid = %d\n",
           num_rules, pr->type, pr->spec, pr->min_mode, pr->max_mode,
           pr->min_uid, pr->max_uid, pr->min_gid, pr->max_gid);
#endif
}

static int read_rules(FILE *fp)
{
    char spec[MAX_NAME_LEN + 5];  // Allows for "/..." suffix + terminator
    char min_uid_buf[MAX_UID_LEN + 1], max_uid_buf[MAX_UID_LEN + 1];
    char min_gid_buf[MAX_GID_LEN + 1], max_gid_buf[MAX_GID_LEN + 1];
    unsigned long min_mode, max_mode;
    int res;
    int num_rules = 0, num_lines = 0;

    // Note: Use of an unsafe C function here is OK, since this is a test
    while ((res = fscanf(fp, "%s %lo %lo %s %s %s %s\n", spec,
                         &min_mode, &max_mode, min_uid_buf, max_uid_buf,
                         min_gid_buf, max_gid_buf)) != EOF) {
        num_lines++;
        if (res < 7) {
            printf("# WARNING # Invalid rule on line number %d\n", num_lines);
            continue;
        }
        add_rule(num_lines, spec,
                 min_mode, max_mode,
                 min_uid_buf, max_uid_buf,
                 min_gid_buf, max_gid_buf);
        num_rules++;
    }

    // Automatically add a rule to match this executable itself
    add_rule(-1, executable_file,
             000, 0777,
             "root", "shell",
             "root", "shell");

    // Automatically add a rule to match the configuration file
    add_rule(-1, config_file,
             000, 0777,
             "root", "shell",
             "root", "shell");

    return num_lines - num_rules;
}

static void print_failed_rule(const perm_rule_t *pr)
{
    printf("# INFO # Failed rule #%d: %s\n", pr->rule_line, pr->rule_text);
}

static void print_new_rule(const char *name, mode_t mode, uid_t uid, gid_t gid)
{
    struct passwd *pw;
    struct group *gr;
    gr = getgrgid(gid);
    pw = getpwuid(uid);
    printf("%s %4o %4o %s %d %s %d\n", name, mode, mode, pw->pw_name, uid,
           gr->gr_name, gid);
}

// Returns 1 if the rule passes, prints the failure and returns 0 if not
static int pass_rule(const perm_rule_t *pr, mode_t mode, uid_t uid, gid_t gid)
{
    if (((pr->min_mode & mode) == pr->min_mode) &&
            ((pr->max_mode | mode) == pr->max_mode) &&
            (pr->min_gid <= gid) && (pr->max_gid >= gid) &&
            (pr->min_uid <= uid) && (pr->max_uid >= uid))
        return 1;
    print_failed_rule(pr);
    return 0;
}

// Returns 0 on success
static int validate_file(const char *name, mode_t mode, uid_t uid, gid_t gid)
{
    perm_rule_t *pr;
    int rules_matched = 0;
    int retval = 0;

    pr = rules[EXACT_FILE];
    while (pr != NULL) {
        if (strcmp(name, pr->spec) == 0) {
            if (!pass_rule(pr, mode, uid, gid))
                retval++;
            else
                rules_matched++;  // Exact match found
        }
        pr = pr->next;
    }

    if ((retval + rules_matched) > 1)
        printf("# WARNING # Multiple exact rules for file: %s\n", name);

    // If any exact rule matched or failed, we are done with this file
    if (retval)
        print_new_rule(name, mode, uid, gid);
    if (rules_matched || retval)
        return retval;

    pr = rules[WILDCARD];
    while (pr != NULL) {
        // Check if the spec is a prefix of the filename, and that the file
        // is actually in the same directory as the wildcard.
        if ((strstr(name, pr->spec) == name) &&
                (!strchr(name + strlen(pr->spec), '/'))) {
            if (!pass_rule(pr, mode, uid, gid))
                retval++;
            else
                rules_matched++;
        }
        pr = pr->next;
    }

    pr = rules[RECURSIVE];
    while (pr != NULL) {
        if (strstr(name, pr->spec) == name) {
            if (!pass_rule(pr, mode, uid, gid))
                retval++;
            else
                rules_matched++;
        }
        pr = pr->next;
    }

    if (!rules_matched)
        retval++;  // In case no rules either matched or failed, be sure to fail

    if (retval)
        print_new_rule(name, mode, uid, gid);

    return retval;
}

// Returns 0 on success
static int validate_link(const char *name, mode_t mode, uid_t uid, gid_t gid)
{
    perm_rule_t *pr;
    int rules_matched = 0;
    int retval = 0;

    // For now, we match links against "exact" file rules only
    pr = rules[EXACT_FILE];
    while (pr != NULL) {
        if (strcmp(name, pr->spec) == 0) {
            if (!pass_rule(pr, mode, uid, gid))
                retval++;
            else
                rules_matched++;  // Exact match found
        }
        pr = pr->next;
    }

    if ((retval + rules_matched) > 1)
        printf("# WARNING # Multiple exact rules for link: %s\n", name);
    if (retval)
        print_new_rule(name, mode, uid, gid);

    // Note: Unlike files, if no rules matches for links, retval = 0 (success).
    return retval;
}

// Returns 0 on success
static int validate_dir(const char *name, mode_t mode, uid_t uid, gid_t gid)
{
    perm_rule_t *pr;
    int rules_matched = 0;
    int retval = 0;

    pr = rules[EXACT_DIR];
    while (pr != NULL) {
        if (strcmp(name, pr->spec) == 0) {
            if (!pass_rule(pr, mode, uid, gid))
                retval++;
            else
                rules_matched++;  // Exact match found
        }
        pr = pr->next;
    }

    if ((retval + rules_matched) > 1)
        printf("# WARNING # Multiple exact rules for directory: %s\n", name);

    // If any exact rule matched or failed, we are done with this directory
    if (retval)
        print_new_rule(name, mode, uid, gid);
    if (rules_matched || retval)
        return retval;

    pr = rules[RECURSIVE];
    while (pr != NULL) {
        if (strstr(name, pr->spec) == name) {
            if (!pass_rule(pr, mode, uid, gid))
                retval++;
            else
                rules_matched++;
        }
        pr = pr->next;
    }

    if (!rules_matched)
        retval++;  // In case no rules either matched or failed, be sure to fail

    if (retval)
        print_new_rule(name, mode, uid, gid);

    return retval;
}

// Returns 0 on success
static int check_path(const char *name)
{
    char namebuf[MAX_NAME_LEN + 1];
    char tmp[MAX_NAME_LEN + 1];
    DIR *d;
    struct dirent *de;
    struct stat s;
    int err;
    int retval = 0;

    err = lstat(name, &s);
    if (err < 0) {
        if (errno != ENOENT)
        {
            perror(name);
            return 1;
        }
        return 0;  // File doesn't exist anymore
    }

    if (S_ISDIR(s.st_mode)) {
        if (name[strlen(name) - 1] != '/')
            snprintf(namebuf, sizeof(namebuf), "%s/", name);
        else
            snprintf(namebuf, sizeof(namebuf), "%s", name);

        retval |= validate_dir(namebuf, PERMS(s.st_mode), s.st_uid, s.st_gid);
        d = opendir(namebuf);
        if(d == 0) {
            printf("%s : opendir failed: %s\n", namebuf, strerror(errno));
            return 1;
        }

        while ((de = readdir(d)) != 0) {
            if (!strcmp(de->d_name, ".") || !strcmp(de->d_name, ".."))
                continue;
            snprintf(tmp, sizeof(tmp), "%s%s", namebuf, de->d_name);
            retval |= check_path(tmp);
        }
        closedir(d);
        return retval;
    } else if (S_ISLNK(s.st_mode)) {
        return validate_link(name, PERMS(s.st_mode), s.st_uid, s.st_gid);
    } else {
        return validate_file(name, PERMS(s.st_mode), s.st_uid, s.st_gid);
    }
}

int main(int argc, char **argv)
{
    FILE *fp;
    int i;

    if (argc > 2) {
      printf("\nSyntax: %s [configfilename]\n", argv[0]);
    }
    config_file = (argc == 2) ? argv[1] : DEFAULT_CONFIG_FILE;
    executable_file = argv[0];

    // Initialize ruleset pointers
    for (i = 0; i < NUM_PR_TYPES; i++)
        rules[i] = NULL;

    if (!(fp = fopen(config_file, "r"))) {
        printf("Error opening %s\n", config_file);
        exit(255);
    }
    read_rules(fp);
    fclose(fp);

    if (check_path("/"))
        return 255;

    printf("Passed.\n");
    return 0;
}
