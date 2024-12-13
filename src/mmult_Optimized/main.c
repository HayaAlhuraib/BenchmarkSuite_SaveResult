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

/* Helper function to print a matrix */
void print_matrix(const char* name, float* matrix, size_t rows, size_t cols) {
    printf("%s:\n", name);
    for (size_t i = 0; i < rows; i++) {
        for (size_t j = 0; j < cols; j++) {
            printf("%.2f ", matrix[i * cols + j]);
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
    }
}

/* Helper function to export a matrix to a CSV file in the Result directory */
void export_matrix_to_csv(const char* filename, float* matrix, size_t rows, size_t cols) {
    char filepath[256];
    snprintf(filepath, sizeof(filepath), "Result/%s", filename);

    FILE* file = fopen(filepath, "w");
    if (!file) {
        fprintf(stderr, "Error opening file %s for writing.\n", filepath);
        return;
    }

    for (size_t i = 0; i < rows; i++) {
        for (size_t j = 0; j < cols; j++) {
            fprintf(file, "%.6f", matrix[i * cols + j]);
            if (j < cols - 1) {
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

    /* Default settings */
    int nthreads = 1;
    int cpu = 0;
    size_t rows_A = 2;      // Default size
    size_t cols_A = 2;
    size_t rows_B = 2;
    size_t cols_B = 2;

    /* Function pointers for implementations */
    void* (*impl)(void* args) = NULL;
    const char* impl_str = NULL;

    /* Parse command-line arguments */
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "-i") == 0 || strcmp(argv[i], "--impl") == 0) {
            assert(++i < argc);
            if (strcmp(argv[i], "naive") == 0) {
                impl = impl_scalar_naive;
                impl_str = "naive";
            } else if (strcmp(argv[i], "opt") == 0) {
                impl = impl_scalar_opt;
                impl_str = "opt";
            } else {
                fprintf(stderr, "Unknown implementation: %s\n", argv[i]);
                exit(1);
            }
            continue;
        }

        if (strcmp(argv[i], "-s") == 0 || strcmp(argv[i], "--size") == 0) {
            assert(++i < argc);
            rows_A = cols_A = rows_B = cols_B = atoi(argv[i]);
            continue;
        }
    }

    if (impl == NULL) {
        fprintf(stderr, "Usage: %s -i {naive|opt} [-s size]\n", argv[0]);
        exit(1);
    }

    /* Create the Result directory */
    create_result_directory();

    /* Allocate matrices */
    srand((unsigned int)time(NULL)); // Seed the random number generator

    float* A = malloc(rows_A * cols_A * sizeof(float));
    float* B = malloc(rows_B * cols_B * sizeof(float));
    float* R = malloc(rows_A * cols_B * sizeof(float));

    if (!A || !B || !R) {
        fprintf(stderr, "Memory allocation failed.\n");
        exit(1);
    }

    /* Initialize input matrices */
    for (size_t i = 0; i < rows_A * cols_A; i++) {
        A[i] = (float)(rand() % 10);
    }

    for (size_t i = 0; i < rows_B * cols_B; i++) {
        B[i] = (float)(rand() % 10);
    }

    /* Print input matrices */
    print_matrix("Matrix A", A, rows_A, cols_A);
    print_matrix("Matrix B", B, rows_B, cols_B);

    /* Export input matrices */
    export_matrix_to_csv("matrix_A.csv", A, rows_A, cols_A);
    export_matrix_to_csv("matrix_B.csv", B, rows_B, cols_B);

    /* Prepare arguments */
    args_t args = {
        .input = malloc((rows_A * cols_A + rows_B * cols_B) * sizeof(float)),
        .output = R,
        .size = rows_A
    };

    memcpy(args.input, A, rows_A * cols_A * sizeof(float));
    memcpy((float*)args.input + rows_A * cols_A, B, rows_B * cols_B * sizeof(float));

    /* Run the selected implementation */
    clock_t start = clock();
    impl(&args);
    clock_t end = clock();

    double runtime = (double)(end - start) / CLOCKS_PER_SEC;

    /* Export result matrix */
    export_matrix_to_csv("result.csv", R, rows_A, cols_B);

    /* Print result matrix and runtime */
    print_matrix("Result Matrix R", R, rows_A, cols_B);
    printf("%s Implementation Runtime: %.6f seconds\n", impl_str, runtime);

    /* Free memory */
    free(A);
    free(B);
    free(R);
    free(args.input);

    return 0;
}
