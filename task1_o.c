#include <math.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

void WriteToFile(char *filename, bool *prime, int n) {
    FILE* file = fopen(filename, "w");

    fprintf(file, "2\n");
    for (int p = 3; p <= n; p += 2) {
        if (prime[p]) {
            fprintf(file, "%d\n", p);
        }
    }
    fclose(file);
}

void SieveOfEratosthenes(int n)
{
    bool* prime = malloc((n + 1) * sizeof(bool));
    for (int i = 0; i <= n; i++)
        prime[i] = true;
    prime[0] = prime[1] = false;
    prime[2] = true;

    if (n < 2) return;

    int add;
    for (int p = 3; p <= sqrt(n); p += 2) {
        if (prime[p]) {
            add = p * 2;
            for (int i = p + add; i <= n; i += add)
                prime[i] = false;
        }
    }

    if (n <= 100) {
        printf("Prime numbers up to %d:\n", n);
        printf("2 ");
        for (int p = 3; p <= n; p += 2) {
            if (prime[p])
            printf("%d ", p);
        }
        printf("\n");
    }
    else {
        WriteToFile("prime2.txt", prime, n);
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
    // Call SieveOfEratosthenes function with user input

    struct timespec start, end; 
    clock_gettime(CLOCK_MONOTONIC, &start); 
    SieveOfEratosthenes(n);
    clock_gettime(CLOCK_MONOTONIC, &end); 

    double time_taken;
    time_taken = (end.tv_sec - start.tv_sec) * 1e9; 
    time_taken = (time_taken + (end.tv_nsec - start.tv_nsec)) * 1e-9; 

    printf("Time taken: %lf seconds\n", time_taken);
    return 0;
}