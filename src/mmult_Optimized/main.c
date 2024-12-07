/* Standard C includes */
#define _GNU_SOURCE
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>
#include <sched.h>
#include <stdbool.h>
#include <inttypes.h>
#include <time.h>
#include <unistd.h>
#include <errno.h>
#include <sys/stat.h>
#include <sys/types.h>

/* Include all implementations declarations */
#include "impl/naive.h"
#include "impl/opt.h"  // Include optimized implementation

/* Include common headers */
#include "common/types.h"
#include "common/macros.h"

/* Include application-specific headers */
#include "include/types.h"

const int SIZE_DATA = 256; // Default matrix size (if not provided)

/* Helper function to print a matrix */
void print_matrix(const char* name, float* matrix, size_t size) {
    printf("%s:\n", name);
    for (size_t i = 0; i < size; i++) {
        for (size_t j = 0; j < size; j++) {
            printf("%.2f ", matrix[i * size + j]);
        }
        printf("\n");
    }
    printf("\n");
}

/* Helper function to create the Result directory */
void create_result_directory() {
    struct stat st = {0};
    if (stat("Result", &st) == -1) {
        if (mkdir("Result", 0700) == 0) {
            printf("Result directory created successfully.\n");
        } else {
            perror("Error creating Result directory");
        }
    } else {
        printf("\n ");
    }
}

/* Helper function to export a matrix to a CSV file in the Result directory */
void export_matrix_to_csv(const char* filename, float* matrix, size_t size) {
    char filepath[256];
    snprintf(filepath, sizeof(filepath), "Result/%s", filename);

    FILE* file = fopen(filepath, "w");
    if (!file) {
        fprintf(stderr, "Error opening file %s for writing.\n", filepath);
        return;
    }

    for (size_t i = 0; i < size; i++) {
        for (size_t j = 0; j < size; j++) {
            fprintf(file, "%.6f", matrix[i * size + j]);
            if (j < size - 1) {
                fprintf(file, ",");
            }
        }
        fprintf(file, "\n");
    }

    fclose(file);
}

int main(int argc, char** argv) {
    /* Set the buffer for printf to NULL */
    setbuf(stdout, NULL);

    /* Arguments */
    int nthreads = 1;
    int cpu      = 0;

    int nruns    = 10000;
    int nstdevs  = 3;

    /* Data */
    int data_size = SIZE_DATA; // Holds the size passed to the program

    /* Parse arguments */
    void* (*impl_scalar_naive_ptr)(void* args) = impl_scalar_naive;
    void* (*impl_scalar_opt_ptr)(void* args) = impl_scalar_opt;

    /* Chosen */
    void* (*impl)(void* args) = NULL;
    const char* impl_str      = NULL;

    bool help = false;
    for (int i = 1; i < argc; i++) {
        /* Implementations */
        if (strcmp(argv[i], "-i") == 0 || strcmp(argv[i], "--impl") == 0) {
            assert(++i < argc);
            if (strcmp(argv[i], "naive") == 0) {
                impl = impl_scalar_naive_ptr; impl_str = "scalar_naive";
            } else if (strcmp(argv[i], "opt") == 0) {
                impl = impl_scalar_opt_ptr; impl_str = "scalar_opt";
            } else {
                impl = NULL; impl_str = "unknown";
            }
            continue;
        }

        /* Input/output data size */
        if (strcmp(argv[i], "-s") == 0 || strcmp(argv[i], "--size") == 0) {
            assert(++i < argc); // Ensure an argument follows the flag
            data_size = atoi(argv[i]); // Parse size as an integer
            continue;
        }
    }

    if (help || impl == NULL) {
        printf("Usage: %s {-i | --impl} naive|opt [Options]\n", argv[0]);
        printf("  Options:\n");
        printf("    -h | --help      Print this message\n");
        printf("    -s | --size      Size of input matrices (default = %d)\n", SIZE_DATA);
        exit(help ? 0 : 1);
    }

    /* Create the Result directory */
    create_result_directory();

    /* Initialize matrices */
    size_t size = data_size;  // Square matrix dimensions
    srand((unsigned int)time(NULL)); // Seed the random number generator

    float* input = malloc(2 * size * size * sizeof(float)); // Allocate A and B in contiguous memory
    float* R = malloc(size * size * sizeof(float)); // Allocate result matrix

    if (!input || !R) {
        fprintf(stderr, "Memory allocation failed.\n");
        exit(1);
    }

    /* Set A and B inside input */
    float* A = input;                           // A starts at the beginning
    float* B = input + size * size;             // B starts right after A

    /* Initialize input matrices */
    for (size_t i = 0; i < size * size; i++) {
        A[i] = (float)(rand() % 10);            // Random numbers between 0 and 9
        B[i] = (float)(rand() % 10);            // Random numbers between 0 and 9
    }
    
    /* Print input matrices */
   /* print_matrix("Matrix A", A, size);*/
   /* print_matrix("Matrix B", B, size);*/


    /* Export input matrices */
    export_matrix_to_csv("matrix_A.csv", A, size);
    export_matrix_to_csv("matrix_B.csv", B, size);
   
   /* Compare Naive and Optimized Implementations */
    float* R_naive = malloc(size * size * sizeof(float));
    args_t args_naive = { .input = input, .output = R_naive, .size = size };

    clock_t start = clock();
    impl_scalar_naive(&args_naive);
    clock_t end = clock();
    export_matrix_to_csv("result_naive.csv", R_naive, size);
    double naive_time = (double)(end - start) / CLOCKS_PER_SEC;

    printf("Naive Runtime: %.6f seconds\n", naive_time);
    print_matrix("Result Matrix R (Naive)", R_naive, size);

    args_t args_opt = { .input = input, .output = R, .size = size };

    start = clock();
    impl_scalar_opt(&args_opt);
    end = clock();
    export_matrix_to_csv("result_optimized.csv", R, size);
    double opt_time = (double)(end - start) / CLOCKS_PER_SEC;

    printf("Optimized Runtime: %.6f seconds\n", opt_time);
    print_matrix("Result Matrix R (Optimized)", R, size);

    printf("Speedup: %.2fx\n", naive_time / opt_time);

    /* Free memory */
    free(input);
    free(R);
    free(R_naive);

    return 0;
}
