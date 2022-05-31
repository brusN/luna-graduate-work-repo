#include <iostream>
#include "df.h"

extern "C" {
    void c_initVector(DF& vector, int size, int value)
    {
        vector.create<int>(size);
        int* vectorData = vector.getData<int>();
        for (int i = 0; i < size; ++i)
        {
            vectorData[i] = value;
        }
    }

    void c_mulVectors(const DF& vector1, const DF& vector2, DF& result)
    {
        size_t vector_size1 = vector1.getSize();
        size_t vector_size2 = vector2.getSize();
        if (vector_size1 != vector_size2)
        {
            throw std::invalid_argument("Vectors have different length!");
        }

        size_t result_vector_size = vector_size1 / sizeof(int);
        const int* vector1_data = vector1.getData<int>();
        const int* vector2_data = vector2.getData<int>();
        int scalar = 0;
        for (int i = 0; i < result_vector_size; i++)
        {
            scalar += vector1_data[i] * vector2_data[i];
        }
        result.setValue(scalar);
    }

    void c_sum_result(const DF& a, const DF& b, DF& c)
    {
		int a_ = a.getValue<int>();
		int b_ = b.getValue<int>();
        c.setValue(a_ + b_);
    }

    void c_copy_result(const DF& a, DF& b)
    {
        b.copy(a);
    }

    void c_printResult(const DF &res)
    {
        printf("Scalar: %d\n", res.getValue<int>());
    }
 }