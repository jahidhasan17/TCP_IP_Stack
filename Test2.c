#include <stdio.h>
#include <pthread.h>
#include <unistd.h> // for sleep()


void *test_method(void *arg){
    printf("Test Method\n");
    return NULL;
}


int main(int argc, char const *argv[])
{
    
    pthread_t client_thread;
    pthread_attr_t attr;

    pthread_attr_init(&attr);
    pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);

    pthread_create(&client_thread, &attr, test_method, NULL);

    sleep(1); // wait for 1 second

    return 0;
}
