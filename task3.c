#include <math.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <omp.h>

// Forward declarations
void SegmentedSieve(long n);
void WriteToFile(char *filename, long **segPrimes, int *segCount, int numSegments);

#define SEGMENT_SIZE 131072L // 2^17

// Simple (non-segmented) sieve, used once to find all base primes up
// to sqrt(n). This part is small and stays serial.
static long* SimpleSieve(long limit, int* outCount)
{
    bool* isComposite = calloc(limit + 1, sizeof(bool));
    long count = 0;

    for (long p = 2; p * p <= limit; p++) {
        if (!isComposite[p]) {
            for (long i = p * p; i <= limit; i += p)
                isComposite[i] = true;
        }
    }

    for (long p = 2; p <= limit; p++)
        if (!isComposite[p]) count++;

    long* basePrimes = malloc(count * sizeof(long));
    long idx = 0;
    for (long p = 2; p <= limit; p++)
        if (!isComposite[p]) basePrimes[idx++] = p;

    free(isComposite);
    *outCount = (int)count;
    return basePrimes;
}

void SegmentedSieve(long n)
{
    if (n < 2) {
        printf("No primes <= %ld\n", n);
        return;
    }

    long limit = (long)sqrt((double)n) + 1;

    // Step 1: base primes up to sqrt(n), computed once, serially.
    int baseCount;
    long* basePrimes = SimpleSieve(limit, &baseCount);

    int numSegments = (int)((n - 2) / SEGMENT_SIZE) + 1;

    long** segPrimes = malloc(numSegments * sizeof(long*));
    int* segCount = malloc(numSegments * sizeof(int));

    #pragma omp parallel for schedule(dynamic)
    for (int seg = 0; seg < numSegments; seg++) {
        long low = 2 + (long)seg * SEGMENT_SIZE;
        long high = low + SEGMENT_SIZE - 1;
        if (high > n) high = n;
        long segLen = high - low + 1;

        // false = "prime" (not yet marked composite), local to this thread
        bool* isComposite = calloc(segLen, sizeof(bool));

        for (int bi = 0; bi < baseCount; bi++) {
            long p = basePrimes[bi];
            if (p * p > high) break; // no multiple of p can start in-range beyond this

            // First multiple of p that is >= low and >= p*p
            long start = (low / p) * p;
            if (start < low) start += p;
            if (start < p * p) start = p * p;

            for (long i = start; i <= high; i += p)
                isComposite[i - low] = true;
        }

        // Collect primes found in this segment into its own buffer
        long* localPrimes = malloc(segLen * sizeof(long));
        int localCount = 0;
        for (long i = 0; i < segLen; i++) {
            long value = low + i;
            if (value >= 2 && !isComposite[i])
                localPrimes[localCount++] = value;
        }

        segPrimes[seg] = localPrimes;
        segCount[seg] = localCount;

        free(isComposite);
    }

    free(basePrimes);


    if (n <= 100) {
        printf("Prime numbers up to %ld:\n", n);
        for (int seg = 0; seg < numSegments; seg++) {
            for (int i = 0; i < segCount[seg]; i++)
                printf("%ld ", segPrimes[seg][i]);
        }
        printf("\n");
        for (int seg = 0; seg < numSegments; seg++)
            free(segPrimes[seg]);
    }
    else {
        WriteToFile("prime.txt", segPrimes, segCount, numSegments);
        for (int seg = 0; seg < numSegments; seg++)
            free(segPrimes[seg]);
    }

    free(segPrimes);
    free(segCount);
}

void WriteToFile(char *filename, long **segPrimes, int *segCount, int numSegments)
{
    FILE* file = fopen(filename, "w");
    if (!file) {
        perror("fopen");
        return;
    }
    for (int seg = 0; seg < numSegments; seg++) {
        for (int i = 0; i < segCount[seg]; i++)
            fprintf(file, "%ld\n", segPrimes[seg][i]);
    }
    fclose(file);
}

int main()
{
    
    long n;
    printf("Enter the maximum number to find primes: ");
    if (scanf("%ld", &n) != 1) {
        fprintf(stderr, "Invalid input\n");
        return 1;
    }
    

    #pragma omp parallel
    {
        #pragma omp single
        printf("Running with %d OpenMP thread(s)\n", omp_get_num_threads());
    }

    double start = omp_get_wtime();
    SegmentedSieve(n);
    double end = omp_get_wtime();

    printf("Time taken: %lf seconds\n", end - start);
    return 0;
}