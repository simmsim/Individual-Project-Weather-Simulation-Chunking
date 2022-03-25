/* 
An array with floats including a 1-point halo in every direction p(0:ip+1, 0:jp+1, 0:kp+1)
Where the actual core region is from 1 to ip/jp/kp.
*/

#define PERIODIC 1
#define INFLOW 2
#define OUTFLOW 3
#define TOPBOTTOM 4
#define CORE 5

inline unsigned int F3D2C(unsigned int i_rng, unsigned int j_rng, // ranges, i.e. (hb-lb)+1
        int i_lb, int j_lb, int k_lb, // lower bounds
        int ix, int jx, int kx) 
{
    return (i_rng*j_rng*(kx-k_lb)+i_rng*(jx-j_lb)+ix-i_lb);
}

/*
Compute the periodic condition, i.e. initialize side halos
*/
void compute_periodic_condition(__global float *p_in, int kp, int jp) 
{    
    int global_id = get_global_id(0);

    int k_range = kp + 2;
    int i = global_id/k_range;
    int k = global_id-(i*k_range);
    
    p_in[F3D2C(kp+2, jp+2, 0,0,0, k,0,i)] = p_in[F3D2C(kp+2, jp+2, 0,0,0, k,jp,i)];
    p_in[F3D2C(kp+2, jp+2, 0,0,0, k,jp+1,i)] = p_in[F3D2C(kp+2, jp+2, 0,0,0, k,1,i)];
}

/*
Compute the open condition for the out-flow plane
*/
void compute_outflow_condition(__global float *p_in, int ip, int jp, int kp) 
{
    int global_id = get_global_id(0);
    int j_range = jp + 2;
    int k = global_id/j_range;
    int j = global_id-(k*j_range);
    

   p_in[F3D2C(kp+2, jp+2, 0,0,0, k,j,ip+1)] = p_in[F3D2C(kp+2, jp+2, 0,0,0, k,j,ip)]; 
}

/*
Compute the open condition for the in-flow plane
*/
void compute_inflow_condition(__global float *p_in, int ip, int jp, int kp) 
{
    int global_id = get_global_id(0);
    int j_range = jp + 2;
    int k = global_id/j_range;
    int j = global_id-(k*j_range);

    p_in[F3D2C(kp+2, jp+2, 0,0,0, k,j,0)] = p_in[F3D2C(kp+2, jp+2, 0,0,0, k,j,1)]; 
}

/* 
Compute the open condition for top and bottom planes
*/
void compute_top_bottom_conditions(__global float *p_in, int ip, int jp, int kp)  
{
    int global_id = get_global_id(0);
    int j_range = jp + 2;
    int i = global_id/j_range;
    int j = global_id-(i*j_range);

    p_in[F3D2C(kp+2, jp+2, 0,0,0, 0,j,i)] = p_in[F3D2C(kp+2, jp+2, 0,0,0, 1,j,i)];
    p_in[F3D2C(kp+2, jp+2, 0,0,0, kp+1,j,i)] = p_in[F3D2C(kp+2, jp+2, 0,0,0, kp,j,i)];
}

void compute_core_region(__global float *p_in, __global float *p_out, __global float *rhs, int ip, int jp, int kp) 
{
    const float cn1 = 1.0 / 3.0;
    const float cn2l = 0.5;
    const float cn2s = 0.5;
    const float cn3l = 0.5;
    const float cn3s = 0.5;
    const float cn4l = 0.5;
    const float cn4s = 0.5;
    const float omega = 1.0;

    int global_id = get_global_id(0);
    int k_range = kp;
    int j_range = jp;

    int i_rel = global_id/(j_range*k_range);
    int i = i_rel + 1;

    int j_rel = ((global_id-(i_rel*(j_range*k_range)))/k_range);
    int j = j_rel + 1;

    int k_rel = global_id - i_rel*(j_range*k_range) - j_rel*k_range;
    int k = k_rel + 1;

    float relmtp = omega*(cn1*(
        cn2l*p_in[F3D2C(kp+2, jp+2, 0,0,0, k,j,i+1)] +
        cn2s*p_in[F3D2C(kp+2, jp+2, 0,0,0, k,j,i-1)] +
        cn3l*p_in[F3D2C(kp+2, jp+2, 0,0,0, k,j+1,i)] +
        cn3s*p_in[F3D2C(kp+2, jp+2, 0,0,0, k,j-1,i)] +
        cn4l*p_in[F3D2C(kp+2, jp+2, 0,0,0, k+1,j,i)] +
        cn4s*p_in[F3D2C(kp+2, jp+2, 0,0,0, k-1,j,i)] -
        rhs[F3D2C(kp+2, jp+2, 0,0,0, k,j,i)]) - 
        p_in[F3D2C(kp+2, jp+2, 0,0,0, k,j,i)]);
    
    p_out[F3D2C(kp+2, jp+2, 0,0,0, k,j,i)] = p_in[F3D2C(kp+2, jp+2, 0,0,0, k,j,i)] + relmtp;
}

__kernel void sor_superkernel(__global float *p_in, __global float *p_out, __global float *rhs,
                            int iChunk, int jChunk, int kChunk, __global int *state_ptr) 
{
    int state = *state_ptr;
    switch (state) {
        case CORE:
            compute_core_region(p_in, p_out, rhs, iChunk, jChunk, kChunk);
            break;
        case PERIODIC:
            compute_periodic_condition(p_in, kChunk, jChunk);
            break;
        case INFLOW:
            compute_inflow_condition(p_in, iChunk, jChunk, kChunk);
            break;
        case OUTFLOW:
            compute_outflow_condition(p_in, iChunk, jChunk, kChunk);
            break;
        case TOPBOTTOM:
            compute_top_bottom_conditions(p_in, iChunk, jChunk, kChunk);
            break;
    }
}