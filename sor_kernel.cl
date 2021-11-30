#define PERIODIC 1
#define INFLOW 2
#define OUTFLOW 3
#define TOPBOTTOM 4
#define CORE 5

inline unsigned int F4D2C(unsigned int i_rng,unsigned int j_rng, unsigned int k_rng, // ranges, i.e. (hb-lb)+1
                          int i_lb, int j_lb, int k_lb, int l_lb, // lower bounds
                          int ix, int jx, int kx, int lx) 
{
    return (i_rng*j_rng*k_rng*(lx-l_lb)+
            i_rng*j_rng*(kx-k_lb)+
            i_rng*(jx-j_lb)+
            ix-i_lb
            );
}

inline unsigned int FTNREF3D0(int ix, int jx, int kx, unsigned int iz,unsigned int jz) {
        return iz*jz*kx+iz*jx+ix ;
}

/* 
An array with floats including a 1-point halo in every direction p(0:ip+1, 0:jp+1, 0:kp+1)
Where the actual core region is from 1 to ip/jp/kp.
*/

/*
Compute the periodic condition, i.e. initialize side halos
p_in(0, i, 0, k) = p_in(0, i, jp, k)
p_in(0, i, jp+1, k) = p_in(0, i, 1, k)
*/
void compute_periodic_condition(__global float *p, const int iChunk, 
                                const int jChunk, const int kChunk) 
{
    // How do we know that we're on the side of the chunk?
    // - When a point is on halo: j = 0 || j = jp + 1(given it's a 1-point halo, 
    // - We'll do pointless periodic boundary calculations for bottom and top 
    //  hallo planes, so that we don't need to do additional checks.
    
    int global_id = get_global_id(0);
    unsigned int ip = iChunk;
    unsigned int jp = jChunk;

    int i_range = iChunk + 2;
    int j_range = jChunk + 2;
    int k_range = kChunk + 2;
    
    int k = global_id/(i_range*j_range);
    int i = global_id-(k*i_range);

    p[FTNREF3D0(i, 0, k, ip+2, jp+2)] = p[FTNREF3D0(i, jp, k, ip+2, jp+2)];
    p[FTNREF3D0(i, jp+1, k, ip+2, jp+2)] = p[FTNREF3D0(i, 1, k, ip+2, jp+2)];
}

/*
Compute the open condition for the in-flow plane
p_in(0, 0, j, k) = p_in(0, 1, j, k)
*/
void compute_inflow_condition(__global float *p_in) {}

/*
Compute the open condition for the out-flow plane
p_in(0, ip+1, j, k) = p_in(0, ip, j, k)
*/
void compute_outflow_condition(__global float *p_in) {}

/* 
Compute the open condition for top and bottom planes
p_in(0, i, j, 0) = p(0, i, j, 1)
p_in(0, i, j, kp+1) = p_in(0, i, j, kp)
*/
void compute_top_bottom_conditions(__global float *p_in) {}

/*
Compute new values in the core region
*/
void compute_core_region(__global float *p_in, __global float *p_out) 
{
    // 1. Get i,j,k
    // 2. Do stencil computation involving 6 neighboring points.
    // 2.1. Get input neighoring points from p_in and place the output in p_out
}

__kernel void sor_superkernel(__global float *p_in, __global float *p_out, int state) 
{
    switch (state) {
        case CORE:
            compute_core_region(p_in, p_out);
            break;
        case PERIODIC:
           // compute_periodic_condition(p_in);
            break;
        case INFLOW:
            compute_inflow_condition(p_in);
            break;
        case OUTFLOW:
            compute_outflow_condition(p_in);
            break;
        case TOPBOTTOM:
            compute_top_bottom_conditions(p_in);
            break;
    }
}