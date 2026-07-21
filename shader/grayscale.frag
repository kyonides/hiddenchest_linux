
uniform sampler2D texture;
varying vec2 v_texCoord;

void main()
{
  /* Sample source color */
  vec4 frag = texture2D(texture, v_texCoord);
  /* Apply grayscale */
  gl_FragColor = vec4(frag.aaa, 1.0);
}
