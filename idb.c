#include "MobileDevice.h"

#include <string.h>
#include <stdlib.h>
#include <time.h>
#include <pwd.h>
#include <grp.h>        /* getgrgid(3) */

#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/types.h>

#define LS_BORDER_DAY 180

#define MAX(a, b) (((a) > (b)) ? (a) : (b))
#define HDOC( ... ) #__VA_ARGS__ "\n";

#define CSTR2CFSTR(str) CFStringCreateWithCString(NULL, str, kCFStringEncodingUTF8)
#define CFSTR2CSTR(str) (char *)CFStringGetCStringPtr(str, CFStringGetSystemEncoding())


/************************************************************************************************/
enum CommandType
{
  PRINT_UDID,
  PRINT_INFO,
  PRINT_APPS,
  INSTALL,
  UNINSTLL,
  APP_DIR,
  PRINT_SYSLOG,
  TUNNEL
};
struct
{
  enum CommandType type;
  struct am_device_notification *notification;
  const char *app_path;
  const char *bundle_id;
  const char *dir_path;
  uint16_t port_ios;
  uint16_t port_local;
} command;

struct
{
  struct passwd *pwd;
  struct group  *grp ;  
} user;

typedef struct am_device *AMDeviceRef;

#define ON_ERROR(...)              \
  do                               \
  {                                \
    fprintf(stderr, __VA_ARGS__);  \
    unregister_notification(EXIT_FAILURE); \
    fflush(stderr);                        \
  } while (0)

/************************************************************************************************/
/* Prototype */
static void on_device_notification(struct am_device_notification_callback_info *info, void *arg);
static void on_device_connected(AMDeviceRef device);
void register_notification();
void unregister_notification(int status);

/* stats */
void print_udid(AMDeviceRef device);
void print_info(AMDeviceRef device);
void print_apps(AMDeviceRef device);
void print_syslog(AMDeviceRef device);

void install(AMDeviceRef device);
void uninstall(AMDeviceRef device);
void create_tunnel(AMDeviceRef device);
void app_dir(AMDeviceRef device);

/************************************************************************************************/
char* str_join(const char *a, const char *b)
{
  size_t la = strlen(a);
  size_t lb = strlen(b);
  char* p = malloc(la + lb + 1);
  memcpy(p, a, la);
  memcpy(p + la, b, lb + 1);
  return p;
}


/************************************************************************************************/
void print_device_value(AMDeviceRef device, CFStringRef key)
{
  CFStringRef value = AMDeviceCopyValue(device, 0, key);
  if (value != NULL) {
    printf("%-20s\t%s\n", CFSTR2CSTR(key), CFSTR2CSTR(value));
    CFRelease(value);
  }
}

void register_notification()
{
  AMDeviceNotificationSubscribe(&on_device_notification, 0, 0, 0, &command.notification);
  CFRunLoopRun();
}
void unregister_notification(int status)
{
  AMDeviceNotificationUnsubscribe(command.notification);
  exit(status);
}

void connect_device(AMDeviceRef device)
{
  AMDeviceConnect(device);
  assert(AMDeviceIsPaired(device));
  assert(AMDeviceValidatePairing(device) == 0);
  assert(AMDeviceStartSession(device) == 0);
}
void disconnect_device(AMDeviceRef device)
{
  assert(AMDeviceStopSession(device) == 0);
  assert(AMDeviceDisconnect(device) == 0);
}
void connect_service(AMDeviceRef device, CFStringRef serviceName, unsigned int *serviceFd)
{
  connect_device(device);
  assert(AMDeviceStartService(device, serviceName, serviceFd, NULL) == 0);
  disconnect_device(device);
}
/************************************************************************************************/
/* Notification */
static void on_device_notification(struct am_device_notification_callback_info *info, void *arg)
{
  switch (info->msg) {
  case ADNCI_MSG_CONNECTED:
    on_device_connected(info->dev);
    default:
      break;
  }
}
static void on_device_connected(AMDeviceRef device)
{
  if (command.type == PRINT_UDID) {
    print_udid(device);
  } else if (command.type == PRINT_INFO) {
    print_info(device);
  } else if (command.type == PRINT_APPS) {
    print_apps(device);
  } else if (command.type == PRINT_SYSLOG) {
    print_syslog(device);
  } else if (command.type == INSTALL) {
    install(device);
  } else if (command.type == UNINSTLL) {
    uninstall(device);
  } else if (command.type == TUNNEL) {
    create_tunnel(device);
  } else if (command.type == APP_DIR) {
    app_dir(device);
  }
}

void create_user()
{
  uid_t uid;
  struct passwd *pwd;
  struct group *grp ;

  uid = getuid();
  pwd = getpwuid(uid);
  grp = getgrgid( pwd->pw_gid );

  user.pwd = pwd;
  user.grp = grp;
}
/************************************************************************************************/
/* Command Logic */

/************************************************
 idb udid
************************************************/
void print_udid(AMDeviceRef device)
{
  char *udid = CFSTR2CSTR(AMDeviceCopyDeviceIdentifier(device));
  if (udid == NULL)
  {
    unregister_notification(1);
  }
  printf("%s\n", udid);
  unregister_notification(0);
}
/************************************************
 idb info
************************************************/
void print_info(AMDeviceRef device)
{
  printf ("%s\n","[INFO]");
  connect_device(device);

  print_device_value(device, CFSTR("BasebandStatus"));
  print_device_value(device, CFSTR("BasebandVersion"));
  print_device_value(device, CFSTR("BluetoothAddress"));

  print_device_value(device, CFSTR("BuildVersion"));
  
  print_device_value(device, CFSTR("CPUArchitecture"));
  print_device_value(device, CFSTR("DeviceClass"));
  print_device_value(device, CFSTR("DeviceColor"));
  print_device_value(device, CFSTR("DeviceName"));
  print_device_value(device, CFSTR("FirmwareVersion"));
  print_device_value(device, CFSTR("HardwareModel"));
  print_device_value(device, CFSTR("HardwarePlatform"));
  print_device_value(device, CFSTR("IntegratedCircuitCardIdentity"));
  print_device_value(device, CFSTR("InternationalMobileSubscriberIdentity"));
  print_device_value(device, CFSTR("MLBSerialNumber"));

  print_device_value(device, CFSTR("MobileSubscriberCountryCode"));
  print_device_value(device, CFSTR("MobileSubscriberNetworkCode"));
  print_device_value(device, CFSTR("ModelNumber"));
  print_device_value(device, CFSTR("PhoneNumber"));

  print_device_value(device, CFSTR("ProductType"));
  print_device_value(device, CFSTR("ProductVersion"));
  print_device_value(device, CFSTR("ProtocolVersion"));

  print_device_value(device, CFSTR("RegionInfo"));
  print_device_value(device, CFSTR("SerialNumber"));
  print_device_value(device, CFSTR("SIMStatus"));

  print_device_value(device, CFSTR("TimeZone"));
  print_device_value(device, CFSTR("UniqueDeviceID"));
  print_device_value(device, CFSTR("WiFiAddress"));
  
  unregister_notification(0);  
}
/************************************************
 idb apps
************************************************/
static void on_app(const void *key, const void *value, void *context)
{
  if ((key == NULL) || (value == NULL))
  {
    return;
  }
  char *bundle_id = CFSTR2CSTR((CFStringRef)key);
  if (bundle_id == NULL) return;

  CFDictionaryRef app_dict = (CFDictionaryRef)value;
  /*
    ApplicationType (System or User)
    CFBundleDisplayName
    CFBundleName
  */
  CFStringRef app_type = CFDictionaryGetValue(app_dict, CFSTR("ApplicationType"));
  CFStringRef app_name = CFDictionaryGetValue(app_dict, CFSTR("CFBundleDisplayName"));

  char *app_name_cstr = (app_name != NULL) ? CFSTR2CSTR(app_name) : "-";
  if (CFStringCompare(app_type, CFSTR("User"), kCFCompareLocalized) == kCFCompareEqualTo) {
    printf ("%-20s\t%s\n",app_name_cstr, bundle_id);
  /* } else if (CFStringCompare(app_type, CFSTR("System"), kCFCompareLocalized) == kCFCompareEqualTo) { */
  /*   printf ("%-20s\t%s\n",app_name_cstr, bundle_id); */
  }
}
CFComparisonResult compare_bundle_id (
  const void *string1, const void *string2, void *locale)
{
      static CFOptionFlags compareOptions = kCFCompareCaseInsensitive |
                                                  kCFCompareNonliteral |
                                                  kCFCompareLocalized |
                                                  kCFCompareNumerically |
                                                  kCFCompareWidthInsensitive |
        kCFCompareForcedOrdering;

      CFRange string1Range = CFRangeMake(0, CFStringGetLength(string1));

          return CFStringCompareWithOptionsAndLocale
            (string1, string2, string1Range, compareOptions, (CFLocaleRef)locale);
}
void print_apps(AMDeviceRef device)
{
  connect_device(device);

  /* CFStringRef k[] = { CFSTR("kLookupReturnAttributesKey") }, v[] = { CFSTR("User") }; */
  /* CFDictionaryRef options = CFDictionaryCreate(NULL, (const void **)&k, (const void **)&v, 1, &kCFTypeDictionaryKeyCallBacks, &kCFTypeDictionaryValueCallBacks); */

  CFDictionaryRef apps;
  AMDeviceLookupApplications(device, NULL, &apps);
  //CFDictionaryApplyFunction(apps, print_bundle_id, NULL);
  CFIndex count = CFDictionaryGetCount(apps);
  CFTypeRef *keysTypeRef = (CFTypeRef *) malloc( count * sizeof(CFTypeRef) );
  CFDictionaryGetKeysAndValues(apps, (const void **) keysTypeRef, NULL);
  const void **keys = (const void **) keysTypeRef;

  /* sort */
  CFMutableArrayRef keyArray = CFArrayCreateMutable(kCFAllocatorDefault, count, NULL);
  int i;
  for (i=0; i<count; i++) {
    CFArrayAppendValue(keyArray, keys[i]);
  }
  CFRange arrayRange = CFRangeMake(0, CFArrayGetCount(keyArray));
  CFLocaleRef locale = CFLocaleCopyCurrent();
  CFArraySortValues(keyArray, arrayRange,
                     compare_bundle_id, (void *)locale);
  for (i=0; i<count; i++) {
    CFStringRef bundle_id = CFArrayGetValueAtIndex(keyArray, i);
    CFDictionaryRef app_dict = CFDictionaryGetValue(apps, bundle_id);
    on_app(bundle_id, app_dict, NULL);
  }
  
  CFRelease(apps);
  unregister_notification(0);  
}
/************************************************
 idb log
************************************************/
void print_syslog(AMDeviceRef device)
{
  unsigned int socket;          /*  (*afc_connection)  */
  connect_service(device, AMSVC_SYSLOG_RELAY, &socket);

  char c;
  while(recv(socket, &c, 1, 0) == 1) {
    if(c != 0)
      putchar(c);
  }
  unregister_notification(0);
}
/************************************************
 idb install 
************************************************/
static void on_transfer(CFDictionaryRef dict, int arg) {
  int percent;
  CFStringRef status = CFDictionaryGetValue(dict, CFSTR("Status"));
  CFNumberGetValue(CFDictionaryGetValue(dict, CFSTR("PercentComplete")), kCFNumberSInt32Type, &percent);
  
  printf("[%3d%%] %s\n", percent / 2, CFStringGetCStringPtr(status, kCFStringEncodingMacRoman));
}
static void on_install(CFDictionaryRef dict, int arg) {
  int percent;
  CFStringRef status = CFDictionaryGetValue(dict, CFSTR("Status"));
  CFNumberGetValue(CFDictionaryGetValue(dict, CFSTR("PercentComplete")), kCFNumberSInt32Type, &percent);
  
  printf("[%3d%%] %s\n", (percent / 2) + 50, CFStringGetCStringPtr(status, kCFStringEncodingMacRoman));
}
void install(AMDeviceRef device)
{
  printf("[%3d%%] Start: %s\n", 0, command.app_path);  

  CFStringRef path = CSTR2CFSTR(command.app_path);
  CFStringRef keys[] = { CFSTR("PackageType") }, values[] = { CFSTR("Developer") };
  CFDictionaryRef options = CFDictionaryCreate(NULL, (const void **)&keys, (const void **)&values, 1, &kCFTypeDictionaryKeyCallBacks, &kCFTypeDictionaryValueCallBacks);

  /* Transfer */
  unsigned int afcFd;
  connect_service(device, AMSVC_AFC, &afcFd);
  assert(AMDeviceTransferApplication(afcFd, path, NULL, on_transfer, 0) == 0);
  close(afcFd);
  
  /* Install */
  unsigned int installFd;
  connect_service(device, AMSVC_INSTALLATION_PROXY, &installFd);
  mach_error_t result = AMDeviceInstallApplication(installFd, path, options, on_install, 0);
  if (result != 0)
  {
    printf("AMDeviceInstallApplication failed: %d\n", result);
    unregister_notification(1);
  }
  close(installFd);
  
  CFRelease(path);
  CFRelease(options);

  printf("[100%%] Installed app: %s\n", command.app_path);  
  unregister_notification(0);
}

/************************************************
 idb uninstall
************************************************/
void uninstall(AMDeviceRef device)
{
  CFStringRef bundle_id = CSTR2CFSTR(command.bundle_id);
  connect_device(device);
  
  mach_error_t result = AMDeviceSecureUninstallApplication(0,device, bundle_id, 0, NULL, 0);
  if (result != 0)
  {
    printf("AMDeviceSecureUnInstallApplication failed: %d\n", result);
    unregister_notification(1);
  }
  
  printf("Uninstalled bundle_id: %s\n", command.bundle_id);  
  unregister_notification(0);
}
/************************************************
 idb dir 
************************************************/
/*
  keys
  - st_ifmt (S_IFDIR, S_IFLNK)
  - st_nlink
  - st_size
  - st_blocks
  - st_mtime
  - st_birthtime
*/
static void on_file(char *file_name, CFMutableDictionaryRef file_dict)
{

  time_t std_time;
  time(&std_time);
  std_time -= (24 * 60 * 60 * LS_BORDER_DAY);

//  CFShow(file_dict);
  CFStringRef ifmt = (CFStringRef)CFDictionaryGetValue(file_dict,CFSTR("st_ifmt"));
  char *ifmt_cstr = (CFStringCompare(ifmt,CFSTR("S_IFDIR"), kCFCompareLocalized) == kCFCompareEqualTo) ? "d" : "-";

  CFStringRef nlink = (CFStringRef)CFDictionaryGetValue(file_dict,CFSTR("st_nlink"));
  CFStringRef size = (CFStringRef)CFDictionaryGetValue(file_dict,CFSTR("st_size"));
  CFStringRef mtime = (CFStringRef)CFDictionaryGetValue(file_dict,CFSTR("st_mtime"));
  time_t mtimel = atoll(CFSTR2CSTR(mtime)) / 1000000000L;

  struct tm *mtimetm = localtime(&mtimel);

  char tmbuf[64];
  if (std_time <= mtimel) {
    strftime(tmbuf, sizeof tmbuf, "%b %d %H:%M", mtimetm);
  } else {
    strftime(tmbuf, sizeof tmbuf, "%b %d  %Y", mtimetm);
  }


  printf ("%s---------%4s %s %6s %6s %s %s\n",
          ifmt_cstr,
          CFSTR2CSTR(nlink),    /* TOOD  */
          user.pwd->pw_name,
          user.grp->gr_name,
          CFSTR2CSTR(size),
          tmbuf,
          file_name);
}

void app_dir(AMDeviceRef device)
{
  CFStringRef bundle_id = CSTR2CFSTR(command.bundle_id);
  
  service_conn_t socket;
  connect_device(device);
  create_user();
  
  AMDeviceStartHouseArrestService(device, bundle_id, NULL, &socket, 0);

  afc_connection *afc_conn;
  int ret = AFCConnectionOpen(socket, 0, &afc_conn);
  if (ret != ERR_SUCCESS) {
    printf ( "AFCConnectionOpen = %i\n" , ret );
    unregister_notification(1);  
  }
  struct afc_directory *dir;
  char *dirent;
  
  AFCDirectoryOpen(afc_conn, command.dir_path, &dir); 
  for (;;) {
    AFCDirectoryRead(afc_conn, dir, &dirent);
    if (!dirent) break;

    /* can't traverse */
    if (strcmp(command.dir_path, ".") == 0 && strcmp(dirent, "..") == 0) continue;

    struct afc_dictionary *file_info;
    char *dir_path = str_join(command.dir_path, "/");
    dir_path = str_join(dir_path, dirent);
    int r = AFCFileInfoOpen(afc_conn, dir_path, &file_info);
    if (r) {
      printf("%s doesn't exist \n", dir_path);
      continue;
    }
    CFMutableDictionaryRef file_dict = CFDictionaryCreateMutable(kCFAllocatorDefault,0,
                                                          &kCFTypeDictionaryKeyCallBacks,
                                                          &kCFTypeDictionaryValueCallBacks);
    char *key, *value;
    AFCKeyValueRead(file_info, &key, &value);
    while(key || value) {
      CFStringRef k = CSTR2CFSTR(key);
      CFStringRef v = CSTR2CFSTR(value);
      CFDictionarySetValue(file_dict, k, v);
      AFCKeyValueRead(file_info, &key, &value);
    }
    AFCKeyValueClose(file_info);
    on_file(dirent, file_dict);
  }
  AFCDirectoryClose(afc_conn, dir);
  unregister_notification(0);
}

/************************************************
 idb tunnel <iPhone port> <local port>
************************************************/
service_conn_t create_local_socket()
{
  int reuse = 1;
  service_conn_t sock_local;
  struct sockaddr_in addr_local;
  socklen_t len_local;

  /* socket */
  if ((sock_local = socket(AF_INET, SOCK_STREAM, 0)) < 0) {  
    fprintf(stderr, "create socket failed. \n");
    unregister_notification(1);
  }
  setsockopt(sock_local, SOL_SOCKET, SO_REUSEADDR, (const char *)&reuse, sizeof(reuse));

  addr_local.sin_family = AF_INET;
  addr_local.sin_port = htons(command.port_local);
  addr_local.sin_addr.s_addr = htonl(INADDR_ANY);
  /* bind */
  len_local = sizeof(addr_local);
  if (bind(sock_local, (struct sockaddr *)&addr_local, len_local) != 0) {
    ON_ERROR("bind failed.");
  }
  /* listen */
  if (listen(sock_local, 5) != 0) {
    ON_ERROR("listen failed.");
  }

  if (command.port_local == 0 && getsockname(sock_local, (struct sockaddr *)&addr_local, &len_local) == 0) {
      command.port_local = ntohs(addr_local.sin_port);
  }
  printf ("Success: Open    localhost (%d)\n", command.port_local);  
  return sock_local;
}

void forward_socket(service_conn_t sock_from, service_conn_t sock_to)
{
  char buf[BUFSIZ];

  service_conn_t sock_accept;
  struct sockaddr_in addr_accept;
  socklen_t len_accept = sizeof(addr_accept);

  if ((sock_accept = accept(sock_from, (struct sockaddr *)&addr_accept, &len_accept)) < 0) {
    ON_ERROR("accept failed. \n");
  }
  ssize_t recv_size = recv(sock_accept, buf, sizeof(buf), 0);  
  if (send(sock_to, buf, recv_size, 0) < 1) {
    ON_ERROR("Failed: write sock_to.");
  }
  recv_size = recv(sock_to, buf, sizeof(buf), 0);
  
  if (send(sock_accept, buf, recv_size, 0) < 1) {
    ON_ERROR("Failed: write sock_accept.");
  }
  close(sock_accept);
}

void create_tunnel(AMDeviceRef device)
{
  connect_device(device);

  /* to iPhone */
  service_conn_t sock_iphone;
  int ret = USBMuxConnectByPort(AMDeviceGetConnectionID(device), htons(command.port_ios), &sock_iphone);
  if (ret != ERR_SUCCESS) {
    ON_ERROR("Failed: Connect usb port(%d)\n", command.port_ios);
  }
  /* afc_connection *afc_conn; */
  /* ret = AFCConnectionOpen(sock_iphone, 0, &afc_conn); */
  /* if (ret != ERR_SUCCESS) { */
  /*   fprintf(stderr,"AFCConnectionOpen = %i\n" , ret); */
  /*   unregister_notification(1);   */
  /* } */
  printf ("Success: Connect iOS Device(%d)\n", command.port_ios);

  /* local port */
  service_conn_t sock_local = create_local_socket();

  /* select(2) */
  fd_set fds, fds_org;
  int maxfd;
  int acceible;
  
  maxfd = MAX(sock_iphone, sock_local);
  
  FD_ZERO(&fds_org);
  FD_SET(sock_iphone, &fds_org);
  FD_SET(sock_local, &fds_org);

  printf ("forwarding iOS(%u) => local(%u) \n", command.port_ios, command.port_local);

  while(1) {
    memcpy(&fds, &fds_org, sizeof(fd_set));
    if ((acceible = select(maxfd+1, &fds, NULL, NULL, NULL)) < 0) {
      ON_ERROR("Failed: Select");
    }
    
    if (FD_ISSET(sock_iphone, &fds)) {
      forward_socket(sock_iphone, sock_local);
    }
    if (FD_ISSET(sock_local, &fds)) {
//      printf("Listening socket is readable.\n");
      forward_socket(sock_local, sock_iphone);
    }
  }
  close(sock_iphone);
  close(sock_local);  
  unregister_notification(0);  
}

/************************************************************************************************/
/* Main */
void usage()
{
  char* str = HDOC(
    Usage:idb <command>\n
    command is below \n
    - udid \n
    - info \n
    - apps \n
    - ls <bundle_id> <relative_path>\n
    - install <app_path or ipa_path> \n
    - uninstall <bundle_id> \n 
  );
  printf("%s\n", str);
}

int main (int argc, char *argv[]) {
  if (argc < 2) {
    usage();
    exit(1);
  }
  if ((argc == 2) && (strcmp(argv[1], "udid") == 0)) {
    command.type = PRINT_UDID;
  } else if ((argc == 2) && (strcmp(argv[1], "info") == 0)) {
    command.type = PRINT_INFO;
  } else if ((argc == 2) && (strcmp(argv[1], "apps") == 0)) {
    command.type = PRINT_APPS;
  } else if ((argc == 2) && (strcmp(argv[1], "log") == 0)) {
    command.type = PRINT_SYSLOG;
  } else if ((argc == 3) && (strcmp(argv[1], "ls") == 0)) {
    command.type = APP_DIR;
    command.bundle_id = argv[2];
    command.dir_path  = ".";
  } else if ((argc == 4) && (strcmp(argv[1], "ls") == 0)) {
    command.type = APP_DIR;
    command.bundle_id = argv[2];
    command.dir_path  = argv[3];
  } else if ((argc == 3) && (strcmp(argv[1], "install") == 0)) {
    command.type = INSTALL;
    command.app_path = argv[2];
  } else if ((argc == 3) && (strcmp(argv[1], "uninstall") == 0)) {
    command.type = UNINSTLL;
    command.bundle_id = argv[2];
  } else if ((argc == 3) && (strcmp(argv[1], "tunnel") == 0)) {
    command.type = TUNNEL;
    command.port_ios   = (uint16_t)atoi(argv[2]);
    command.port_local = 0;     /* ANY_PORT */
  } else if ((argc == 4) && (strcmp(argv[1], "tunnel") == 0)) {
    command.type = TUNNEL;
    command.port_ios   = (uint16_t)atoi(argv[2]);
    command.port_local = (uint16_t)atoi(argv[3]);
  } else {
    fprintf(stderr, "Unknown command\n");
    usage();
    exit(1);
  }
  AMDSetLogLevel(5);
  AMDAddLogFileDescriptor(fileno(stderr));
  register_notification();
  return 0;
}
