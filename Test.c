#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>

// Struct for thread arguments
typedef struct {
    int start;
    int end;
    int* primes;
    int* count;
} ThreadArgs;

// Function to check if a number is prime
int is_prime(int num) {
    if (num <= 1) return 0;
    if (num % 2 == 0 && num > 2) return 0;
    for(int i = 3; i * i <= num; i += 2) {
        if (num % i == 0)
            return 0;
    }
    return 1;
}

// Thread function
void* find_primes(void* args) {
    ThreadArgs* ta = (ThreadArgs*)args;
    for (int num = ta->start; num <= ta->end; num++) {
        if (is_prime(num)) {
            ta->primes[(*ta->count)++] = num;
        }
    }
    return NULL;
}

int main() {
    const int N = 500;
    const int num_threads = 4;
    pthread_t threads[num_threads];
    ThreadArgs thread_args[num_threads];
    int* primes[num_threads];
    int counts[num_threads];

    // Initialize and create threads
    for (int i = 0; i < num_threads; i++) {
        counts[i] = 0;
        primes[i] = malloc(N / 4 * sizeof(int));
        thread_args[i] = (ThreadArgs){.start = i * N / 4 + 1, .end = (i + 1) * N / 4, .primes = primes[i], .count = &counts[i]};
        pthread_create(&threads[i], NULL, find_primes, &thread_args[i]);
    }

    // Wait for threads to finish and print primes
    for (int i = 0; i < num_threads; i++) {
        pthread_join(threads[i], NULL);
        for (int j = 0; j < counts[i]; j++) {
            printf("%d\n", primes[i][j]);
        }
        free(primes[i]);
    }

    return 0;
}