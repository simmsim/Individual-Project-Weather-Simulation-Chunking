#define PERIODIC 1
#define INFLOW 2
#define OUTFLOW 3
#define TOPBOTTOM 4
#define CORE 5

/* 
An array with floats including a 1-point halo in every direction p(0:ip+1, 0:jp+1, 0:kp+1)
Where the actual core region is from 1 to ip/jp/kp.
*/

/*
Compute the periodic condition, i.e. initialize side halos
p_in(0, i, 0, k) = p_in(0, i, jp, k)
p_in(0, i, jp+1, k) = p_in(0, i, 1, k)
*/
void compute_periodic_condition(__global float *p_in) {}

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
        case PERIODIC:
            compute_periodic_condition(p_in);
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