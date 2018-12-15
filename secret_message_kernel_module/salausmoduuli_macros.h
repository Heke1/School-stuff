#include <linux/ioctl.h>

//if in use, have to be changed. Technically could also get the dynamically
//allocated major by grepping the /proc/devices, but yeah let's not do that.
#define MAJOR_NUM 222
#define NAME "salausmoduuli"
#define CLASS "harkka"
#define MSG_LEN 256
#define MAX_KEY_LEN 25

#define ENCRYPT _IOR(MAJOR_NUM, 0, char*)
#define DECRYPT _IOR(MAJOR_NUM, 1, char*)

//for setting the length of upcoming key
//As a curiosity, kernels in ubuntu LTS (4.10.0-40-generic) and debian jessie (3.16.0-4-amd64) were ok with running strlen(char*)
//on userspace ptr passed as an argument on decrypt and encrypt calls.
//However Arch running with 4.13.12-1-zen didn't like it at all.. :D
//dunno if this happens due a change in mainline or perhaps the zen patchset or different configuration makes this happen.
#define SET_LEN _IOR(MAJOR_NUM, 2, int)
