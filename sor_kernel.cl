#define PERIODIC 1
#define INFLOW 2
#define OUTFLOW 3
#define TOPBOTTOM 4
#define CORE 5

unsigned int F3D2C(unsigned int i_rng,unsigned int j_rng, // ranges, i.e. (hb-lb)+1
        int i_lb, int j_lb, int k_lb, // lower bounds
        int ix, int jx, int kx) 
{
    return (i_rng*j_rng*(kx-k_lb)+i_rng*(jx-j_lb)+ix-i_lb);
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
void compute_periodic_condition(__global float *p, const int ip, 
                                const int jp) 
{
    int i = get_global_id(0);
    int k = get_global_id(2);
    /*
    int i_range = iChunk + 2;
    int j_range = jChunk + 2;
    int k_range = kChunk + 2;
    
    int k = global_id/(i_range*j_range);
    int i = global_id-(k*i_range);

    p[FTNREF3D0(i, 0, k, ip+2, jp+2)] = p[FTNREF3D0(i, jp, k, ip+2, jp+2)];
    p[FTNREF3D0(i, jp+1, k, ip+2, jp+2)] = p[FTNREF3D0(i, 1, k, ip+2, jp+2)];
    */
    p[F3D2C(ip+2, jp+2, 0,0,0, i,0,k)] = p[F3D2C(ip+2, jp+2, 0,0,0, i,jp,k)];
    p[F3D2C(ip+2, jp+2, 0,0,0, i,jp+1,k)] = p[F3D2C(ip+2, jp+2, 0,0,0, i,1,k)];

    //printf("p left: %d, ", p[F3D2C(ip+2, jp+2, 0,0,0, i,0,k)]);
}

/*
Compute the open condition for the out-flow plane
p_in(0, ip+1, j, k) = p_in(0, ip, j, k)
*/
void compute_outflow_condition(__global float *p_in, const int ip, 
                              const int jp) 
{
    int j = get_global_id(1);
    int k = get_global_id(2);

    p_in[F3D2C(ip+2, jp+2, 0,0,0, ip+1,j,k)] = p_in[F3D2C(ip+2, jp+2, 0,0,0, ip,j,k)];
}

/*
Compute the open condition for the in-flow plane
p_in(0, 0, j, k) = p_in(0, 1, j, k)
*/
void compute_inflow_condition(__global float *p_in, const int ip, 
                               const int jp) 
{
    int j = get_global_id(1);
    int k = get_global_id(2);

    p_in[F3D2C(ip+2, jp+2, 0,0,0, 0,j,k)] = p_in[F3D2C(ip+2, jp+2, 0,0,0, 1,j,k)];
}

/* 
Compute the open condition for top and bottom planes
p_in(0, i, j, 0) = p(0, i, j, 1)
p_in(0, i, j, kp+1) = p_in(0, i, j, kp)
*/
void compute_top_bottom_conditions(__global float *p_in, const int ip, 
                               const int jp, const int kp)  
{
    int i = get_global_id(0);
    int j = get_global_id(1);

    p_in[F3D2C(ip+2, jp+2, 0,0,0, i,j,0)] = p_in[F3D2C(ip+2, jp+2, 0,0,0, i,j,1)];
    p_in[F3D2C(ip+2, jp+2, 0,0,0, 0,j,kp+1)] = p_in[F3D2C(ip+2, jp+2, 0,0,0, 1,j,kp)];
}

/*
Compute new values in the core region

p(i,j,k) = p(i,j,k) + omega*( &
p(i+1,j,k) + &
p(i-1,j,k) + &
p(i,j+1,k) + &
p(i,j-1,k) + &
p(i,j,k+1) + &
p(i,j,k-1))/6 &
- p(i,j,k))
*/
void compute_core_region(__global float *p_in, __global float *p_out,
                         const int ip, const int jp) 
{
    // TODO find out if offset can be specified in EnqueueNDRange
    int i = get_global_id(0);
    int j = get_global_id(1);
    int k = get_global_id(2);

    p_out[F3D2C(ip+2, jp+2, 0,0,0, i,j,0)] = (p_in[F3D2C(ip+2, jp+2, 0,0,0, i+1,j,k)] 
                                            + p_in[F3D2C(ip+2, jp+2, 0,0,0, i-1,j,k)]
                                            + p_in[F3D2C(ip+2, jp+2, 0,0,0, i,j+1,k)]
                                            + p_in[F3D2C(ip+2, jp+2, 0,0,0, i,j-1,k)]
                                            + p_in[F3D2C(ip+2, jp+2, 0,0,0, i,j,k-1)]
                                            + p_in[F3D2C(ip+2, jp+2, 0,0,0, i,j,k+1)])/6;
}

__kernel void sor_superkernel(__global float *p_in, __global float *p_out, 
                            const int iChunk, const int jChunk, const int kChunk,
                            int state) 
{
    switch (state) {
        case CORE:
            compute_core_region(p_in, p_out, iChunk, jChunk);
            break;
        case PERIODIC:
            compute_periodic_condition(p_in, iChunk, jChunk);
            break;
        case INFLOW:
            compute_inflow_condition(p_in, iChunk, jChunk);
            break;
        case OUTFLOW:
            compute_outflow_condition(p_in, iChunk, jChunk);
            break;
        case TOPBOTTOM:
            compute_top_bottom_conditions(p_in, iChunk, jChunk, kChunk);
            break;
    }
}