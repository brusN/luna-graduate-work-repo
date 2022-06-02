#include <cstdio>
#include <cstring>
#include "ucenv.h"

using namespace luna::ucenv;

struct Matrix {
	size_t Height, Width;
	double* Data;
};

void create_matr_buf(OutputDF &out, const int &height, const int &width) {
    out.create(2*sizeof(size_t) + height*width*sizeof(double));


    size_t *data = static_cast<size_t *>(out.get_data());
    data[0]=height;
    data[1]=width;
}

void* shift(void* buf, int ofs) {
    return (void*)(((char*)buf) + ofs);
}

void mf_init_random(struct Matrix *m, const double &min_value, const double &max_value) {
	assert(m!=NULL);
	assert(max_value>=min_value);
	assert(m->Height>0 && m->Width>0 && m->Data!=NULL);

    for (size_t i=0; i<m->Height; i++) {
        for (size_t j=0; j<m->Width; j++) {
            m->Data[i*m->Width+j]=min_value+rand()*(max_value-min_value)/RAND_MAX;
        }
    }
}

void attach(struct Matrix &A, const InputDF &a) {
    const size_t *data=static_cast<const size_t *>(a.get_data());
    A.Height=data[0];
    A.Width=data[1];
    A.Data=(double*)(data+2);
}

extern "C"
{
    void c_init(int val, OutputDF &df)
    {
        df.setValue<int>(val);
        printf("c_init: %d --> %s, size: %d\n", val, df.getName().c_str(), (int)df.getSize());
    }

    void c_init_submatrix(const char* /*file*/, int /*row*/, int /*col*/, int height, int width, OutputDF &dest) 
    {
	    struct Matrix m;

	    create_matr_buf(dest, height, width);    
	    m.Height=height;
	    m.Width=width;	
        m.Data=(double*)(static_cast<const size_t *>(dest.get_data())+2);

	    mf_init_random(&m, 1, 2);
    }


    void c_copy_matrix(const InputDF &a, OutputDF &b) 
    {
        b.copy(a);
    }

    void c_sum_matrix(const InputDF &a, const InputDF &b, OutputDF &c) 
    {
	    struct Matrix A, B, C;
	    attach(A, a);
	    attach(B, b);
	    assert(A.Height == B.Height && A.Width == B.Width);
	    create_matr_buf(c, A.Height, A.Width);
	    C.Height = A.Height;
	    C.Width = B.Width;
        C.Data = (double*)(static_cast<const size_t *>(c.get_data())+2);

        for (size_t i=0; i<C.Height*C.Width; i++)
		    C.Data[i]=A.Data[i]+B.Data[i];
    }

    void c_mult_matrix(const InputDF &a, const InputDF &b, OutputDF &c)
    {
	    struct Matrix A, B;

        printf("c_mult_matrix %s %s %s\n", a.getName().c_str(), b.getName().c_str(), c.getName().c_str());

	    attach(A, a);
	    attach(B, b);

	    create_matr_buf(c, A.Height, B.Width);
        printf("c_mult_matrix %s %s %s\n", a.getName().c_str(), b.getName().c_str(), c.getName().c_str());

        double* Csubm = (double*)(static_cast<const size_t *>(c.get_data())+2);

        printf("A: %dx%d\n", (int)A.Height, (int)A.Width);
        printf("B: %dx%d\n", (int)B.Height, (int)B.Width);

        for (size_t i=0; i<A.Height; i++) {
            for (size_t j=0; j<B.Width; j++) {
                for (size_t k=0; k<A.Width; k++) {
                    Csubm[i*B.Width+j]+=A.Data[i*A.Width+k]*B.Data[k*B.Width+j];
                }
            }
        }
    }
}