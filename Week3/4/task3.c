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
    if (limit < 2) {
        *outCount = 0;
        return malloc(0);
    }

    long numOdds = (limit - 3) / 2 + 1;
    if (numOdds < 0) numOdds = 0;
    bool* isComposite = (numOdds > 0) ? calloc(numOdds, sizeof(bool)) : NULL;

    for (long i = 0; i < numOdds; i++) {
        long p = 2 * i + 3;
        if (p * p > limit) break;
        if (!isComposite[i]) {
            // Only mark odd multiples of p; stepping by 2*p keeps them odd.
            for (long val = p * p; val <= limit; val += 2 * p)
                isComposite[(val - 3) / 2] = true;
        }
    }

    long count = 1; // account for prime 2
    for (long i = 0; i < numOdds; i++)
        if (!isComposite[i]) count++;

    long* basePrimes = malloc(count * sizeof(long));
    long idx = 0;
    basePrimes[idx++] = 2;
    for (long i = 0; i < numOdds; i++)
        if (!isComposite[i]) basePrimes[idx++] = 2 * i + 3;

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

    #pragma omp parallel for schedule(static)
    for (int seg = 0; seg < numSegments; seg++) {
        long low = 2 + (long)seg * SEGMENT_SIZE;
        long high = low + SEGMENT_SIZE - 1;
        if (high > n) high = n;

        // Only odd numbers in [low, high] are tracked; the even number 2
        // (which only ever appears in the very first segment) is added
        // separately below. index i <-> value (firstOdd + 2*i).
        long firstOdd = (low % 2 == 0) ? low + 1 : low;
        long numOdds = (firstOdd <= high) ? (high - firstOdd) / 2 + 1 : 0;

        // false = "prime" (not yet marked composite), local to this thread
        bool* isComposite = (numOdds > 0) ? calloc(numOdds, sizeof(bool)) : NULL;

        for (int bi = 0; bi < baseCount; bi++) {
            long p = basePrimes[bi];
            if (p == 2) continue; // no even numbers are tracked, so skip
            if (p * p > high) break; // no multiple of p can start in-range beyond this

            // First multiple of p that is >= low and >= p*p
            long start = (low / p) * p;
            if (start < low) start += p;
            if (start < p * p) start = p * p;
            if (start % 2 == 0) start += p; // p is odd, so this keeps start odd

            for (long i = start; i <= high; i += 2 * p)
                isComposite[(i - firstOdd) / 2] = true;
        }

        // Collect primes found in this segment into its own buffer
        long* localPrimes = malloc((high - low + 2) * sizeof(long));
        int localCount = 0;

        if (low <= 2 && high >= 2)
            localPrimes[localCount++] = 2;

        for (long i = 0; i < numOdds; i++) {
            if (!isComposite[i])
                localPrimes[localCount++] = firstOdd + 2 * i;
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

int main(int argc, char *argv[])
{
    omp_set_num_threads(atoi(argv[2])); // Set number of threads from command line argument

    #pragma omp parallel
    {
        #pragma omp single
        printf("Running with %d OpenMP thread(s)\n", omp_get_num_threads());
    }

    double start = omp_get_wtime();
    SegmentedSieve(argv[1]);
    double end = omp_get_wtime();

    printf("Time taken: %lf seconds\n", end - start);
    return 0;
}