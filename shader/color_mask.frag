uniform sampler2D texture;
varying vec2 v_texCoord;
uniform vec4 color;
uniform float range; // Range 0.05 to 0.2
const float edge_width = 0.05;

void main()
{
  vec4 frag = texture2D(texture, v_texCoord);
  // Calculate the absolute distance between the pixel and the mask color
  float dist = distance(frag.rgb, color.rgb);
  // Smoothly transition alpha based on the tolerance threshold
  frag.a *= smoothstep(range, range + edge_width, dist);
  gl_FragColor = frag;
}