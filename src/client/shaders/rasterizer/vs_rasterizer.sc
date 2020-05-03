$input a_position, a_texcoord0
$output v_texcoord0
// Valid input attributes:
// a_position, a_normal, a_tangent, a_bitangent, a_color0, a_color1, a_color2, a_color3, a_indices, a_weight,
// a_texcoord0, a_texcoord1, a_texcoord2, a_texcoord3, a_texcoord4, a_texcoord5, a_texcoord6, a_texcoord7,
// i_data0, i_data1, i_data2, i_data3, i_data4.

#include "../common.sh"

// uniform vec4 u_viewRect;
// uniform vec4 u_viewTexel;
// uniform mat4 u_view;
// uniform mat4 u_invView;
// uniform mat4 u_proj;
// uniform mat4 u_invProj;
// uniform mat4 u_viewProj;
// uniform mat4 u_invViewProj;
// uniform mat4 u_model[BGFX_CONFIG_MAX_BONES];
// uniform mat4 u_modelView;
// uniform mat4 u_modelViewProj;
// uniform vec4 u_alphaRef4;

void main()
{
    gl_Position = mul(u_modelViewProj, vec4(a_position, 1.0));
    v_texcoord0 = a_texcoord0;
}
