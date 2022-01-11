#include <iostream>
#include <stdlib.h>
#include <math.h>
#include "../utility/array_index_f2c.h"

void sor(float *p0,float *p1,float *rhs) {
#include "sor_params.h"

    int i;
    int j;
    int k;
    const float cn1 = 1.0 / 3.0;
    const float cn2l = 0.5;
    const float cn2s = 0.5;
    const float cn3l = 0.5;
    const float cn3s = 0.5;
    const float cn4l = 0.5;
    const float cn4s = 0.5;
    const float omega = 1.0;
    float reltmp;
    for (i = 0;i <= im+1;i += 1) {
        for (j = 0;j <= jm+1;j += 1) {
            for (k = 0;k <= km+1;k += 1) {
                //std::cout << "("<<i <<","<< j <<","<< k << ");"<<F3D2C(im+2,jm+2,0,0,0,i,j,k)<<"\n";
                //  assume i=x =  west to east, y=j = south to north, k=z = vertical
                if (i==im+1) {
                    //std::cout <<"i==im+1\n";
                //  circular
                //  i=im+1
                    p1[F3D2C(im+2,jm+2,0,0,0,i,j,k)] = p0[F3D2C(im+2,jm+2,0,0,0,i-im,j,k)];
                } else if (i==0) {
                    //std::cout <<"i==0\n";
                //  i=0
                //  circular
                    p1[F3D2C(im+2,jm+2,0,0,0,i,j,k)] = p0[F3D2C(im+2,jm+2,0,0,0,i+im,j,k)];
                } else if (j==jm+1) {
                    //std::cout <<"j==jm+1\n";
                //  open
                //  j = jm+1
                    p1[F3D2C(im+2,jm+2,0,0,0,i,j,k)] = p0[F3D2C(im+2,jm+2,0,0,0,i-1,j,k)];
                } else if (j==0) {
                    //std::cout <<"j==0\n";
                //  fixed
                //  j = 0
                //  We keep the original values
                    p1[F3D2C(im+2,jm+2,0,0,0,i,j,k)] = p0[F3D2C(im+2,jm+2,0,0,0,i,j,k)];
                 } else if (i>0 && i<im+1 && j>0 && j<jm+1 && k>0 && k<km+1) {
                    //std::cout <<"core\n";
                //  the core
                //  The actual SOR expression
                    reltmp = omega*(cn1*(
                                cn2l*p0[F3D2C(im+2,jm+2,0,0,0,i+1,j,k)]+
                                cn2s*p0[F3D2C(im+2,jm+2,0,0,0,i-1,j,k)]+
                                cn3l*p0[F3D2C(im+2,jm+2,0,0,0,i,j+1,k)]+
                                cn3s*p0[F3D2C(im+2,jm+2,0,0,0,i,j-1,k)]+
                                cn4l*p0[F3D2C(im+2,jm+2,0,0,0,i,j,k+1)]+
                                cn4s*p0[F3D2C(im+2,jm+2,0,0,0,i,j,k-1)]-
                                rhs[F3D2C(im+2,jm+2,0,0,0,i,j,k)])-
                            p0[F3D2C(im+2,jm+2,0,0,0,i,j,k)]);
                    p1[F3D2C(im+2,jm+2,0,0,0,i,j,k)] = p0[F3D2C(im+2,jm+2,0,0,0,i,j,k)]+reltmp;
                }
             }
         }
    }
}
    

int main() {
    #include "sor_params.h"

    float *p0 = new float[(int64_t)(im+2)*(int64_t)(jm+2)*(int64_t)(km+2)];
    float *p1 = new float[(int64_t)(im+2)*(int64_t)(jm+2)*(int64_t)(km+2)];
    float *rhs = new float[(int64_t)(im+2)*(int64_t)(jm+2)*(int64_t)(km+2)];
    int iter;
    const int niters = 50;
    int i;
    int j;
    int k;
    for (i = 0;i <= im+1;i += 1) {
        for (j = 0;j <= jm+1;j += 1) {
            for (k = 0;k <= km+1;k += 1) {
                rhs[F3D2C(im+2,jm+2,0,0,0,i,j,k)] = 0.1+((i+1)*(j+1)*(k+1))/((im+2)*(jm+2)*(km+2));
                p0[F3D2C(im+2,jm+2,0,0,0,i,j,k)] = ((i+1)*(j+1)*(k+1))/((im+2)*(jm+2)*(km+2));
                p1[F3D2C(im+2,jm+2,0,0,0,i,j,k)] = ((i+1)*(j+1)*(k+1))/((im+2)*(jm+2)*(km+2));
                //p0[F3D2C(im+2,jm+2,0,0,0,i,j,k)] = 1.0;
                // p1[F3D2C(im+2,jm+2,0,0,0,i,j,k)] = 1.0;
            }
        }
    }
    clock_t total_start = clock();
    
    for (iter = 1;iter <= niters;iter += 1) {
        if (iter % 2 == 0) {
            sor(p1, p0, rhs);
        } else {
            sor(p0, p1, rhs);
        }        
    }

    clock_t total_end = clock();
    double total_time = (double)(total_end - total_start) / CLOCKS_PER_SEC;
    std::cout << p0[F3D2C(im+2,jm+2, 0,0,0,im/2,jm/2,km/2)] << "\n";
    std::cout << "Total: " << total_time << std::endl;
    delete[] p0;
    delete[] p1;
    delete[] rhs;
}