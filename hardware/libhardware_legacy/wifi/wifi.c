/*
 * Copyright 2008, The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <stdlib.h>
#include <fcntl.h>
#include <errno.h>
#include <string.h>
#include <dirent.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <unistd.h>
#include <poll.h>
#include <ctype.h>

//yuemodify 
// /home/abc/hdd1/android5_1_1/hardware/libhardware/include/hardware/qemud.h
// /home/abc/hdd1/android5_1_1/hardware/libhardware_legacy/wifi/wifi.c
// #include <hardware/qemud.h>
#include <sys/epoll.h>
#include "../../libhardware/include/hardware/qemud.h"
#define SET_ONCE_DATA 0
// void my_wifi_init();
#define  QEMU_CHANNEL_NAME  "wifi"
#define  READ_epoll_register 1
// static void * pthread(void *arg);
int epoll_register( int  epoll_fd, int  fd );

//yuemodify
// #define  WIFI_DEBUG  0
#if WIFI_DEBUG
#  define  WIFID(...)   ALOGD(__VA_ARGS__)
#else
#  define  WIFID(...)   ((void)0)
#endif

int g_wifi_fd =-1;
int g_wifi_fd1 =-1;

#include "hardware_legacy/wifi.h"
#ifdef LIBWPA_CLIENT_EXISTS
#include "libwpa_client/wpa_ctrl.h"
#endif


#define LOG_TAG "WifiHW"
#include "cutils/log.h"
#include "cutils/memory.h"
#include "cutils/misc.h"
#include "cutils/properties.h"
#include "private/android_filesystem_config.h"

#define _REALLY_INCLUDE_SYS__SYSTEM_PROPERTIES_H_
#include <sys/_system_properties.h>

extern int do_dhcp();
extern int ifc_init();
extern void ifc_close();
extern char *dhcp_lasterror();
extern void get_dhcp_info();
extern int init_module(void *, unsigned long, const char *);
extern int delete_module(const char *, unsigned int);
void wifi_close_sockets();

#ifndef LIBWPA_CLIENT_EXISTS
#define WPA_EVENT_TERMINATING "CTRL-EVENT-TERMINATING "
struct wpa_ctrl
{
};
void wpa_ctrl_cleanup(void) {}
struct wpa_ctrl *wpa_ctrl_open(const char *ctrl_path) { return NULL; }
void wpa_ctrl_close(struct wpa_ctrl *ctrl) {}
int wpa_ctrl_request(struct wpa_ctrl *ctrl, const char *cmd, size_t cmd_len,
                     char *reply, size_t *reply_len, void (*msg_cb)(char *msg, size_t len))
{
    return 0;
}
int wpa_ctrl_attach(struct wpa_ctrl *ctrl) { return 0; }
int wpa_ctrl_detach(struct wpa_ctrl *ctrl) { return 0; }
int wpa_ctrl_recv(struct wpa_ctrl *ctrl, char *reply, size_t *reply_len)
{
    return 0;
}
int wpa_ctrl_get_fd(struct wpa_ctrl *ctrl) { return 0; }
#endif

static struct wpa_ctrl *ctrl_conn;
static struct wpa_ctrl *monitor_conn;

/* socket pair used to exit from a blocking read */
static int exit_sockets[2];

static char primary_iface[PROPERTY_VALUE_MAX];
// TODO: use new ANDROID_SOCKET mechanism, once support for multiple
// sockets is in

#ifndef WIFI_DRIVER_MODULE_ARG
#define WIFI_DRIVER_MODULE_ARG ""
#endif
#ifndef WIFI_FIRMWARE_LOADER
#define WIFI_FIRMWARE_LOADER ""
#endif
#define WIFI_TEST_INTERFACE "sta"

#ifndef WIFI_DRIVER_FW_PATH_STA
#define WIFI_DRIVER_FW_PATH_STA NULL
#endif
#ifndef WIFI_DRIVER_FW_PATH_AP
#define WIFI_DRIVER_FW_PATH_AP NULL
#endif
#ifndef WIFI_DRIVER_FW_PATH_P2P
#define WIFI_DRIVER_FW_PATH_P2P NULL
#endif

#ifndef WIFI_DRIVER_FW_PATH_PARAM
#define WIFI_DRIVER_FW_PATH_PARAM "/sys/module/wlan/parameters/fwpath"
#endif

#define WIFI_DRIVER_LOADER_DELAY 1000000
#define SYSFS_PATH_MAX 256

static const char IFACE_DIR[] = "/data/system/wpa_supplicant";
#ifdef WIFI_DRIVER_MODULE_PATH
static const char DRIVER_NAME_PROP[] = "wlan.modname";
static const char DRIVER_PATH_PROP[] = "wlan.modpath";
static const char DRIVER_MODULE_ARG[] = WIFI_DRIVER_MODULE_ARG;
#endif
static const char FIRMWARE_LOADER[] = WIFI_FIRMWARE_LOADER;
static const char DRIVER_PROP_NAME[] = "wlan.driver.status";
static const char SUPPLICANT_NAME[] = "wpa_supplicant";
static const char SUPP_PROP_NAME[] = "init.svc.wpa_supplicant";
static const char P2P_SUPPLICANT_NAME[] = "p2p_supplicant";
static const char P2P_PROP_NAME[] = "init.svc.p2p_supplicant";
static const char SUPP_CONFIG_TEMPLATE[] = "/system/etc/wifi/wpa_supplicant.conf";
static const char SUPP_CONFIG_FILE[] = "/data/misc/wifi/wpa_supplicant.conf";
static const char P2P_CONFIG_FILE[] = "/data/misc/wifi/p2p_supplicant.conf";
static const char CONTROL_IFACE_PATH[] = "/data/misc/wifi/sockets";
static const char MODULE_FILE[] = "/proc/modules";
static const char SYSFS_CLASS_NET[] = "/sys/class/net";
static const char MODULE_DEFAULT_DIR[] = "/system/lib/modules";
static const char SYS_MOD_NAME_DIR[] = "device/driver/module";

static const char IFNAME[] = "IFNAME=";
#define IFNAMELEN (sizeof(IFNAME) - 1)
static const char WPA_EVENT_IGNORE[] = "CTRL-EVENT-IGNORE ";

static const char SUPP_ENTROPY_FILE[] = WIFI_ENTROPY_FILE;
static unsigned char dummy_key[21] = {0x02, 0x11, 0xbe, 0x33, 0x43, 0x35,
                                      0x68, 0x47, 0x84, 0x99, 0xa9, 0x2b,
                                      0x1c, 0xd3, 0xee, 0xff, 0xf1, 0xe2,
                                      0xf3, 0xf4, 0xf5};

/* Is either SUPPLICANT_NAME or P2P_SUPPLICANT_NAME */
static char supplicant_name[PROPERTY_VALUE_MAX];
/* Is either SUPP_PROP_NAME or P2P_PROP_NAME */
static char supplicant_prop_name[PROPERTY_KEY_MAX];

int has_wifi_driver_loaded = 0;
int is_wifi_supplicant_connection_active = 0;
int is_ctrl_event_connected = 0;
int is_wifi_reconnect = 0;

static char default_mac[20] = "12:34:56:78:9a:bc";
static char default_ssid[20] = "WIFI";
static char default_ip[20] = "10.254.1.123";
/*111
rf.mac
rf.ssid
rf.ip
*/

#define ALOGE(...) do { } while (0)


static int sock_sv[2];


void mylog_write(const char* title, const char* fmt, ...) {
    int fd_mylog = open("/yuewifi.txt", O_WRONLY | O_APPEND);
    char buf[8192];
    if(fd_mylog < 0)
    {
        creat("/yuewifi.txt", 666);
        fd_mylog = open("/yuewifi.txt", O_WRONLY | O_APPEND);
    }
    int len = snprintf(buf, 8192, "[%s]: ", title);

    va_list ap;

    va_start(ap, fmt);

    len += vsnprintf(buf + len, 8192, fmt, ap);
    buf[8192 - 1] = 0;

    va_end(ap);
    buf[len] = '\n';
    len++;

    write(fd_mylog, buf, len);
    close(fd_mylog);
}


int do_dhcp_request(int *ipaddr, int *gateway, int *mask,
                    int *dns1, int *dns2, int *server, int *lease)
{

    ALOGE("REDF-LOG do_dhcp_request\n");
    return 0;
}

const char *get_dhcp_error_string()
{
    ALOGE("REDF-LOG get_dhcp_error_string\n");
    return "not supported!";
}

int is_wifi_driver_loaded()
{
    return has_wifi_driver_loaded;
}

int wifi_load_driver()
{

    ALOGE("REDF-LOG wifi_load_driver\n");
    has_wifi_driver_loaded = 1;
    is_ctrl_event_connected = 1;

    if (sock_sv[0] > 0 || socketpair(AF_UNIX, SOCK_SEQPACKET, 0, sock_sv) >= 0)
    {
        property_set(DRIVER_PROP_NAME, "ok");
        return 0;
    }
    ALOGE("REDF-LOG wifi_load_driver Could not create socketpair %s\n", strerror(errno));
    return -1;
}

int wifi_unload_driver()
{
    ALOGE("REDF-LOG wifi unload driver\n");
    has_wifi_driver_loaded = 0;
    is_ctrl_event_connected = 0;
    is_wifi_reconnect = 0;
    property_set(DRIVER_PROP_NAME, "unloaded");

    return 0;
}

int ensure_entropy_file_exists()
{
    ALOGE("REDF-LOG ensure_entropy_file_exists\n");
    return -1;
}

int ensure_config_file_exists(const char *config_file)
{
    ALOGE("REDF-LOG ensure_config_file_exists, config_file: %s\n", config_file);
    return -1;
}

int wifi_start_supplicant(int p2p_supported)
{

    property_set("ctl.start", SUPPLICANT_NAME);
    is_wifi_supplicant_connection_active = 1;

    ALOGE("REDF-LOG wifi_start_supplicant, p2p_supported: %d\n", p2p_supported);

    return 0;
}

int wifi_stop_supplicant(int p2p_supported)
{
    is_wifi_supplicant_connection_active = 0;
    ALOGE("REDF-LOG wifi_stop_supplicant, p2p_supported: %d\n", p2p_supported);
    return 0;
}

int wifi_connect_on_socket_path(const char *path)
{
    ALOGE("REDF-LOG wifi_connect_on_socket_path, path: %s\n", path);
    return -1;
}

/* Establishes the control and monitor socket connections on the interface */
int wifi_connect_to_supplicant()
{
    ALOGE("REDF-LOG wifi_connect_to_supplicant\n");
    return 0;
}

int send_data(char *data)
{
    if (is_ctrl_event_connected)
    {
        int result = write(sock_sv[1], data, strlen(data) + 1);
        if (result >= 0)
        {
            ALOGE("REDF-LOG send data[%s]\n", data);    
            return result;
        }
        ALOGE("REDF-LOG send data error! %s\n", strerror(errno));
    }
    else
    {
        ALOGE("REDF-LOG send data error! current wap connect is TERMINATED\n");
    }
    return 0;
}


int wifi_send_command(const char *cmd, char *reply, size_t *reply_len)
{
    int epoll_fd = -1;
    //ALOGD("yuelog 0424  wifi.c wifi_send_command  accept command = %s\n",cmd);
    if (strstr(cmd, "wlan0 DRIVER "))
    {
        if (strstr(cmd, "wlan0 DRIVER MACADDR"))
        {
            char mac_address[PROPERTY_VALUE_MAX] = {0};
            property_get("vmprop.wifibssid", mac_address, default_mac);
            snprintf(reply, *reply_len, "Macaddr = %s", mac_address);
            goto finalret;
        }
        snprintf(reply, *reply_len, "OK");
        goto finalret;
    }
    else if (strstr(cmd, "wlan0 LIST_NETWORKS"))
    {
        if (strstr(cmd, "LAST_ID=-1"))
        {
            char ssid[PROPERTY_VALUE_MAX] = {0};
            property_get("vmprop.wifissid", ssid, default_ssid);
            snprintf(reply, *reply_len, "network id / ssid / bssid / flags\n0\t%s\tany", ssid);
            goto finalret;
        }
        else
        {
            snprintf(reply, *reply_len, "network id / ssid / bssid / flags");
            goto finalret;
        }
    }
    else if (strstr(cmd, "wlan0 GET_NETWORK"))
    {
        if (strstr(cmd, " ssid"))
        {
            char ssid[PROPERTY_VALUE_MAX] = {0};
            property_get("vmprop.wifissid", ssid, default_ssid);
            snprintf(reply, *reply_len, "\"%s\"", ssid);
            goto finalret;
        }
        else if (strstr(cmd, " bssid"))
        {
            char mac_address[PROPERTY_VALUE_MAX] = {0};
            property_get("vmprop.wifibssid", mac_address, default_mac);
            snprintf(reply, *reply_len, "%s", mac_address);
            goto finalret;
        }
        else if (strstr(cmd, "priority"))
        {
            snprintf(reply, *reply_len, "2");
            goto finalret;
        }
        else if (strstr(cmd, "sim_num"))
        {
            snprintf(reply, *reply_len, "1");
            goto finalret;
        }
        else if (strstr(cmd, "scan_ssid") || strstr(cmd, "mode") || strstr(cmd, "frequency") || strstr(cmd, "wep_tx_keyidx"))
        {
            snprintf(reply, *reply_len, "0");
            goto finalret;
        }
        else if (strstr(cmd, "proto"))
        {
            snprintf(reply, *reply_len, "WPA_RSN");
            goto finalret;
        }
        else if (strstr(cmd, "key_mgmt"))
        {
            snprintf(reply, *reply_len, "NONE");
            goto finalret;
        }
        if (strstr(cmd, "pairwise"))
        {
            snprintf(reply, *reply_len, "CCMP TKIP");
            goto finalret;
        }
        if (strstr(cmd, "group"))
        {
            snprintf(reply, *reply_len, "CCMP TKIP WEP104 WEP40");
            goto finalret;
        }
        if (strstr(cmd, "engine"))
        {
            snprintf(reply, *reply_len, "0");
            goto finalret;
        }
    }
    else
    {
        if (strstr(cmd, "wlan0 GET_CAPABILITY modes"))
        {
            snprintf(reply, *reply_len, "IBSS AP");
            goto finalret;
        }
        if (strstr(cmd, "wlan0 SCAN_INTERVAL") || strstr(cmd, "wlan0 STA_AUTOCONNECT"))
        {
            snprintf(reply, *reply_len, "OK");
            goto finalret;
        }
        if (!strcmp(cmd, "IFNAME=wlan0 STATUS") || strstr(cmd, "wlan0 STATUS-NO_EVENTS"))
        {
            if (is_wifi_reconnect)
            {
                char ssid[PROPERTY_VALUE_MAX] = {0};
                char mac_address[PROPERTY_VALUE_MAX] = {0};
                char ip_address[PROPERTY_VALUE_MAX] = {0};
                property_get("vmprop.wifibssid", mac_address, default_mac);
                property_get("vmprop.ip", ip_address, default_ip);
                property_get("vmprop.wifissid", ssid, default_ssid);
                snprintf(reply, *reply_len, "bssid=%s\nfreq=2412\nssid=%s\nid=0\nmode=station\npairwise_cipher=NONE\ngroup_cipher=NONE\nkey_mgmt=NONE\nwpa_state=COMPLETED\nip_address=%s", mac_address, ssid, ip_address);
                goto finalret;
            }
            snprintf(reply, *reply_len, "wpa_state=DISCONNECTED");
            goto finalret;
        }
        if (strstr(cmd, "wlan0 SCAN "))
        {
            snprintf(reply, *reply_len, "OK");
            send_data("IFNAME=wlan0 CTRL-EVENT-SCAN-STARTED");
            send_data("IFNAME=wlan0 CTRL-EVENT-SCAN-RESULTS");
            goto finalret;
        }
        if (strstr(cmd, "wlan0 BSS_FLUSH "))
        {
            snprintf(reply, *reply_len, "OK");
            goto finalret;
        }
        if (strstr(cmd, "wlan0 BSS RANGE=0"))
        {
            //IFNAME=wlan0 BSS RANGE=0- MASK=0x21987
            //ALOGD("yuelog 0424  wifi.c wifi_send_command  strstr(cmd, \"wlan0 BSS RANGE=0\")  cmd= %s\n",cmd);
            char ssid[PROPERTY_VALUE_MAX] = {0};
            char mac_address[PROPERTY_VALUE_MAX] = {0};
            char prop[92] = {0};
            bool wlanrunflag = false;
            property_get("vmprop.wifibssid", mac_address, default_mac);
            property_get("vmprop.wifissid", ssid, default_ssid);
            //old
            // ALOGD("yuelog mywifi strstr(cmd, wlan0 BSS RANGE=0");
            // snprintf(reply, *reply_len, "ie=\nid=0\nbssid=%s\nfreq=2437\nlevel=-41\ntsf=0000005475069290\nflags=[ESS]\nssid=%s\n####", mac_address, ssid);
            // goto finalret;

            //new
            // snprintf(reply, *reply_len, "ie=\nid=0\nbssid=%s\nfreq=2437\nlevel=-41\ntsf=0000005475069290\nflags=[ESS]\nssid=%s\n####", "00:0f:e2:74:a6:31", ssid);

           //new
            // snprintf(reply, *reply_len, "ie=\nid=0\nbssid=%s\nfreq=2437\nlevel=-41\ntsf=0000005475069290\nflags=[ESS]\nssid=%s\n####ie=\nid=0\nbssid=%s\nfreq=2437\nlevel=-41\ntsf=0000005475069290\nflags=[ESS]\nssid=%s\n####", "00:0f:e2:74:a6:31", ssid,"6c:e8:73:a6:d8:70","VMOSWIFI1");
#if SET_ONCE_DATA == 1
            WIFID("yuelog mywifi  will SET_ONCE_DATA  %d",*reply_len );
            snprintf(reply, *reply_len,
            "ie=\nid=0\nbssid=%s\nfreq=2437\nlevel=-41\ntsf=0000005475069290\nflags=[ESS]\nssid=%s\n####ie=\nid=0\nbssid=%s\nfreq=2437\nlevel=-41\ntsf=0000005475069290\nflags=[ESS]\nssid=%s\n####ie=\nid=0\nbssid=%s\nfreq=2437\nlevel=-41\ntsf=0000005475069290\nflags=[ESS]\nssid=%s\n####ie=\nid=0\nbssid=%s\nfreq=2437\nlevel=-41\ntsf=0000005475069290\nflags=[ESS]\nssid=%s\n####ie=\nid=0\nbssid=%s\nfreq=2437\nlevel=-41\ntsf=0000005475069290\nflags=[ESS]\nssid=%s\n####", 
            "d4:2c:44:e9:bd:62", "VMOSWIFI",
            "94:d4:69:fb:68:e1","VMOSWIFI1050",
            "18:8b:9d:dd:c5:a3","VMOSWIFI1051",
            "f0:3e:90:db:98:98","VMOSWIFI1052",
            "f0:3e:90:9b:96:d8","VMOSWIFI1053");

            // int i = 0;
            // for(i = 0;i<*reply_len;i++){
            //     ALOGD("yuelog mywifi  will SET_ONCE_DATA  reply[%d]=%c",i,reply[i]);
            // }
#else //socket接收数据
    //socket connect

    WIFID("yuelog0430 so wifihal0505   socket connect" );
    if(g_wifi_fd<0)
    {
        if (property_get("vmpro.wifi", prop, "") != 0) {
            wlanrunflag = prop[0] == '1' ? true : false;
        }

        if (wlanrunflag){
            ALOGE("yuelog mywifi   socket connect qemud_channel_open 1" );
            g_wifi_fd = qemud_channel_open(QEMU_CHANNEL_NAME);
        }else{
            g_wifi_fd = -1;
            snprintf(reply, *reply_len, "ie=\nid=0\nbssid=%s\nfreq=2437\nlevel=-41\ntsf=0000005475069290\nflags=[ESS]\nssid=%s\n####", mac_address, ssid);
            goto finalret;
        }  
    }

    if (g_wifi_fd < 0) {
        ALOGE("yuelog wifihal0505 %s qemud_channel_open create socket error  or  cant connect \n",QEMU_CHANNEL_NAME);
        //return 0;
        ErrorSendData(reply,reply_len,ssid);
        goto finalret;
    }

    WIFID("yuelog wifihal0505   read data from '%s' qemud channel", QEMU_CHANNEL_NAME );

    //发送命令
    epoll_fd   = epoll_create(2);
    //char  buff[256] = "CMD_START";//获取rfbox的固定数据     "CMD_START"  去rfbox jni中获取数据   "CMD_MY_WIFI_DATA" 去真机上获取数据
    char  buff[256] = "CMD_MY_WIFI_DATA";
    int ret;
    int total_len;
    int read_bytes = 0;
    //发送命令 CMD_LOCATION_DATA
    ret = TEMP_FAILURE_RETRY(write(g_wifi_fd, buff, strlen(buff)+1));
    if (ret != strlen(buff)+1) {
        ALOGE("yuelog wifihal0505 %s: Could not write CMD_LOCATION_DATA,error = %s", strerror(errno));
        if (ret == 0) {
            errno = ECONNRESET;
        } else if (ret > 0) {
            errno = EINVAL;
        }
        ErrorSendData(reply,reply_len,ssid);
        goto finalret;
    }

    WIFID("yuelog wifihal0505  send cmd = %s",buff );
    //ErrorSendData(reply,reply_len,ssid); //yes

    

    char Mybuff[4096]={"\0"};   

    #if READ_epoll_register ==1
        //接收数据
        epoll_register( epoll_fd, g_wifi_fd);
        int i = 0;
        for (;;) {//for1
                WIFID("yuelog wifihal0505   for1 ,count = %d",i++);
                struct epoll_event   events[1];
                int                  ne, nevents;

                nevents = epoll_wait( epoll_fd, events, 1, -1 ); //等一个?
                if (nevents < 0) {
                    ALOGE("yuelog wifihal0505 epoll_wait() unexpected error1: %s", strerror(errno));
                    if (errno != EINTR)
                        ALOGE("yuelog wifihal0505 epoll_wait() unexpected error: %s", strerror(errno));
                    continue;
                }
                WIFID("yuelog wifihal0505  thread received %d events", nevents);
                for (ne = 0; ne < nevents; ne++) {
                    WIFID("yuelog wifihal0505  1 for2");
                    if ((events[ne].events & (EPOLLERR|EPOLLHUP)) != 0) {
                        ALOGE("yuelog wifihal0505 EPOLLERR or EPOLLHUP after epoll_wait() !?");
                        //goto finalret;//应该发送一个固定的数据
                        ErrorSendData(reply,reply_len,ssid);
                        goto finalret;
                    }
                    if ((events[ne].events & EPOLLIN) != 0) {
                        WIFID("yuelog wifihal0505   EPOLLIN");
                        int  fd = events[ne].data.fd;
                        if (fd == g_wifi_fd){
                            WIFID("yuelog wifihal0505    g_wifi_fd");
                            char  buff[8192] = {"\0"};//modify  应该收更多
                            WIFID("yuelog wifihal0505  fd event");
                            int nconut = 0;
                            for (;;) {
                                WIFID("yuelog wifihal0505   for3");
                                int  nn, ret;

                                ret = read( fd, buff, sizeof(buff) );
                                if (ret < 0) {
                                    if (errno == EINTR)
                                        continue;
                                    if (errno != EWOULDBLOCK)
                                        ALOGE("yuelog wifihal0505 error while reading from mywifi  daemon socket: %s:", strerror(errno));
                                    break;
                                }

                                //处理接收的buff 数据
                                WIFID("yuelog0428 wifihal0505 处理接收的buff 数据 ret = %d",ret);

                                if (strstr(buff,"CantGetWifi")!=NULL || ret ==11) {
                                    /* code */
                                    WIFID("yuelog wifihal0505 处理接收的buff 数据  CantGetWifi ");
                                    snprintf(reply, *reply_len, "ie=\nid=0\nbssid=%s\nfreq=2437\nlevel=-41\ntsf=0000005475069290\nflags=[ESS]\nssid=%s\n####", mac_address, ssid);
                                    goto finalret;
                                }
                                
            
                                //如果一次就能收完,收到一次就足够了
                                //应该跳出最大的循环
                                //ErrorSendData(reply,reply_len,ssid); //yes 暂时这样

                                //ie=\nid=0\nbssid=%s\nfreq=2437\nlevel=-41\ntsf=0000005475069290\nflags=[ESS]\nssid=%s\n####
                                //ie=\nid=0\nbssid=d4:2c:44:e9:bd:62\nfreq=2437\nlevel=-41\ntsf=0000005475069290\nflags=[ESS]\nssid=VMOSWIFI\n####
                                WIFID("yuelog wifihal0505  mywifi  222222 *reply =%d   ret = %d  ret+117-8 = %d",*reply,ret,ret+117-8);    
                                snprintf(reply, ret+117-8, "ie=\nid=0\nbssid=%s\nfreq=2437\nlevel=-41\ntsf=0000005475069290\nflags=[ESS]\nssid=%s\n####%s","12:34:56:78:9a:bc",ssid,buff);
                                goto finalret;


                            }//for (;;) {
                            WIFID("yuelog wifihal0505 fd event end");
                        }else{
                            WIFID("yuelog wifihal0505  epoll_wait() returned unkown fd %d ?", fd);
                    }
                }//if ((events[ne].events & EPOLLIN) != 0)
            }//for (ne = 0; ne < nevents; ne++)
        }//for (;;) {
    #else //
        snprintf(reply, *reply_len, "ie=\nid=0\nbssid=%s\nfreq=2437\nlevel=-41\ntsf=0000005475069290\nflags=[ESS]\nssid=%s\n####", mac_address, ssid);
       if(1)
       {
            WIFID("yuelog mywifi   CMD_MY_WIFI_DATA 1" );
            while (1) 
            {

            WIFID("yuelog mywifi   CMD_MY_WIFI_DATA 2" );
                int read_size = read(g_wifi_fd, ((char*)&total_len) + read_bytes, 4 - read_bytes);
                if (read_size < 0 && errno != EINTR) {
                    read_bytes = 0; 
                    //

            WIFID("yuelog mywifi   CMD_MY_WIFI_DATA 3" );
                    //close(g_wifi_fd);
                    ErrorSendData(reply,reply_len,ssid);
                    goto finalret;
                    //return ret;
                }
                read_bytes += read_size;
                if (read_bytes == 4)
                    break;
            }

            WIFID("yuelog mywifi   CMD_MY_WIFI_DATA 4 %d",total_len );
            read_bytes = 0;
            while (1) 
            {

            WIFID("yuelog mywifi   CMD_MY_WIFI_DATA 5" );
                int read_size = read(g_wifi_fd, Mybuff + read_bytes, total_len - read_bytes);
                if (read_size < 0 && errno != EINTR) 
                {

            WIFID("yuelog mywifi   CMD_MY_WIFI_DATA 6" );
                    //close(g_wifi_fd);
                    ErrorSendData(reply,reply_len,ssid);
                    goto finalret;
                }
                read_bytes += read_size;
                if (read_bytes == total_len)
                    break;
            }
            int i = 0;
            WIFID("yuelog mywifi   Mybuff size  total_len= %d   read_bytes = %d",total_len,read_bytes); 
            for(i= 0;i<total_len;i++){
               WIFID("yuelog mywifi   Mybuff[%d]=%c",i,Mybuff[i] );     
            }
            


       }

    #endif
#endif
            goto finalret;

        }
        if (strstr(cmd, "wlan0 SET_NETWORK ") || strstr(cmd, "wlan0 SAVE_CONFIG"))
        {
            snprintf(reply, *reply_len, "OK");
            goto finalret;
        }
        if (strstr(cmd, "wlan0 ENABLE_NETWORK "))
        {
            char databuf[0x100] = {0};
            char ssid[PROPERTY_VALUE_MAX] = {0};
            char mac_address[PROPERTY_VALUE_MAX] = {0};
            property_get("vmprop.wifibssid", mac_address, default_mac);
            property_get("vmprop.wifissid", ssid, default_ssid);
            snprintf(reply, *reply_len, "OK");
            snprintf(databuf, sizeof(databuf), "IFNAME=wlan0 Trying to associate with %s (SSID='%s' freq=2437 MHz)", mac_address, ssid);
            send_data(databuf);
            goto finalret;
        }
        if (strstr(cmd, "wlan0 SELECT_NETWORK "))
        {
            snprintf(reply, *reply_len, "OK");
            goto finalret;
        }
        if (strstr(cmd, "wlan0 RECONNECT"))
        {
            char databuf[0x100] = {0};
            char ssid[PROPERTY_VALUE_MAX] = {0};
            char mac_address[PROPERTY_VALUE_MAX] = {0};
            property_get("vmprop.wifibssid", mac_address, default_mac);
            property_get("vmprop.wifissid", ssid, default_ssid);
            snprintf(databuf, sizeof(databuf), "IFNAME=wlan0 CTRL-EVENT-STATE-CHANGE id=0 state=5 BSSID=%s SSID=%s", mac_address, ssid);
            send_data(databuf);
            snprintf(databuf, sizeof(databuf), "IFNAME=wlan0 CTRL-EVENT-STATE-CHANGE id=0 state=6 BSSID=%s SSID=%s", mac_address, ssid);
            send_data(databuf);
            snprintf(databuf, sizeof(databuf), "IFNAME=wlan0 Associated with %s", mac_address);
            send_data(databuf);
            snprintf(databuf, sizeof(databuf), "IFNAME=wlan0 WPA: Key negotiation completed with %s [PTK=CCMP GTK=CCMP]", mac_address);
            send_data(databuf);
            snprintf(databuf, sizeof(databuf), "IFNAME=wlan0 CTRL-EVENT-CONNECTED - Connection to %s completed [id=0 id_str=]", mac_address);
            send_data(databuf);
            snprintf(databuf, sizeof(databuf), "IFNAME=wlan0 CTRL-EVENT-STATE-CHANGE id=0 state=9 BSSID=%s SSID=%s", mac_address, ssid);
            send_data(databuf);
            is_wifi_reconnect = 1;
            snprintf(reply, *reply_len, "OK");
            goto finalret;
        }
        if (strstr(cmd, "wlan0 SIGNAL_POLL"))
        {
            snprintf(reply, *reply_len, "RSSI=-45\nLINKSPEED=72\nNOISE=9999\nFREQUENCY=2437");
            goto finalret;
        }
        if (strstr(cmd, "wlan0 BLACKLIST clear") || strstr(cmd, "wlan0 SET ")) {
            snprintf(reply, *reply_len, "OK");
            goto finalret;
        }
        if (!strcmp(cmd, "IFNAME=wlan0 TERMINATE"))
        {
            char databuf[0x100] = {0};
            char ssid[PROPERTY_VALUE_MAX] = {0};
            char mac_address[PROPERTY_VALUE_MAX] = {0};
            property_get("vmprop.wifibssid", mac_address, default_mac);
            property_get("vmprop.wifissid", ssid, default_ssid);
            snprintf(databuf, sizeof(databuf), "IFNAME=wlan0 CTRL-EVENT-DISCONNECTED bssid=%s reason=3 locally_generated=1", mac_address);
            send_data(databuf);
            snprintf(databuf, sizeof(databuf), "IFNAME=wlan0 CTRL-EVENT-STATE-CHANGE id=0 state=0 BSSID=%s SSID=%s", mac_address, ssid);
            send_data(databuf);
            snprintf(databuf, sizeof(databuf), "IFNAME=wlan0 CTRL-EVENT-BSS-REMOVED 0 %s", mac_address);
            send_data(databuf);
            send_data("IFNAME=wlan0 CTRL-EVENT-TERMINATING");
            snprintf(reply, *reply_len, "OK");
            goto finalret;
        }
    }
    reply_len = 0;
    ALOGE("REDF-LOG wifi_send_command unknown, cmd[%s]", cmd);
    return -1;
finalret:
    if (epoll_fd != -1)
        close(epoll_fd);
    *reply_len = strlen(reply);
    //mylog_write(__FUNCTION__,"yuelog wifilast0505 REDF-LOG wifi_send_command, cmd[%s], reply[%s]", cmd, reply);
    WIFID("wifilast0505 REDF-LOG wifi_send_command, cmd[%s], reply[%s]", cmd, reply);
    return 0;
}

int epoll_register( int  epoll_fd, int  fd )
{
    struct epoll_event  ev;
    int                 ret, flags;

    /* important: make the fd non-blocking */
    flags = fcntl(fd, F_GETFL);
    fcntl(fd, F_SETFL, flags | O_NONBLOCK);

    ev.events  = EPOLLIN;
    ev.data.fd = fd;
    do {
        ret = epoll_ctl( epoll_fd, EPOLL_CTL_ADD, fd, &ev );
    } while (ret < 0 && errno == EINTR);
    return ret;
}

void ErrorSendData(char * reply,size_t *reply_len,char * ssid)
{
        ALOGD("yuelog mywifi  will ErrorSendData    size_t *reply_len = %d", *reply_len);
            snprintf(reply, *reply_len,
            "ie=\nid=0\nbssid=%s\nfreq=2437\nlevel=-41\ntsf=0000005475069290\nflags=[ESS]\nssid=%s\n####ie=\nid=0\nbssid=%s\nfreq=2437\nlevel=-41\ntsf=0000005475069290\nflags=[ESS]\nssid=%s\n####ie=\nid=0\nbssid=%s\nfreq=2437\nlevel=-41\ntsf=0000005475069290\nflags=[ESS]\nssid=%s\n####ie=\nid=0\nbssid=%s\nfreq=2437\nlevel=-41\ntsf=0000005475069290\nflags=[ESS]\nssid=%s\n####ie=\nid=0\nbssid=%s\nfreq=2437\nlevel=-41\ntsf=0000005475069290\nflags=[ESS]\nssid=%s\n####", 
            "12:34:56:78:9a:bc", ssid,
            "94:d4:69:fb:68:e1","VMOSWIFIerror1",
            "18:8b:9d:dd:c5:a3","VMOSWIFIerror2",
            "f0:3e:90:db:98:98","VMOSWIFIerror3",
            "f0:3e:90:9b:96:d8","VMOSWIFIerror4");
}


int wifi_supplicant_connection_active()
{
    ALOGE("REDF-LOG wifi_supplicant_connection_active\n");
    return is_wifi_supplicant_connection_active ? 0 : -1;
}

int wifi_ctrl_recv(char *reply, size_t *reply_len)
{
    ALOGE("REDF-LOG wifi_ctrl_recv\n");
    return -2;
}

int wifi_wait_on_socket(char *buf, size_t buflen)
{
    size_t nread = buflen - 1;
    int read_fd = sock_sv[0];
    if (is_ctrl_event_connected && read_fd != -1)
    {
        int read_len = read(read_fd, buf, nread);
        if (read_len < 0)
        {
            ALOGE("REDF-LOG wifi_wait_on_socket read socksv failed! %s\n", strerror(errno));
            close(read_fd);
            close(sock_sv[1]);
            sock_sv[0] = -1;
            sock_sv[1] = -1;

            return snprintf(buf, buflen, "IFNAME=%s %s - connection closed", "wlan0", WPA_EVENT_TERMINATING);
        }
        buf[read_len] = '\0';
        if (strstr(buf, "wlan CTRL-EVENT-SCAN-STARTED"))
        {
            usleep(0x186A0u);
        }
        if (strstr(buf, "wlan CTRL-EVENT-TERMINATING"))
        {
            is_ctrl_event_connected = 0;
        }
        return read_len;
    }
    else
    {
        return snprintf(buf, buflen, "IFNAME=%s %s - connection closed", "wlan0", WPA_EVENT_TERMINATING);
    }
}

int wifi_wait_for_event(char *buf, size_t buflen)
{
    return wifi_wait_on_socket(buf, buflen);
}

void wifi_close_sockets()
{
    ALOGE("REDF-LOG wifi_close_sockets\n");
}

void wifi_close_supplicant_connection()
{
    is_wifi_supplicant_connection_active = 0;
}

int wifi_command(const char *command, char *reply, size_t *reply_len)
{
    return wifi_send_command(command, reply, reply_len);
}

const char *wifi_get_fw_path(int fw_type)
{
    ALOGE("REDF-LOG wifi_get_fw_path, fw_type: %d\n", fw_type);
    return NULL;
}

int wifi_change_fw_path(const char *fwpath)
{
    ALOGE("REDF-LOG wifi_change_fw_path, fwpath: %s\n", fwpath);
    return 0;
}
