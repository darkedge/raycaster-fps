$output v_texcoord0
// Valid input attributes:
// a_position, a_normal, a_tangent, a_bitangent, a_color0, a_color1, a_color2, a_color3, a_indices, a_weight,
// a_texcoord0, a_texcoord1, a_texcoord2, a_texcoord3, a_texcoord4, a_texcoord5, a_texcoord6, a_texcoord7,
// i_data0, i_data1, i_data2, i_data3, i_data4.

void main()
{
    v_texcoord0 = vec2((gl_VertexID << 1) & 2, gl_VertexID & 2);
    gl_Position = vec4(v_texcoord0 * vec2(2.0, -2.0) + vec2(-1.0, 1.0), 0.0, 1.0);
}
