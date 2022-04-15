#include <iostream>
#include "df.h"
#include <mpi.h>

#define ROOT_RANK 0

#define N 6
#define M 6

void initMatrix(DF &matrix, double value) {
    matrix.create<double>(N * M);
    auto *ptrData = matrix.getData<double>();
    for (int i = 0; i < N; ++i) {
        for (int j = 0; j < M; ++j) {
            ptrData[i * M + j] = value;
        }
    }
}

void initVector(DF &vector, double value) {
    vector.create<double>(M);
    double *ptrData = vector.getData<double>();
    for (int i = 0; i < M; ++i) {
        ptrData[i] = value;
    }
}

// May be default operator
void getCountPartsForProcess(int *partElementsForProcess, const int countElements, const int rank, const int size) {
    *partElementsForProcess = countElements / size;
    int r = countElements % size;
    if (rank < r) {
        ++*partElementsForProcess;
    }
}

// May be default operator
void getCountPartsForEachProcess(int *partElementsForProcess, const int countElements, const int rank, const int size) {
    int countElementsForEachProcess = countElements / size;
    int r = countElements % size;
    for (int i = 0; i < size; ++i) {
        partElementsForProcess[i] = countElementsForEachProcess;
    }
    for (int i = 0; i < r; ++i) {
        ++partElementsForProcess[i];
    }
}

void getShiftTable(int *shiftTable, const int *partElementsForProcess, const int size) {
    shiftTable[0] = 0;
    for (int i = 1; i < size; ++i) {
        shiftTable[i] = partElementsForProcess[i - 1] + shiftTable[i - 1];
    }
}

int main(int argc, char **argv) {
    MPI_Init(&argc, &argv);
    int rank, size;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);

    DF matrix, vector, result;
    if (rank == ROOT_RANK) {
        initMatrix(matrix, 1.0f);
        initVector(vector, 1.0f);
    }

    int countPartsForProc[size];
    int shiftTable[size];
    getCountPartsForEachProcess(countPartsForProc, N, rank, size);
    getShiftTable(shiftTable, countPartsForProc, size);

    /*
    if (rank == ROOT_RANK) {
        std::cout << "Shift table" << std::endl;
        for (int i = 0; i < size; ++i) {
            std::cout << shiftTable[i] << std::endl;
        }
    } */

    DF partMatrix;
    partMatrix.create<double>(countPartsForProc[rank]);

    double * data = (double *)malloc(sizeof(double) * countPartsForProc[rank]);
    MPI_Scatterv(matrix.getData<double>(), countPartsForProc, shiftTable, MPI_DOUBLE, data, countPartsForProc[rank], MPI_DOUBLE, ROOT_RANK, MPI_COMM_WORLD);

    std::cout << "Rank #" << rank << std::endl;
    for (int i = 0; i < M; ++i) {
        std::cout << data[i] << std::endl;
    }

    MPI_Finalize();
}