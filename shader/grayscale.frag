
uniform sampler2D texture;
uniform lowp float factor;
varying vec2 v_texCoord;

void main()
{
  /* Sample source color */
  vec4 frag = texture2D(texture, v_texCoord);
  float new_alpha = abs(factor - frag.a);
  vec3 alpha = vec3(new_alpha);
  /* Apply grayscale */
  gl_FragColor = vec4(alpha, 1.0);
}
