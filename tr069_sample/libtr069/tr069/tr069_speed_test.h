#ifndef __TR069_SPEED_TEST_H__
#define __TR069_SPEED_TEST_H__

#include "n_lib.h"

#define IPTV_SPEED_TEST_DIR "/var/speedtest"
#define IPTV_SPEED_TEST_FILE "/var/speedtest/tf"
#define IPTV_SPEED_TEST_CTIMEOUT 10
#define IPTV_SPEED_TEST_DTIMEOUT 600

#define MAX_DURATION 300

struct speed_test_infos {
	unsigned int duration;
	char filepath[F_MAX_PATH_SIZE];
};

int tr069_speed_test(struct speed_test_infos *info);
int upload_speed_test(void *arg);

#endif
