/*
 * Copyright (C) 2013 The Android Open Source Project
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

/*
 * Linux task stats reporting tool. Queries and prints out the kernel's
 * taskstats structure for a given process or thread group id. See
 * https://www.kernel.org/doc/Documentation/accounting/ for more information
 * about the reported fields.
 */

#include <errno.h>
#include <getopt.h>
#include <netlink/attr.h>
#include <netlink/genl/genl.h>
#include <netlink/handlers.h>
#include <netlink/msg.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/cdefs.h>
#include <time.h>
#include <unistd.h>

#include <linux/taskstats.h>

struct TaskStatistics {
    int pid;
    int tgid;
    struct taskstats stats;
};

int send_command(struct nl_sock* netlink_socket, uint16_t nlmsg_type,
                 uint32_t nlmsg_pid, uint8_t genl_cmd, uint16_t nla_type,
                 void* nla_data, int nla_len) {
    struct nl_msg* message = nlmsg_alloc();
    int seq = 0;
    int version = 1;
    int header_length = 0;
    int flags = NLM_F_REQUEST;
    genlmsg_put(message, nlmsg_pid, seq, nlmsg_type, header_length, flags,
                genl_cmd, version);
    nla_put(message, nla_type, nla_len, nla_data);

    /* Override the header flags since we don't want NLM_F_ACK. */
    struct nlmsghdr* header = nlmsg_hdr(message);
    header->nlmsg_flags = flags;

    int result = nl_send(netlink_socket, message);
    nlmsg_free(message);
    return result;
}

int print_receive_error(struct sockaddr_nl* address __unused,
                        struct nlmsgerr* error, void* arg __unused) {
    fprintf(stderr, "Netlink receive error: %s\n", strerror(-error->error));
    return NL_STOP;
}

int parse_family_id(struct nl_msg* msg, void* arg) {
    struct genlmsghdr* gnlh = (struct genlmsghdr*)nlmsg_data(nlmsg_hdr(msg));
    struct nlattr* attr = genlmsg_attrdata(gnlh, 0);
    int remaining = genlmsg_attrlen(gnlh, 0);

    do {
        if (attr->nla_type == CTRL_ATTR_FAMILY_ID) {
            *((int*)arg) = nla_get_u16(attr);
            return NL_STOP;
        }
    } while ((attr = nla_next(attr, &remaining)));
    return NL_OK;
}

int get_family_id(struct nl_sock* netlink_socket, const char* name) {
    if (send_command(netlink_socket, GENL_ID_CTRL, getpid(),
                     CTRL_CMD_GETFAMILY,
                     CTRL_ATTR_FAMILY_NAME,
                     (void*)name, strlen(name) + 1) < 0) {
        return 0;
    }

    int family_id = 0;
    struct nl_cb* callbacks = nl_cb_get(nl_cb_alloc(NL_CB_VALID));
    nl_cb_set(callbacks, NL_CB_VALID, NL_CB_DEFAULT, &parse_family_id,
              &family_id);
    nl_cb_err(callbacks, NL_CB_DEFAULT, &print_receive_error, NULL);

    if (nl_recvmsgs(netlink_socket, callbacks) < 0) {
        return 0;
    }
    nl_cb_put(callbacks);
    return family_id;
}

void parse_aggregate_task_stats(struct nlattr* attr, int attr_size,
                                struct TaskStatistics* stats) {
    do {
        switch (attr->nla_type) {
            case TASKSTATS_TYPE_PID:
                stats->pid = nla_get_u32(attr);
                break;
            case TASKSTATS_TYPE_TGID:
                stats->tgid = nla_get_u32(attr);
                break;
            case TASKSTATS_TYPE_STATS:
                nla_memcpy(&stats->stats, attr, sizeof(stats->stats));
                break;
            default:
                break;
        }
    } while ((attr = nla_next(attr, &attr_size)));
}

int parse_task_stats(struct nl_msg* msg, void* arg) {
    struct TaskStatistics* stats = (struct TaskStatistics*)arg;
    struct genlmsghdr* gnlh = (struct genlmsghdr*)nlmsg_data(nlmsg_hdr(msg));
    struct nlattr* attr = genlmsg_attrdata(gnlh, 0);
    int remaining = genlmsg_attrlen(gnlh, 0);

    do {
        switch (attr->nla_type) {
            case TASKSTATS_TYPE_AGGR_PID:
            case TASKSTATS_TYPE_AGGR_TGID:
                parse_aggregate_task_stats(nla_data(attr), nla_len(attr),
                                           stats);
                break;
            default:
                break;
        }
    } while ((attr = nla_next(attr, &remaining)));
    return NL_STOP;
}

int query_task_stats(struct nl_sock* netlink_socket, int family_id,
                     int command_type, int parameter,
                     struct TaskStatistics* stats) {
    memset(stats, 0, sizeof(*stats));
    int result = send_command(netlink_socket, family_id, getpid(),
                              TASKSTATS_CMD_GET, command_type, &parameter,
                              sizeof(parameter));
    if (result < 0) {
        return result;
    }

    struct nl_cb* callbacks = nl_cb_get(nl_cb_alloc(NL_CB_VALID));
    nl_cb_set(callbacks, NL_CB_VALID, NL_CB_DEFAULT, &parse_task_stats, stats);
    nl_cb_err(callbacks, NL_CB_DEFAULT, &print_receive_error, &family_id);

    result = nl_recvmsgs(netlink_socket, callbacks);
    if (result < 0) {
        return result;
    }
    nl_cb_put(callbacks);
    return stats->pid || stats->tgid;
}

double average_ms(unsigned long long total, unsigned long long count) {
    if (!count) {
        return 0;
    }
    return ((double)total) / count / 1e6;
}

unsigned long long average_ns(unsigned long long total,
                              unsigned long long count) {
    if (!count) {
        return 0;
    }
    return total / count;
}

void print_task_stats(const struct TaskStatistics* stats,
                      int human_readable) {
    const struct taskstats* s = &stats->stats;
    printf("Basic task statistics\n");
    printf("---------------------\n");
    printf("%-25s%d\n", "Stats version:", s->version);
    printf("%-25s%d\n", "Exit code:", s->ac_exitcode);
    printf("%-25s0x%x\n", "Flags:", s->ac_flag);
    printf("%-25s%d\n", "Nice value:", s->ac_nice);
    printf("%-25s%s\n", "Command name:", s->ac_comm);
    printf("%-25s%d\n", "Scheduling discipline:", s->ac_sched);
    printf("%-25s%d\n", "UID:", s->ac_uid);
    printf("%-25s%d\n", "GID:", s->ac_gid);
    printf("%-25s%d\n", "PID:", s->ac_pid);
    printf("%-25s%d\n", "PPID:", s->ac_ppid);

    if (human_readable) {
        time_t begin_time = s->ac_btime;
        printf("%-25s%s", "Begin time:", ctime(&begin_time));
    } else {
        printf("%-25s%d sec\n", "Begin time:", s->ac_btime);
    }
    printf("%-25s%llu usec\n", "Elapsed time:", s->ac_etime);
    printf("%-25s%llu usec\n", "User CPU time:", s->ac_utime);
    printf("%-25s%llu\n", "Minor page faults:", s->ac_minflt);
    printf("%-25s%llu\n", "Major page faults:", s->ac_majflt);
    printf("%-25s%llu usec\n", "Scaled user time:", s->ac_utimescaled);
    printf("%-25s%llu usec\n", "Scaled system time:", s->ac_stimescaled);

    printf("\nDelay accounting\n");
    printf("----------------\n");
    printf("       %15s%15s%15s%15s%15s%15s\n",
           "Count",
           human_readable ? "Delay (ms)" : "Delay (ns)",
           "Average delay",
           "Real delay",
           "Scaled real",
           "Virtual delay");

    if (!human_readable) {
        printf("CPU    %15llu%15llu%15llu%15llu%15llu%15llu\n",
               s->cpu_count,
               s->cpu_delay_total,
               average_ns(s->cpu_delay_total, s->cpu_count),
               s->cpu_run_real_total,
               s->cpu_scaled_run_real_total,
               s->cpu_run_virtual_total);
        printf("IO     %15llu%15llu%15llu\n",
               s->blkio_count,
               s->blkio_delay_total,
               average_ns(s->blkio_delay_total, s->blkio_count));
        printf("Swap   %15llu%15llu%15llu\n",
               s->swapin_count,
               s->swapin_delay_total,
               average_ns(s->swapin_delay_total, s->swapin_count));
        printf("Reclaim%15llu%15llu%15llu\n",
               s->freepages_count,
               s->freepages_delay_total,
               average_ns(s->freepages_delay_total, s->freepages_count));
    } else {
        const double ms_per_ns = 1e6;
        printf("CPU    %15llu%15.3f%15.3f%15.3f%15.3f%15.3f\n",
               s->cpu_count,
               s->cpu_delay_total / ms_per_ns,
               average_ms(s->cpu_delay_total, s->cpu_count),
               s->cpu_run_real_total / ms_per_ns,
               s->cpu_scaled_run_real_total / ms_per_ns,
               s->cpu_run_virtual_total / ms_per_ns);
        printf("IO     %15llu%15.3f%15.3f\n",
               s->blkio_count,
               s->blkio_delay_total / ms_per_ns,
               average_ms(s->blkio_delay_total, s->blkio_count));
        printf("Swap   %15llu%15.3f%15.3f\n",
               s->swapin_count,
               s->swapin_delay_total / ms_per_ns,
               average_ms(s->swapin_delay_total, s->swapin_count));
        printf("Reclaim%15llu%15.3f%15.3f\n",
               s->freepages_count,
               s->freepages_delay_total / ms_per_ns,
               average_ms(s->freepages_delay_total, s->freepages_count));
    }

    printf("\nExtended accounting fields\n");
    printf("--------------------------\n");
    if (human_readable && s->ac_stime) {
        printf("%-25s%.3f MB\n", "Average RSS usage:",
               (double)s->coremem / s->ac_stime);
        printf("%-25s%.3f MB\n", "Average VM usage:",
               (double)s->virtmem / s->ac_stime);
    } else {
        printf("%-25s%llu MB\n", "Accumulated RSS usage:", s->coremem);
        printf("%-25s%llu MB\n", "Accumulated VM usage:", s->virtmem);
    }
    printf("%-25s%llu KB\n", "RSS high water mark:", s->hiwater_rss);
    printf("%-25s%llu KB\n", "VM high water mark:", s->hiwater_vm);
    printf("%-25s%llu\n", "IO bytes read:", s->read_char);
    printf("%-25s%llu\n", "IO bytes written:", s->write_char);
    printf("%-25s%llu\n", "IO read syscalls:", s->read_syscalls);
    printf("%-25s%llu\n", "IO write syscalls:", s->write_syscalls);

    printf("\nPer-task/thread statistics\n");
    printf("--------------------------\n");
    printf("%-25s%llu\n", "Voluntary switches:", s->nvcsw);
    printf("%-25s%llu\n", "Involuntary switches:", s->nivcsw);
}

void print_usage() {
  printf("Linux task stats reporting tool\n"
         "\n"
         "Usage: taskstats [options]\n"
         "\n"
         "Options:\n"
         "  --help        This text\n"
         "  --pid PID     Print stats for the process id PID\n"
         "  --tgid TGID   Print stats for the thread group id TGID\n"
         "  --raw         Print raw numbers instead of human readable units\n"
         "\n"
         "Either PID or TGID must be specified. For more documentation about "
         "the reported fields, see\n"
         "https://www.kernel.org/doc/Documentation/accounting/"
         "taskstats-struct.txt\n");
}

int main(int argc, char** argv) {
    int command_type = 0;
    int pid = 0;
    int human_readable = 1;

    const struct option long_options[] = {
        {"help", no_argument, 0, 0},
        {"pid", required_argument, 0, 0},
        {"tgid", required_argument, 0, 0},
        {"raw", no_argument, 0, 0},
        {0, 0, 0, 0}
    };

    while (1) {
        int option_index;
        int option_char = getopt_long_only(argc, argv, "", long_options,
                                           &option_index);
        if (option_char == -1) {
            break;
        }
        switch (option_index) {
            case 0:
                print_usage();
                return EXIT_SUCCESS;
            case 1:
                command_type = TASKSTATS_CMD_ATTR_PID;
                pid = atoi(optarg);
                break;
            case 2:
                command_type = TASKSTATS_CMD_ATTR_TGID;
                pid = atoi(optarg);
                break;
            case 3:
                human_readable = 0;
                break;
            default:
                break;
        };
    }

    if (!pid) {
        printf("Either PID or TGID must be specified\n");
        return EXIT_FAILURE;
    }

    struct nl_sock* netlink_socket = nl_socket_alloc();
    if (!netlink_socket || genl_connect(netlink_socket) < 0) {
        perror("Unable to open netlink socket (are you root?)");
        goto error;
    }

    int family_id = get_family_id(netlink_socket, TASKSTATS_GENL_NAME);
    if (!family_id) {
        perror("Unable to determine taskstats family id "
               "(does your kernel support taskstats?)");
        goto error;
    }
    struct TaskStatistics stats;
    if (query_task_stats(netlink_socket, family_id, command_type, pid,
                         &stats) < 0) {
        perror("Failed to query taskstats");
        goto error;
    }
    print_task_stats(&stats, human_readable);

    nl_socket_free(netlink_socket);
    return EXIT_SUCCESS;

error:
    if (netlink_socket) {
        nl_socket_free(netlink_socket);
    }
    return EXIT_FAILURE;
}
