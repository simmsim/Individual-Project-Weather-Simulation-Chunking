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
p_in(0, i, 0, k) = p_in(0, i, jp, k)
p_in(0, i, jp+1, k) = p_in(0, i, 1, k)
*/
void compute_periodic_condition(__global float *p_in, int ip, int jp) 
{    
    int global_id = get_global_id(0);
    int i_range = ip + 2;
    int k = global_id/i_range;
    int i = global_id-(k*i_range);
    
    p_in[F3D2C(ip+2, jp+2, 0,0,0, i,0,k)] = p_in[F3D2C(ip+2, jp+2, 0,0,0, i,jp,k)];
    p_in[F3D2C(ip+2, jp+2, 0,0,0, i,jp+1,k)] = p_in[F3D2C(ip+2, jp+2, 0,0,0, i,1,k)];

    /*
    // Alternative
    int i = get_global_id(0);
    int k = get_global_id(2);
    */
}

/*
Compute the open condition for the out-flow plane
p_in(0, ip+1, j, k) = p_in(0, ip, j, k)
*/
void compute_outflow_condition(__global float *p_in, int ip, int jp) 
{
    int global_id = get_global_id(0);
    int j_range = jp + 2;
    int k = global_id/j_range;
    int j = global_id-(k*j_range);

   p_in[F3D2C(ip+2, jp+2, 0,0,0, ip+1,j,k)] = p_in[F3D2C(ip+2, jp+2, 0,0,0, ip,j,k)]; 

    /*
    // Alternative
    int j = get_global_id(1);
    int k = get_global_id(2);
    */
}

/*
Compute the open condition for the in-flow plane
p_in(0, 0, j, k) = p_in(0, 1, j, k)
*/
void compute_inflow_condition(__global float *p_in, int ip, int jp) 
{
    int global_id = get_global_id(0);
    int j_range = jp + 2;
    int k = global_id/j_range;
    int j = global_id-(k*j_range);

    p_in[F3D2C(ip+2, jp+2, 0,0,0, 0,j,k)] = p_in[F3D2C(ip+2, jp+2, 0,0,0, 1,j,k)]; 
   
    /*
    // Alternative
    int j = get_global_id(1);
    int k = get_global_id(2);
    */
}

/* 
Compute the open condition for top and bottom planes
p_in(0, i, j, 0) = p(0, i, j, 1)
p_in(0, i, j, kp+1) = p_in(0, i, j, kp)
*/
void compute_top_bottom_conditions(__global float *p_in, int ip, int jp, int kp)  
{
    int global_id = get_global_id(0);
    int i_range = ip + 2;
    int j = global_id/i_range;
    int i = global_id-(j*i_range);

    p_in[F3D2C(ip+2, jp+2, 0,0,0, i,j,0)] = p_in[F3D2C(ip+2, jp+2, 0,0,0, i,j,1)];
    p_in[F3D2C(ip+2, jp+2, 0,0,0, i,j,kp+1)] = p_in[F3D2C(ip+2, jp+2, 0,0,0, i,j,kp)];
    
    /*
    // Alternative
    int i = get_global_id(0);
    int j = get_global_id(1);
    */
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
    /*
    // Alternative
    int i = get_global_id(0);
    int j = get_global_id(1);
    int k = get_global_id(2);
    */
    int global_id = get_global_id(0);
    int i_range = ip;
    int j_range = jp;

    int k_rel = global_id/(j_range*i_range);
    int k = k_rel + 1;
    int j_rel = ((global_id-(k_rel*(j_range*i_range)))/i_range);
    int j = j_rel + 1;
    int i_rel = ((global_id-(k_rel*(j_range*i_range)))-(j_rel*i_range));
    int i = i_rel + 1;
    
    float relmtp = omega*(cn1*(
        cn2l*p_in[F3D2C(ip+2, jp+2, 0,0,0, i+1,j,k)] +
        cn2s*p_in[F3D2C(ip+2, jp+2, 0,0,0, i-1,j,k)] +
        cn3l*p_in[F3D2C(ip+2, jp+2, 0,0,0, i,j+1,k)] +
        cn3s*p_in[F3D2C(ip+2, jp+2, 0,0,0, i,j-1,k)] +
        cn4l*p_in[F3D2C(ip+2, jp+2, 0,0,0, i,j,k+1)] +
        cn4s*p_in[F3D2C(ip+2, jp+2, 0,0,0, i,j,k-1)] -
        rhs[F3D2C(ip+2, jp+2, 0,0,0, i,j,k)]) - 
        p_in[F3D2C(ip+2, jp+2, 0,0,0, i,j,k)]);
    
    p_out[F3D2C(ip+2, jp+2, 0,0,0, i,j,k)] = p_in[F3D2C(ip+2, jp+2, 0,0,0, i,j,k)] + relmtp;
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