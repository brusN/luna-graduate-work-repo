#include "ucenv.h"
#include <iostream>

extern "C" {
    void c_initMatrixByValue(DF &matrix, int N, int M, double value) {
        matrix.create<double>(N * M);
        auto *ptrData = matrix.getData<double>();
        for (int i = 0; i < N; ++i) {
           for (int j = 0; j < M; ++j) {
              ptrData[i * M + j] = value;
          }
        }
    }

    void c_initVectorByValue(DF &vector, int N, double value) {
        vector.create<double>(N);
        double *ptrData = vector.getData<double>();
        for (int i = 0; i < N; ++i) {
            ptrData[i] = value;
        }
    }

    void c_mulMatrixOnVector(const DF &matrix, const DF &vector, int N, int M, DF &resultVector) {
    
        const double *matrixData = matrix.getData<double>(); 
        const double *vectorData = vector.getData<double>(); 
        resultVector.create<double>(N);
        double *resultVectorData = resultVector.getData<double>(); 
        for (int i = 0; i < N; ++i) {
            resultVectorData[i] = 0;
            for (int j = 0; j < M; ++j) {
                resultVectorData[i] += matrixData[i * M + j] * vectorData[j];
            }
        }
    }

    void c_printVector(const DF &vector, int size) {
        const double *vectorData = vector.getData<double>();
        std::cout << "Vector is [";
        for (int i = 0; i < size - 1; ++i) {
            std::cout << vectorData[i] << " ";
        }
        std::cout << vectorData[size - 1] << "]" << std::endl;
    }

    void c_concatParts(const DF &part1, int partSize1, const DF &part2, int partSize2, DF &concatedParts) {
        const double *partData1 = part1.getData<double>();
        const double *partData2 = part1.getData<double>();
        concatedParts.create<double>(partSize1 + partSize2);
        double *concatedPartsData = concatedParts.getData<double>(); 
        for (int i = 0; i < partSize1; ++i) {
            concatedPartsData[i] = partData1[i];
        }
        for (int i = 0; i < partSize2; ++i) {
            concatedPartsData[i + partSize1] = partData1[i];
        }
    }
}