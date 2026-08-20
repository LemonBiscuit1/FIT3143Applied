#include <math.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <omp.h>

// Forward declaration (missing in the original file, which caused an
// implicit-declaration warning / undefined behaviour on some compilers)
void WriteToFile(char *filename, bool *prime, int n);

// Define SieveOfEratosthenes function (OpenMP-parallel version)
void SieveOfEratosthenes(int n)
{
    // Allocate memory for prime array and initialize all
    // elements as true
    bool* prime = malloc((n + 1) * sizeof(bool));

    // Parallel initialisation - independent, so trivially parallel
    #pragma omp parallel for schedule(static)
    for (int i = 0; i <= n; i++)
        prime[i] = true;

    // 0 and 1 are not prime numbers
    prime[0] = prime[1] = false;

    // The OUTER loop (choosing which p to sieve with) must stay serial:
    // whether p is prime depends on all the marking done by smaller
    // primes, so iterations of this loop are not independent.
    int limit = (int)sqrt((double)n);
    for (int p = 3; p <= limit; p++) {
        // If p is prime
        if (prime[p]) {
            // Mark all multiples of p as non-prime.
            // This INNER loop is where almost all the work is, and each
            // iteration writes to a different index of `prime`, so it is
            // safe (and effective) to parallelize.
            #pragma omp parallel for schedule(dynamic, 1024)
            for (int i = p * p; i <= n; i += p)
                prime[i] = false;
        }
    }

    if (n <= 100) {
        // Print all prime numbers up to n (kept serial - tiny amount of
        // work and I/O ordering matters for the printed output)
        printf("Prime numbers up to %d:\n", n);
        if (n >= 2) printf("2 ");
        for (int p = 3; p <= n; p += 2) {
            if (prime[p])
                printf("%d ", p);
        }
        printf("\n");
    }
    else {
        WriteToFile("prime.txt", prime, n);
    }

    // Free allocated memory
    free(prime);
}

// Define main function
int main()
{
    // Declare variable to hold maximum number
    int n;
    // Prompt user to enter the maximum number
    printf("Enter the maximum number to find primes: ");
    // Read user input
    scanf("%d", &n);

    // Report how many threads OpenMP will use, for reference
    #pragma omp parallel
    {
        #pragma omp single
        printf("Running with %d OpenMP thread(s)\n", omp_get_num_threads());
    }

    // Use omp_get_wtime for a wall-clock timer that works correctly
    // across the parallel regions above
    double start = omp_get_wtime();
    SieveOfEratosthenes(n);
    double end = omp_get_wtime();

    printf("Time taken: %lf seconds\n", end - start);
    return 0;
}

void WriteToFile(char *filename, bool *prime, int n) {
    FILE* file = fopen(filename, "w");
    if (!file) {
        perror("fopen");
        return;
    }

    fprintf(file, "2\n");
    // Kept serial: writing to a single FILE* from multiple threads would
    // need synchronization that would likely erase any speed benefit,
    // and I/O is not the bottleneck compared to the sieve itself.
    for (int p = 3; p <= n; p += 2) {
        if (prime[p]) {
            fprintf(file, "%d\n", p);
        }
    }
    fclose(file);
}