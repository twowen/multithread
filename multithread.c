/*
 * Owen Wu <twowen@gmail.com>
 *
 * Refer to: https://mmanoba.wordpress.com/2011/07/18/c-program-linux-posix-multithread-multitasking-example/
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
#include <termios.h>

#define FALSE   0
#define TRUE    1

static struct termios tty_state_current;

pthread_t func_one_thread, func_two_thread, func_three_thread;

pthread_cond_t thread_cond_one, thread_cond_two, thread_cond_three;

pthread_mutex_t mutex_flag;

short tflag_one_on, tflag_two_on, tflag_three_on, quit_flag = FALSE;

int count_one, count_two = 0;

int echo_on (void)
{
    struct termios tty_state;

    if (tcgetattr(0, &tty_state) < 0)
    {
        perror("echo_on: tcgetattr");
        return -1;
    }

    tty_state.c_lflag |= ECHO;
    return tcsetattr(0, TCSANOW, &tty_state);
}

int echo_off (void)
{
    struct termios tty_state;

    if (tcgetattr(0, &tty_state) < 0)
    {
        perror("echo_off: tcgetattr");
        return -1;
    }

    tty_state.c_lflag &= ~ECHO;
    return tcsetattr(0, TCSANOW, &tty_state);
}

void restore_terminal_setting (void)
{
    tcsetattr(0, TCSANOW, &tty_state_current);
}

void disable_waiting_for_enter (void)
{
    struct termios tty_state;

    tcgetattr(0, &tty_state_current);
    tty_state = tty_state_current;
    tty_state.c_lflag &= ~(ICANON | ECHO);
    tcsetattr(0, TCSANOW, &tty_state);
    atexit(restore_terminal_setting);
}

void *watch_for_user_keypress (void)
{
    char getKey;

    disable_waiting_for_enter();
    echo_off();

    do
    {
        getKey = getchar();
        switch (getKey)
        {
            case 49:
                if (!tflag_one_on)
                {
                    tflag_one_on = TRUE;
                    pthread_mutex_lock(&mutex_flag);
                    pthread_cond_signal(&thread_cond_one);
                    pthread_mutex_unlock(&mutex_flag);
                }
                else
                {
                    tflag_one_on = FALSE;
                }
                break;
            case 50:
                if (!tflag_two_on)
                {
                    tflag_two_on = TRUE;
                    pthread_mutex_lock(&mutex_flag);
                    pthread_cond_signal(&thread_cond_two);
                    pthread_mutex_unlock(&mutex_flag);
                }
                else
                {
                    tflag_two_on = FALSE;
                }
                break;
            case 51:
                if (!tflag_three_on)
                {
                    tflag_three_on = TRUE;
                    pthread_mutex_lock(&mutex_flag);
                    pthread_cond_signal(&thread_cond_three);
                    pthread_mutex_unlock(&mutex_flag);
                }
                else
                {
                    tflag_three_on = FALSE;
                }
                break;
        }
    }
    while (getKey != 48);

    quit_flag = TRUE;

    pthread_cond_broadcast(&thread_cond_one);
    pthread_cond_broadcast(&thread_cond_two);
    pthread_cond_broadcast(&thread_cond_three);

    pthread_exit(NULL);
}

void *func_one (void)
{
    pthread_mutex_lock(&mutex_flag);
    pthread_cond_wait(&thread_cond_one, &mutex_flag);
    pthread_mutex_unlock(&mutex_flag);

    if (quit_flag) pthread_exit(NULL);

    while (TRUE)
    {
        if (!tflag_one_on)
        {
            pthread_mutex_lock(&mutex_flag);
            pthread_cond_wait(&thread_cond_one, &mutex_flag);
            pthread_mutex_unlock(&mutex_flag);
        }
        if (quit_flag) break;
        fprintf(stderr, "<<--%d-->> ", ++count_one);
        sleep(1);
    }

    pthread_exit(NULL);
}

void *func_two (void)
{
    pthread_mutex_lock(&mutex_flag);
    pthread_cond_wait(&thread_cond_two, &mutex_flag);
    pthread_mutex_unlock(&mutex_flag);

    if (quit_flag) pthread_exit(NULL);

    while (TRUE)
    {
        if (!tflag_two_on)
        {
            pthread_mutex_lock(&mutex_flag);
            pthread_cond_wait(&thread_cond_two, &mutex_flag);
            pthread_mutex_unlock(&mutex_flag);
        }
        if (quit_flag) break;
        fprintf(stderr, "<<oo%doo>> ", ++count_two);
        sleep(1);
    }

    pthread_exit(NULL);
}

void *func_three (void)
{
    int count = 0;

    pthread_mutex_lock(&mutex_flag);
    pthread_cond_wait(&thread_cond_three, &mutex_flag);
    pthread_mutex_unlock(&mutex_flag);

    if (quit_flag) pthread_exit(NULL);

    while (TRUE)
    {
        if (!tflag_three_on)
        {
            pthread_mutex_lock(&mutex_flag);
            pthread_cond_wait(&thread_cond_three, &mutex_flag);
            pthread_mutex_unlock(&mutex_flag);
        }
        if (quit_flag) break;
        fprintf(stderr, "<<==%d::%d==>> ", ++count, (count_one + count_two));
        sleep(1);
    }

    pthread_exit(NULL);
}

int main (int argc, char *argv[])
{
    pthread_t watch_for_user_keypress_thread;

    pthread_mutex_init(&mutex_flag, NULL);

    pthread_cond_init(&thread_cond_one, NULL);
    pthread_cond_init(&thread_cond_two, NULL);
    pthread_cond_init(&thread_cond_three, NULL);

    pthread_create(&watch_for_user_keypress_thread, NULL, (void *)watch_for_user_keypress, NULL);
    pthread_create(&func_one_thread, NULL, (void *)func_one, NULL);
    pthread_create(&func_two_thread, NULL, (void *)func_two, NULL);
    pthread_create(&func_three_thread, NULL, (void *)func_three, NULL);

    pthread_join(watch_for_user_keypress_thread, NULL);
    pthread_join(func_one_thread, NULL);
    pthread_join(func_two_thread, NULL);
    pthread_join(func_three_thread, NULL);

    printf("\n");
    return 0;
}
