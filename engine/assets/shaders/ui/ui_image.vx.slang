import view;
import ui;

ConstantBuffer<RenderView> gpu_view;

StructuredBuffer<ImageInstance> images;

/* Rotate the point 'p' on the Z axis. */
inline float2 rotate_vertex(const float2 p, const ImageInstance instance) {
    const float2 o = p - instance.extent() * instance.pivot();

    /* <https://stackoverflow.com/questions/34050929/3d-point-rotation-algorithm> */
    const float cosa = cos(instance.angles().z);
    const float sina = sin(instance.angles().z);
    const float cosb = cos(instance.angles().y);
    const float sinb = sin(instance.angles().y);
    const float cosc = cos(instance.angles().x);
    const float sinc = sin(instance.angles().x);
    const float xx = cosa * cosb;
    const float xy = cosa * sinb * sinc - sina * cosc;
    const float yx = sina * cosb;
    const float yy = sina * sinb * sinc + cosa * cosc;
    return float2(xx * o.x + xy * o.y, yx * o.x + yy * o.y);
}

[shader("vertex")]
VertexOutput main(VertexInput input, int instance_id: SV_InstanceID) {
    /* Calculate the position of the vertex in normalized device coordinates */
    const ImageInstance instance = images[instance_id];
    const float2 ndc = (instance.pos() + rotate_vertex(input.pos.xy * instance.extent(), instance)) / gpu_view.resolution;

    /* Flip the UVs if the extent is negative on the X axis */
    // if (instance.extent().x < 0.0) input.uv = 1.0 - input.uv;
    /* Flip the UVs if the angle is negative on the Z axis */
    // if (instance.angles().z > 0.0) input.uv.y = 1.0 - input.uv.y;

    VertexOutput output;
    output.pos = float4(ndc * 2.0f - 1.0f, 0.0f, 1.0f);
    output.uvs = input.uvs;
    output.instance_id = instance_id;

    return output;
}