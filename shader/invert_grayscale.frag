
uniform sampler2D texture;
varying vec2 v_texCoord;

void main()
{
  /* Sample source color */
  vec4 frag = texture2D(texture, v_texCoord);
  vec3 alpha = vec3(1.0 - frag.a);
  /* Apply inverted grayscale */
  gl_FragColor = vec4(alpha, 1.0);
}
