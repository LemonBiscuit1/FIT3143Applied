#define _POSIX_C_SOURCE 200809L

#include <math.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <pthread.h>

pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
bool* primes;
int* g_small_primes;
int g_small_primes_count;
int WINDOW_SIZE = 65536;
int bound;
int n;

void* thread() {
    int lbound;
    int ubound;

    // Screen for primes using windows, checking a small region of every prime.
    // Claim a screen of width WINDOW_SIZE, and increment it to let a next thread do the work
    while (bound < n) {
        pthread_mutex_lock(&mutex);
        lbound = bound;
        ubound = n < bound + WINDOW_SIZE - 1 ? n : bound + WINDOW_SIZE - 1; // Cap bound at n

        bound += WINDOW_SIZE;
        pthread_mutex_unlock(&mutex);
        
        for (int k = 0; k < g_small_primes_count; k++) {
            int p = g_small_primes[k];
            int add = p * 2;
            int start = lbound - (lbound % add) + p;

            if (start < lbound) start += add;
            if (start < p * p) start = p * p;

            for (int i = start; i <= ubound; i += add) {
                primes[i] = false;
            }
        }
    }
    return NULL;
}

void sieve(bool* b, int n) {
    for (int p = 3; p <= n; p += 2) {
        if (b[p]) {
            int add = p * 2;
            for (int i = p * p; i <= n; i += add)
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
    printf("Enter the maximum number to find primes: ");
    scanf("%d", &n);

    // // Thread count
    int t;
    printf("Enter the maximum number of threads: ");
    scanf("%d", &t);

    if (n < 2) return 0;
    if (t < 1) return 0;

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

    bound = sqrt_n + 1;
    for (int i = 0; i < t; i++) pthread_create(&threads[i], NULL, thread, NULL);
    
    // Join all threads
    for (int i = 0; i < t; i++) pthread_join(threads[i], NULL);

    WriteToFile("prime3.txt", primes, n);

    // Cleanup
    free(primes);
    free(g_small_primes);

    clock_gettime(CLOCK_MONOTONIC, &end);

    double time_taken;
    time_taken = (end.tv_sec - start.tv_sec) + (end.tv_nsec - start.tv_nsec) * 1e-9;

    printf("Time taken: %lf seconds\n", time_taken);

    return 0;
}