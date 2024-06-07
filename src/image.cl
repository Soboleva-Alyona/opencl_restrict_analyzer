const sampler_t sampler = CLK_NORMALIZED_COORDS_FALSE |
CLK_ADDRESS_CLAMP_TO_EDGE |
CLK_FILTER_NEAREST;

__kernel void imgCopy(__read_only image2d_t input)
{
    int2 coords;
    coords.x = get_global_id(0);
    coords.y = get_global_id(1);
    float4 data = read_imagef(input, sampler, coords);
}
