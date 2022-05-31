#include <iostream>
#include "ucenv.h"

extern "C" {
    void c_initVector(DF &vector, int size, int value) {
        vector.create<int>(size);
        int *vectorData = vector.getData<int>();
        for (int i = 0; i < size; ++i) {
            vectorData[i] = value;
        }
    }

    void c_sumVectors(const DF &vector1, const DF &vector2, DF &resultVector) {
        size_t vectorSize1 = vector1.getSize();
        size_t vectorSize2 = vector2.getSize();
        if (vectorSize1 != vectorSize2) {
            throw std::invalid_argument("Vectors have different length!");
        }

        size_t resultVectorSize = vectorSize1 / sizeof(int);
        const int *vector1Data = vector1.getData<int>();
        const int *vector2Data = vector2.getData<int>();
        resultVector.create<int>(resultVectorSize);
        int *resultVectorData = resultVector.getData<int>();
        for (size_t i = 0; i < resultVectorSize; ++i) {
            resultVectorData[i] = vector1Data[i] + vector2Data[i];
        }
    }

    void c_printVector(const DF &vector, int partIndex) {
        int size = vector.getSize() / sizeof(int);
        const int *vectorData = vector.getData<int>();
        printf("Vector with index part %d is [", partIndex);
        for (int i = 0; i < size - 1; ++i) {
            printf("%d ", vectorData[i]);
        }
        printf("%d]\n", vectorData[size - 1]);
    }
}