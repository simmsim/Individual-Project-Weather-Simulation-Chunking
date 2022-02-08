#include <iostream>
#include <stdlib.h>
#include <math.h>
#include <chrono>
#include <string.h>

#include "sor_params.h"
#include "../utility/array_index_f2c.h"
#include "../utility/print_exec_time_to_file.h"

void sor(float *p0,float *p1,float *rhs) {
    const float cn1 = 1.0 / 3.0;
    const float cn2l = 0.5;
    const float cn2s = 0.5;
    const float cn3l = 0.5;
    const float cn3s = 0.5;
    const float cn4l = 0.5;
    const float cn4s = 0.5;
    const float omega = 1.0;

    for (int k = 0; k <= kp + 1; k++) {
        for (int j = 0; j <= jp + 1; j++) {
            for (int i = 0; i <= ip + 1; i++) {
                // bottom-condition
                if (k == 0) {
                    p0[F3D2C(ip+2, jp+2, 0,0,0, i,j,0)] = p0[F3D2C(ip+2, jp+2, 0,0,0, i,j,1)];
                } else if (k == kp + 1) {
                    // bottom-condition
                    p0[F3D2C(ip+2, jp+2, 0,0,0, i,j,kp+1)] = p0[F3D2C(ip+2, jp+2, 0,0,0, i,j,kp)];
                } else if (j == 0 && (k > 0 && k < kp + 1)) {
                    // circular, left
                    p0[F3D2C(ip+2, jp+2, 0,0,0, i,0,k)] = p0[F3D2C(ip+2, jp+2, 0,0,0, i,jp,k)];
                } else if (j == jp + 1 && (k > 0 && k < kp + 1)) {
                    // circular, right
                    p0[F3D2C(ip+2, jp+2, 0,0,0, i,jp+1,k)] = p0[F3D2C(ip+2, jp+2, 0,0,0, i,1,k)];
                } else if (i == 0 && (k > 0 && k < kp + 1)) {
                    // in-flow
                    p0[F3D2C(ip+2, jp+2, 0,0,0, 0,j,k)] = p0[F3D2C(ip+2, jp+2, 0,0,0, 1,j,k)]; 
                } else if (i == ip + 1 && (k > 0 && k < kp + 1)) {
                    // out-flow
                    p0[F3D2C(ip+2, jp+2, 0,0,0, ip+1,j,k)] = p0[F3D2C(ip+2, jp+2, 0,0,0, ip,j,k)]; 
                } 
            }
        }
    }

    for (int k = 0; k <= kp + 1; k++) {
        for (int j = 0; j <= jp + 1; j++) {
            for (int i = 0; i <= ip + 1; i++) {
                if ((i > 0 && i < ip + 1) && (j > 0 && j < jp + 1) && (k > 0 && k < kp + 1)) {
                    float relmtp = omega*(cn1*(
                        cn2l*p0[F3D2C(ip+2, jp+2, 0,0,0, i+1,j,k)] +
                        cn2s*p0[F3D2C(ip+2, jp+2, 0,0,0, i-1,j,k)] +
                        cn3l*p0[F3D2C(ip+2, jp+2, 0,0,0, i,j+1,k)] +
                        cn3s*p0[F3D2C(ip+2, jp+2, 0,0,0, i,j-1,k)] +
                        cn4l*p0[F3D2C(ip+2, jp+2, 0,0,0, i,j,k+1)] +
                        cn4s*p0[F3D2C(ip+2, jp+2, 0,0,0, i,j,k-1)] -
                        rhs[F3D2C(ip+2, jp+2, 0,0,0, i,j,k)]) - 
                        p0[F3D2C(ip+2, jp+2, 0,0,0, i,j,k)]);

                    p1[F3D2C(ip+2, jp+2, 0,0,0, i,j,k)] = p0[F3D2C(ip+2, jp+2, 0,0,0, i,j,k)] + relmtp;
                }
            }
        }
    }
}

int main(int argc, char* argv[]) {
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
                p1[F3D2C(ip+2,jp+2,0,0,0,i,j,k)] = ((float)(i+1)*(j+1)*(k+1))/((float)(ip+2)*(jp+2)*(kp+2));
            }
        }
    }

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

    char * fileName = (char *)malloc(strlen(argv[1]) + 1);
    strcpy(fileName, argv[1]);

    print_to_file(begin, end, fileName);
    // std::cout << p0[F3D2C(ip+2,jp+2, 0,0,0,ip/2,jp/2,kp/2)] << "\n";

    free(fileName);
    delete[] p0;
    delete[] p1;
    delete[] rhs;
}