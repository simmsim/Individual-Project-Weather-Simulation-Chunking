#define PERIODIC 1
#define INFLOW 2
#define OUTFLOW 3
#define TOPBOTTOM 4
#define CORE 5

// Calculates the position, evaluated by the effect of the loop ordering
inline int4 calc_loop_iters(int idx, int im, int jm, int km, int i_off, int j_off, int k_off) 
{
    int4 ijk;
    
#if LOOP_ORDER == 1     
    // jki 
    int ik_range = km*im;
    ijk.s1=  j_off + idx/ik_range; //j
    ijk.s2 = k_off + idx % ik_range / im; //k
    ijk.s0 = i_off + idx % im; //i
#elif LOOP_ORDER == 2
    // ijk: j -> i, k -> j, i-> k
    int kj_range = km*jm;
    ijk.s0=  i_off + idx/kj_range; //i
    ijk.s1 = j_off + idx % kj_range / km; //j
    ijk.s2 = k_off + idx % km; //k
#elif LOOP_ORDER == 3 // This cause an out-or-resources error for Phys1!
    // kij: j -> k, k -> i, i -> j 
    int ji_range = jm*im;
    ijk.s2=  k_off + idx/ji_range; //k
    ijk.s0 = i_off + idx % ji_range / jm; //i
    ijk.s1 = j_off + idx % jm; //j
#elif LOOP_ORDER == 4
    // jik: j -> j, k -> i, i-> k
    int ik_range = km*im;
    ijk.s1=  j_off + idx/ik_range; //j
    ijk.s0 = i_off + idx % ik_range / km; //i
    ijk.s2 = k_off + idx % km; //k
#elif LOOP_ORDER == 5
    // ikj: j->i, k->k,i->j 
    int jk_range = km*jm;
    ijk.s0=  i_off + idx/jk_range; //i
    ijk.s2 = k_off + idx % jk_range / jm; //k
    ijk.s1 = j_off + idx % jm; //j
#elif LOOP_ORDER == 6
    // kji: j->k,k->j,i->i    
    int ij_range = jm*im;
    ijk.s2=  k_off + idx/ij_range; //k
    ijk.s1 = j_off + idx % ij_range / im; //j
    ijk.s0 = i_off + idx % im; //i
#endif    
    return ijk;
}

// From 4D in Fortran to 1D in C
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

/* 
An array with floats including a 1-point halo in every direction p(0:ip+1, 0:jp+1, 0:kp+1)
Where the actual core region is from 1 to ip/jp/kp.
*/

/*
Compute the periodic condition, i.e. initialize side halos
p_in(0, i, 0, k) = p_in(0, i, jp, k)
p_in(0, i, jp+1, k) = p_in(0, i, 1, k)
*/
void compute_periodic_condition(__global float *p_in, const int iHalChunk, 
                                const int jHalChunk, const int kHalChunk) 
{
    // How do we know that we're on the side of the chunk?
    // - When a point is on halo: j = 0 || j = jp + 1(given it's a 1-point halo, 
    //  TODO: check with supervisor if the solution should be generalized to n-point halo)
    // - We'll do pointless periodic boundary calculations for bottom and top 
    //  hallo planes, so that we don't need to do additional checks.
    // - Need to get i, j, k position first!
    
    int global_id = get_global_id(0);
    int4 ijk = calc_loop_iters(global_id, iHalChunk, jHalChunk, kHalChunk, 0, 0, 0);
    unsigned int i = ijk.s0;
    unsigned int j = ijk.s1;
    unsigned int k = ijk.s2;

    unsigned int ip = iHalChunk - 2;
    unsigned int jp = jHalChunk - 2;

    if (j == 0 || j == jp + 1) {
        p_in[F4D2C(((1 - 0)+1), (((ip+2) - 0)+1), (((jp+2) - 0)+1), 0,0,0,0, 0,i,0,k)] = p_in[F4D2C(((1 - 0)+1), (((ip+2) - 0)+1), (((jp+2) - 0)+1), 0,0,0,0, 0,i,jp,k)];
        p_in[F4D2C(((1 - 0)+1), (((ip+2) - 0)+1), (((jp+2) - 0)+1), 0,0,0,0, 0,i,jp+1,k)] = p_in[F4D2C(((1 - 0)+1), (((ip+2) - 0)+1), (((jp+2) - 0)+1), 0,0,0,0, 0,i,1,k)];
    }
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