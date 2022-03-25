#include <iostream>
#include <stdlib.h>
#include <math.h>
#include <chrono>
#include <string.h>
//#include <omp.h>

#include "sor_params.h"
#include "../utility/array_index_f2c.h"
#include "../utility/print_exec_time_to_file.h"

void sor(float *p0,float *p1,float *rhs);

void sor(float *p0,float *p1,float *rhs) {
    const float cn1 = 1.0 / 3.0;
    const float cn2l = 0.5;
    const float cn2s = 0.5;
    const float cn3l = 0.5;
    const float cn3s = 0.5;
    const float cn4l = 0.5;
    const float cn4s = 0.5;
    const float omega = 1.0;

    for (int i = 0; i <= ip + 1; i++) {
        for (int j = 0; j <= jp + 1; j++) {
            for (int k = 0; k <= kp + 1; k++) {
                // bottom-condition
                if (k == 0) {
                    p0[F3D2C(kp+2, jp+2, 0,0,0, 0,j,i)] = p0[F3D2C(kp+2, jp+2, 0,0,0, 1,j,i)];
                } 
                if (k == ip + 1) {
                    // bottom-condition
                    p0[F3D2C(kp+2, jp+2, 0,0,0, kp+1,j,i)] = p0[F3D2C(kp+2, jp+2, 0,0,0, kp,j,i)];
                 }
                  if (j == 0 && (k > 0 && k < ip + 1)) {
                    // circular, left
                    p0[F3D2C(kp+2, jp+2, 0,0,0, k,0,i)] = p0[F3D2C(kp+2, jp+2, 0,0,0, k,jp,i)];
                }
                if (j == jp + 1 && (k > 0 && k < ip + 1)) {
                    // circular, right
                    p0[F3D2C(kp+2, jp+2, 0,0,0, k,jp+1,i)] = p0[F3D2C(kp+2, jp+2, 0,0,0, k,1,i)];
                } 
                if (i == 0 && (k > 0 && k < ip + 1)) {
                    // in-flow
                    p0[F3D2C(kp+2, jp+2, 0,0,0, k,j,0)] = p0[F3D2C(kp+2, jp+2, 0,0,0, k,j,1)]; 
                }
                if (i == ip + 1 && (k > 0 && k < ip + 1)) {
                    // out-flow
                    p0[F3D2C(kp+2, jp+2, 0,0,0, k,j,ip+1)] = p0[F3D2C(kp+2, jp+2, 0,0,0, k,j,ip)];  
                } 
            }
        }
    }

    for (int i = 1; i <= ip; i++) {
        for (int j = 1; j <= jp; j++) {
            for (int k = 1; k <= kp; k++) {
                float relmtp = omega*(cn1*(
        cn2l*p0[F3D2C(kp+2, jp+2, 0,0,0, k,j,i+1)] +
        cn2s*p0[F3D2C(kp+2, jp+2, 0,0,0, k,j,i-1)] +
        cn3l*p0[F3D2C(kp+2, jp+2, 0,0,0, k,j+1,i)] +
        cn3s*p0[F3D2C(kp+2, jp+2, 0,0,0, k,j-1,i)] +
        cn4l*p0[F3D2C(kp+2, jp+2, 0,0,0, k+1,j,i)] +
        cn4s*p0[F3D2C(kp+2, jp+2, 0,0,0, k-1,j,i)] -
        rhs[F3D2C(kp+2, jp+2, 0,0,0, k,j,i)]) - 
        p0[F3D2C(kp+2, jp+2, 0,0,0, k,j,i)]);
    
        p1[F3D2C(kp+2, jp+2, 0,0,0, k,j,i)] = p0[F3D2C(kp+2, jp+2, 0,0,0, k,j,i)] + relmtp;
            }
        }
    }
}
    

int main(int argc, char* argv[]) {
    #include "sor_params.h"
    float *p0 = new float[(int64_t)(ip+2)*(int64_t)(jp+2)*(int64_t)(kp+2)];
    float *p1 = new float[(int64_t)(ip+2)*(int64_t)(jp+2)*(int64_t)(kp+2)];
    float *rhs = new float[(int64_t)(ip+2)*(int64_t)(jp+2)*(int64_t)(kp+2)];

    int i;
    int j;
    int k;
    for (i = 0;i <= ip+1;i += 1) {
        for (j = 0;j <= jp+1;j += 1) {
            for (k = 0;k <= kp+1;k += 1) {
                rhs[F3D2C(ip+2,jp+2,0,0,0,i,j,k)] = 0.1+((float)(i+1)*(j+1)*(k+1))/((float)(ip+2)*(jp+2)*(kp+2));
                p0[F3D2C(ip+2,jp+2,0,0,0,i,j,k)] = ((float)(i+1)*(j+1)*(k+1))/((float)(ip+2)*(jp+2)*(kp+2));
                //p1[F3D2C(ip+2,jp+2,0,0,0,i,j,k)] = ((float)(i+1)*(j+1)*(k+1))/((float)(ip+2)*(jp+2)*(kp+2));
            }
        }
    }
    // for (int z = 0; z < (int64_t)(ip+2)*(int64_t)(jp+2)*(int64_t)(kp+2); z++) {
    //      p0[z] = z;
    //     // p1[z] = z;
    //     // std::cout << " " << p0[z];
    // }

    int iter;
    int niters = atoi(argv[2]);

    std::chrono::steady_clock::time_point begin = std::chrono::steady_clock::now();
    for (iter = 1;iter <= niters;iter += 1) {
        if (iter % 2 == 0) {
            sor(p1, p0, rhs);
        } else {
            sor(p0, p1, rhs);
        }        
    }
    std::chrono::steady_clock::time_point end = std::chrono::steady_clock::now();
    // for (int i = 0; i < (int64_t)(ip+2)*(int64_t)(jp+2)*(int64_t)(kp+2); i++) {
    //     std::cout << " " << p1[i];
    // }
    // std::cout << "\n";

    char * fileName = (char *)malloc(strlen(argv[1]) + 1);
    strcpy(fileName, argv[1]);

    print_to_file(begin, end, fileName);
    int index = F3D2C(kp+2, jp+2, 0,0,0, 1,1,1);
    float actualValue =  p0[index];
    std::cout << "Value p0 " << actualValue << " for index " << index << "\n";
    std::cout << "Value p1 " << p1[index] << " for index " << index << "\n";

    free(fileName);
    delete[] p0;
    delete[] p1;
    delete[] rhs;
}
    