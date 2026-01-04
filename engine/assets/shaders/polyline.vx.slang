import view;

ConstantBuffer<RenderView> render_view;

/* Vertex shader input. */
struct VertexIn {
    float3 point_a : POSITION0;
    float3 point_b : POSITION1;
    float4 color : COLOR;
    float width : WIDTH;
};

/* Vertex shader output. */
struct VertexOut {
    float4 clip_pos : SV_POSITION;
    float4 color : COLOR;
};

/* Instanced vertices. */
static const float3 vertices[6] = {
    float3(0.0f, -0.5f, 0.0f),
    float3(0.0f, -0.5f, 1.0f),
    float3(0.0f,  0.5f, 1.0f),
    float3(0.0f, -0.5f, 0.0f),
    float3(0.0f,  0.5f, 1.0f),
    float3(0.0f,  0.5f, 0.0f)
};

/* Credit: <https://github.com/ForesightMiningSoftwareCorporation/bevy_polyline> */
float4 clip_near_plane(const float4 a, const float4 b) {
    /* Move a if a is behind the near plane and b is in front. */
    if (a.z > a.w && b.z <= b.w) {
        /* Interpolate a towards b until it's at the near plane. */
        const float distance_a = a.z - a.w;
        const float distance_b = b.z - b.w;
        const float t = distance_a / (distance_a - distance_b);
        return a + (b - a) * t;
    }
    return a;
}

[shader("vertex")]
VertexOut main(VertexIn input, uint vertex_id: SV_VertexID) {
    /* Get the vertex for this thread */
    const float3 vertex = vertices[vertex_id];

    /* Convert line points into clip-space and perform manual near plane clipping */
    float4 clip0 = mul(render_view.world_to_clip, float4(input.point_a, 1.0f));
    float4 clip1 = mul(render_view.world_to_clip, float4(input.point_b, 1.0f));
    clip0 = clip_near_plane(clip0, clip1);
    clip1 = clip_near_plane(clip1, clip0);

    /* Pick either point a or b based on the Z coordinate of the vertex */
    const float4 clip = lerp(clip0, clip1, vertex.z);

    /* Calculate the screen-space location of point a and b */
    const float2 screen0 = float2(render_view.resolution) * (0.5f * clip0.xy / clip0.w + 0.5f);
    const float2 screen1 = float2(render_view.resolution) * (0.5f * clip1.xy / clip1.w + 0.5f);

    /* Find our two screen-space basis vectors */
    const float2 x_basis = normalize(screen1 - screen0);
    const float2 y_basis = float2(-x_basis.y, x_basis.x);

    /* A negative line width indicates it should be in screen-space */
    float line_width = abs(input.width);
    float4 line_color = input.color;

    /* For positive line widths we use world-space units */
    if (input.width > 0.0f) line_width /= clip.w * 0.01f;

    /* Fade out thin lines, based on <https://acegikmo.com/shapes/docs/#anti-aliasing> */
    if (line_width < 1.0f) {
        line_color.a *= max(0.0f, line_width);
        line_width = 1.0f;
    }

    /* Calculate the final line point for this thread in clip-space */
    const float2 pt_offset = line_width * (vertex.x * x_basis + vertex.y * y_basis);
    const float2 pt = lerp(screen0 + pt_offset, screen1 + pt_offset, vertex.z);

    VertexOut output;
    output.clip_pos = float4(clip.w * ((2.0f * pt) / float2(render_view.resolution) - 1.0f), clip.z, clip.w);
    output.color = line_color;
    return output;
}
