#include <lcm/lcm.h>
#include <pthread.h>
#include <unistd.h>

#include "common/timestamp.h"
#include "lcmtypes/maebot_motor_command_t.h"

#define CMD_PRD 50000 //us  -> 20Hz
#define MTR_SPD 0.25f
#define MTR_STOP 0.0f

maebot_motor_command_t msg;
pthread_mutex_t msg_mutex;

void *
diff_drive_thread (void *arg)
{
    lcm_t *lcm = lcm_create (NULL);

    uint64_t utime_start;
    while(1) {
        utime_start = utime_now ();

        pthread_mutex_lock (&msg_mutex);
        maebot_motor_command_t_publish (lcm, "MAEBOT_MOTOR_COMMAND", &msg);
        pthread_mutex_unlock (&msg_mutex);

        usleep (CMD_PRD - (utime_now() - utime_start));
    }

    return NULL;
}

int
main (int argc, char *argv[])
{
    if (pthread_mutex_init (&msg_mutex, NULL)) {
        printf ("mutex init failed\n");
        return 1;
    }

    // Init msg
    // no need for mutex here, as command thread hasn't started yet.
    msg.left_motor_speed = MTR_STOP;
    msg.right_motor_speed = MTR_STOP;

    // Start sending motor commands
    pthread_t diff_drive_thread_pid;
    pthread_create (&diff_drive_thread_pid, NULL, diff_drive_thread, NULL);

    // forward
    pthread_mutex_lock (&msg_mutex);
    msg.left_motor_speed  = MTR_SPD;
    msg.right_motor_speed = MTR_SPD;
    pthread_mutex_unlock (&msg_mutex);

    usleep (500000);

    // reverse
    pthread_mutex_lock (&msg_mutex);
    msg.left_motor_speed  = -MTR_SPD;
    msg.right_motor_speed = -MTR_SPD;
    pthread_mutex_unlock (&msg_mutex);

    usleep (500000);

    // left turn
    pthread_mutex_lock (&msg_mutex);
    msg.left_motor_speed  = -MTR_SPD;
    msg.right_motor_speed = MTR_SPD;
    pthread_mutex_unlock (&msg_mutex);

    usleep (500000);

    // right turn
    pthread_mutex_lock (&msg_mutex);
    msg.left_motor_speed  = MTR_SPD;
    msg.right_motor_speed = -MTR_SPD;
    pthread_mutex_unlock (&msg_mutex);

    usleep (500000);

    // stop
    pthread_mutex_lock (&msg_mutex);
    msg.left_motor_speed  = MTR_STOP;
    msg.right_motor_speed = MTR_STOP;
    pthread_mutex_unlock (&msg_mutex);

    usleep (100000);

    return 0;
}
