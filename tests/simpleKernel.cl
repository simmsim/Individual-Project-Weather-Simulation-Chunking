__kernel void simple_kernel(__global float *p_in) {
    int global_id = get_global_id(0);
    p_in[global_id] = p_in[global_id] + 1;
}

