
uniform sampler2D source;
uniform sampler2D mask_tex;

varying vec2 v_texCoord;

void main()
{
  vec4 tex_color = texture2D(source, v_texCoord);
  vec4 mask_color = texture2D(mask_tex, v_texCoord);
  tex_color.a *= mask_color.r;
  gl_FragColor = tex_color;
}