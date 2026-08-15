#define _POSIX_C_SOURCE 200809L

#include <math.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <pthread.h>
#include <semaphore.h>

sem_t semaphore;
bool* primes;
int* g_small_primes;
int g_small_primes_count;
int WINDOW_SIZE;

typedef struct s_t_arg {
    int lbound;
    int ubound;
} t_arg;

void* thread(void* args) {
    t_arg data = *(t_arg*) args;

    for (int k = 0; k < g_small_primes_count; k++) {
        int p = g_small_primes[k];
        int add = p * 2;
        int start = data.lbound - (data.lbound % add) + p;
        if (start < data.lbound) start += add;

        for (int i = start; i <= data.ubound; i += add) {
            primes[i] = false;
        }
    }

    sem_post(&semaphore);
    return NULL;
}

void sieve(bool* b, int n) {
    for (int p = 3; p <= n; p += 2) {
        if (b[p]) {
            int add = p * 2;
            for (int i = p + add; i <= n; i += add)
                b[i] = false;
        }
    }
}

void WriteToFile(char *filename, bool *prime, int n) {
    FILE* file = fopen(filename, "w");
    char buf[1 << 20];
    setvbuf(file, buf, _IOFBF, sizeof(buf));

    fprintf(file, "2\n");
    for (int p = 3; p <= n; p += 2) {
        if (prime[p]) {
            fprintf(file, "%d\n", p);
        }
    }
    fclose(file);
}

// Define main function
int main()
{
    // Get prime bound
    int n;
    printf("Enter the maximum number to find primes: ");
    scanf("%d", &n);
    
    WINDOW_SIZE = (int) floor(2 * n / sqrt(n)); // Approximate average size of n / [any prime] to get even distributions of operations

    // Thread count
    int t;
    printf("Enter the maximum number of threads: ");
    scanf("%d", &t);
    sem_init(&semaphore, 0, t);

    if (n < 2) return 0;

    int sqrt_n = (int) sqrt(n);

    struct timespec start, end;
    clock_gettime(CLOCK_MONOTONIC, &start);

    // Initial primes array setup
    primes = malloc((n + 1) * sizeof(bool));
    for (int i = 3; i <= n; i += 2) primes[i] = true;
    primes[0] = primes[1] = false;
    primes[2] = true;

    sieve(primes, sqrt_n); // Get primes in range [0,sqrt(n)] to run through threaded sieve

    // Store all primes to iterate through in thread
    g_small_primes = malloc(sizeof(int) * (sqrt_n / 2 + 2));
    g_small_primes_count = 0;
    for (int i = 3; i <= sqrt_n; i += 2)
        if (primes[i])
            g_small_primes[g_small_primes_count++] = i;

    pthread_t* threads = malloc(t * sizeof(pthread_t));
    t_arg* args = malloc(t * sizeof(t_arg));

    int counter = 0;
    int bound = sqrt_n + 1;

    // Screen for primes usins windows, checking a small region of every prime.
    // Parallel screening done in t number of screens using threads with bounds [lbound, ubound].
    while (bound < n) {
        sem_wait(&semaphore);
        args[counter % t].lbound = bound;
        args[counter % t].ubound = n < bound + WINDOW_SIZE - 1 ? n : bound + WINDOW_SIZE - 1; // Cap bound at n

        pthread_create(&threads[counter % t], NULL, thread, &args[counter % t]);
        counter++;

        // Free up next thread
        if ((counter + 1) >= t) {
            pthread_join(threads[(counter + 1) % t], NULL);
        }

        bound += WINDOW_SIZE;
    }

    // Make sure all threads are done
    for (int i = 0; i < t; i++) sem_wait(&semaphore);
    
    // Join all leftover threads
    counter %= t;
    for (int i = 0; i < counter; i++) pthread_join(threads[i], NULL);

    WriteToFile("prime3.txt", primes, n);

    // Cleanup
    free(primes);
    free(g_small_primes);
    free(threads);
    free(args);
    sem_destroy(&semaphore);

    clock_gettime(CLOCK_MONOTONIC, &end);

    double time_taken;
    time_taken = (end.tv_sec - start.tv_sec) + (end.tv_nsec - start.tv_nsec) * 1e-9;

    printf("Time taken: %lf seconds\n", time_taken);
    return 0;
}