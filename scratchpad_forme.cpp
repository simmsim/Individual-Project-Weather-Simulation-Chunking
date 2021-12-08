void simpleCompute1D(float * ch1, float * ch2) {
    int iDim = 10;
    int jDim = 1;
    int kDim = 1;

    int iHalChunk = 4;

    for (int k = 0; k < kDim; k++) {
        for (int j = 0; j < jDim; j++) {
            for (int i = 1; i < iHalChunk-1; i++) {
                ch2[i] = ch1[i-1] + ch1[i] + ch1[i+1];
            }
        }
    }
}

inline unsigned int F2D2C(
        unsigned int i_rng, // ranges, i.e. (hb-lb)+1
        int i_lb, int j_lb, // lower bounds
        int ix, int jx
        ) {
    return (i_rng*(jx-j_lb)+ix-i_lb);
}

void simpleCompute2D(float * ch1, float * ch2) {
    int iHalChunk = 4;
    int jHalChunk = 6;
    int kHalChunk = 1;

    for (int k = 0; k < kHalChunk; k++) {
        for (int j = 1; j < jHalChunk-1; j++) {
            for (int i = 1; i < iHalChunk-1; i++) {
                ch2[F2D2C(iHalChunk, 0, 0, i, j)] = ch1[F2D2C(iHalChunk, 0, 0, i, j)]
                                                + ch1[F2D2C(iHalChunk, 0, 0, i-1, j)]
                                                + ch1[F2D2C(iHalChunk, 0, 0, i+1, j)]
                                                + ch1[F2D2C(iHalChunk, 0, 0, i, j-1)]
                                                + ch1[F2D2C(iHalChunk, 0, 0, i, j+1)];
            }
        }
    }
}