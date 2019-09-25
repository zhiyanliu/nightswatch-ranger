//
// Created by Zhi Yan Liu on 2019/9/23.
//

#include <errno.h>
#ifdef __linux__
#include <linux/limits.h>
#elif defined(__APPLE__)
#include <sys/syslimits.h>
#endif
#include <signal.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>

#include "aws_iot_json_utils.h"
#include "aws_iot_log.h"

#include "apps.h"
#include "rund.h"
#include "utils.h"


int app_run(char *app_home_path_v, char *app_process_pid_path_v, char *app_name) {
    char app_spec_path_v[PATH_MAX + 1], app_root_path_v[PATH_MAX + 1], *app_spec, **app_argv;
    int app_spec_l, app_argc, arg_idx, pid, fd[2], rc = 0;
    FILE *fp;

    jsmn_parser json_spec_parser, json_process_parser;
    jsmntok_t json_spec_tok_v[IROOTECH_DMP_RP_RUND_APP_SPEC_MAX], *tok_app_process,
        json_process_tok_v[IROOTECH_DMP_RP_RUND_APP_PROCESS_MAX], *tok_app_args;
    int32_t token_c;

    rc = app_root_path(app_root_path_v, PATH_MAX + 1, app_name);
    if (-1 == rc) {
        IOT_ERROR("failed to get application process file path: %d", rc);
        return rc;
    }

    // application config
    rc = app_spec_path(app_spec_path_v, PATH_MAX + 1, app_name);
    if (-1 == rc) {
        IOT_ERROR("failed to get application spec file path: %d", rc);
        return rc;
    }

    app_spec = read_str_file(app_spec_path_v, &app_spec_l);
    if (NULL == app_spec) {
        IOT_ERROR("failed to read application spec file at %s", app_spec_path_v);
        return 1;
    }

    // application spec
    jsmn_init(&json_spec_parser);
    token_c = jsmn_parse(&json_spec_parser, app_spec, app_spec_l, json_spec_tok_v, IROOTECH_DMP_RP_RUND_APP_SPEC_MAX);
    if(token_c < 0) {
        IOT_ERROR("failed to parse application spec json: %d", token_c);
        rc = token_c;
        goto end;
    }

    tok_app_process = findToken(RUND_SPEC_APP_PROCESS_PROPERTY_NAME, app_spec, json_spec_tok_v);
    if (NULL == tok_app_process) {
        IOT_WARN("job application process property %s not found, nothing to do", RUND_SPEC_APP_PROCESS_PROPERTY_NAME);
        rc = 1;
        goto end;
    }

    // application process
    jsmn_init(&json_process_parser);
    token_c = jsmn_parse(&json_process_parser, app_spec + tok_app_process->start,
            tok_app_process->end - tok_app_process->start,
            json_process_tok_v, IROOTECH_DMP_RP_RUND_APP_PROCESS_MAX);
    if (token_c < 0) {
        IOT_ERROR("failed to parse application process json: %d", token_c);
        rc = token_c;
        goto end;
    }

    // application process arguments
    tok_app_args = findToken(RUND_SPEC_APP_ARGUMENTS_PROPERTY_NAME, app_spec + tok_app_process->start,
            json_process_tok_v);
    if (NULL == tok_app_args) {
        IOT_WARN("job application arguments property %s not found, nothing to do",
                RUND_SPEC_APP_ARGUMENTS_PROPERTY_NAME);
        rc = 1;
        goto end;
    }

    if (JSMN_ARRAY != tok_app_args->type) {
        IOT_WARN("job application arguments property is not a valid string array");
        rc = 1;
        goto end;
    }

    app_argc = tok_app_args->size;
    if (0 < app_argc) {
        size_t arg_buff_len;
        jsmntok_t *tok_app_arg;

        app_argv = malloc(app_argc * sizeof(char*) + 1);  // last one is NULL, for execvp() call

        for (arg_idx = 0; arg_idx < app_argc; arg_idx++) {
            tok_app_arg = &tok_app_args[arg_idx + 1];
            arg_buff_len = (tok_app_arg->end - tok_app_arg->start + 1) * sizeof(char);

            if (0 == arg_idx) {
                int len = strlen(app_root_path_v);
                // add application root path to first argument
                app_argv[arg_idx] = malloc((PATH_MAX + 1) * sizeof(char));
                strcpy(app_argv[arg_idx], app_root_path_v);
                strcpy(app_argv[arg_idx] + len, "/");
                strncpy(app_argv[arg_idx] + len + 1,
                       app_spec + tok_app_process->start + tok_app_arg->start, arg_buff_len - 1);
            } else {
                app_argv[arg_idx] = malloc(arg_buff_len);
                memset(app_argv[arg_idx], 0, arg_buff_len);
                strncpy(app_argv[arg_idx], app_spec + tok_app_process->start + tok_app_arg->start, arg_buff_len - 1);
            }
        }

        app_argv[app_argc] = NULL;
    }

    // write pid file for the application process
    fp = fopen(app_process_pid_path_v, "wb");
    if (NULL == fp) {
        IOT_ERROR("failed to write pid file for application %s", app_name);
        rc = 1;
        goto end;
    }
    fprintf(fp, "%d", getpid());
    fclose(fp);

    // switch to application process
    setsid();
    chdir(app_root_path_v);
    signal (SIGHUP, SIG_IGN);
    rc = execvp(app_argv[0], app_argv);
    if (-1 == rc) {
        IOT_ERROR("failed to launch the process for application %s, errno = %d", app_name, errno);
        unlink(app_process_pid_path_v);
        goto end;
    }

    rc = 0;

end:
    free(app_spec);

    if (app_argc > 0) {
        for (arg_idx = 0; arg_idx < app_argc - 1; arg_idx++)
            free(app_argv[arg_idx]);
        free(app_argv);
    }

    return rc;
}

// INFO(zhiyan): https://www.redhat.com/archives/axp-list/2001-January/msg00355.html
typedef struct{
    int           pid;                      /** The process id. **/
    char          exName [_POSIX_PATH_MAX]; /** The filename of the executable **/
    char          state; /** 1 **/          /** R is running, S is sleeping,
			                                    D is sleeping in an uninterruptible wait,
			                                    Z is zombie, T is traced or stopped **/
    unsigned      euid,                      /** effective user id **/
                  egid;                      /** effective group id */
    int           ppid;                     /** The pid of the parent. **/
    int           pgrp;                     /** The pgrp of the process. **/
    int           session;                  /** The session id of the process. **/
    int           tty;                      /** The tty the process uses **/
    int           tpgid;                    /** (too long) **/
    unsigned int	flags;                    /** The flags of the process. **/
    unsigned int	minflt;                   /** The number of minor faults **/
    unsigned int	cminflt;                  /** The number of minor faults with childs **/
    unsigned int	majflt;                   /** The number of major faults **/
    unsigned int  cmajflt;                  /** The number of major faults with childs **/
    int           utime;                    /** user mode jiffies **/
    int           stime;                    /** kernel mode jiffies **/
    int		      cutime;                   /** user mode jiffies with childs **/
    int           cstime;                   /** kernel mode jiffies with childs **/
    int           counter;                  /** process's next timeslice **/
    int           priority;                 /** the standard nice value, plus fifteen **/
    unsigned int  timeout;                  /** The time in jiffies of the next timeout **/
    unsigned int  itrealvalue;              /** The time before the next SIGALRM is sent to the process **/
    int           starttime; /** 20 **/     /** Time the process started after system boot **/
    unsigned int  vsize;                    /** Virtual memory size **/
    unsigned int  rss;                      /** Resident Set Size **/
    unsigned int  rlim;                     /** Current limit in bytes on the rss **/
    unsigned int  startcode;                /** The address above which program text can run **/
    unsigned int  endcode;                  /** The address below which program text can run **/
    unsigned int  startstack;               /** The address of the start of the stack **/
    unsigned int  kstkesp;                  /** The current value of ESP **/
    unsigned int  kstkeip;                  /** The current value of EIP **/
    int		      signal;                   /** The bitmap of pending signals **/
    int           blocked; /** 30 **/       /** The bitmap of blocked signals **/
    int           sigignore;                /** The bitmap of ignored signals **/
    int           sigcatch;                 /** The bitmap of catched signals **/
    unsigned int  wchan;  /** 33 **/        /** (too long) **/
    int		      sched, 		            /** scheduler **/
                  sched_priority;		    /** scheduler priority **/
} proc_pid_stat;

// TODO(production): add more indicators for different resources, like runc did. leveraging libgtop liks a good way.
int app_events(char *app_name) {
    char app_process_pid_path_v[PATH_MAX + 1], *process_pid_s,
        stat_file[PATH_MAX + 1],
        *stat_report_fmt = "{\"type\": \"stats\", \"id\": \"%s\", \"data\": {\"cpu\": {\"usage\": {\"total\": %d}}}}\n";
    int process_pid_l, rc;
    FILE *fp;
    proc_pid_stat pinfo;

    rc = app_process_pid_path(app_process_pid_path_v, PATH_MAX + 1, app_name);
    if (-1 == rc) {
        IOT_ERROR("failed to get application process pid file path: %d", rc);
        return rc;
    }

    process_pid_s = read_str_file(app_process_pid_path_v, &process_pid_l);
    if (NULL == process_pid_s) {
        IOT_ERROR("failed to get application process pid from %s", app_process_pid_path_v);
        return 1;
    }

    snprintf(stat_file, PATH_MAX + 1, "/proc/%d/stat", atoi(process_pid_s));
    free(process_pid_s);

    while (1) {
        fp = fopen(stat_file, "r");
        if (fp == NULL)
            goto end;

        fscanf(fp, "%d %s %c %d %d %d %d %d %u %u %u %u %u %d %d %d %d %d %d %u %u %d %u %u %u %u %u %u %u %u %d %d %d %d %u",
                &pinfo.pid,
                (char*)&pinfo.exName,
                &pinfo.state,
                &pinfo.ppid,
                &pinfo.pgrp,
                &pinfo.session,
                &pinfo.tty,
                &pinfo.tpgid,
                &pinfo.flags,
                &pinfo.minflt,
                &pinfo.cminflt,
                &pinfo.majflt,
                &pinfo.cmajflt,
                &pinfo.utime,
                &pinfo.stime,
                &pinfo.cutime,
                &pinfo.cstime,
                &pinfo.counter,
                &pinfo.priority,
                &pinfo.timeout,
                &pinfo.itrealvalue,
                &pinfo.starttime,
                &pinfo.vsize,
                &pinfo.rss,
                &pinfo.rlim,
                &pinfo.startcode,
                &pinfo.endcode,
                &pinfo.startstack,
                &pinfo.kstkesp,
                &pinfo.kstkeip,
                &pinfo.signal,
                &pinfo.blocked,
                &pinfo.sigignore,
                &pinfo.sigcatch,
                &pinfo.wchan);
        fclose (fp);

        printf(stat_report_fmt, app_name, pinfo.utime + pinfo.stime + pinfo.cutime + pinfo.cstime);

        sleep(1);
    }

end:
    return 0;
}

int app_state(char *app_name) {
    char app_process_pid_path_v[PATH_MAX + 1], *process_pid_s;
    int process_pid_l, process_pid, rc;

    rc = app_process_pid_path(app_process_pid_path_v, PATH_MAX + 1, app_name);
    if (-1 == rc) {
        IOT_ERROR("failed to get application process pid file path: %d", rc);
        return rc;
    }

    process_pid_s = read_str_file(app_process_pid_path_v, &process_pid_l);
    if (NULL == process_pid_s) {
        IOT_ERROR("failed to get application process pid from %s", app_process_pid_path_v);
        return 1;
    }

    process_pid = atoi(process_pid_s);
    free(process_pid_s);

    rc = kill(process_pid, 0);
    if (0 == rc)
        return 0;  // application process is existing
    else if (ESRCH == errno)
        return 1;  // not existing
    else
        IOT_ERROR("failed to check application process state, pid = %d", process_pid)
        return 2;
}
