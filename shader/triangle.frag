
uniform vec2 center;
uniform vec2 rect_wh;
uniform vec4 color;
uniform float radius;
uniform float angle;
varying vec2 v_texCoord;
const float sqrt3 = 1.7320508075688772;

float calc_triangle(vec2 p)
{
  float rad = radians(angle) + radians(90.0);
  float c = cos(rad);
  float s = sin(rad);
  p = vec2(p.x * c - p.y * s, p.x * s + p.y * c);
  p.x = abs(p.x);
  vec2 normal = vec2(sqrt3 * 0.5, 0.5);
  p.y -= radius;
  float side_dist = dot(p, normal);
  float bottom_dist = -p.y - (1.5 * radius);
  return max(side_dist, bottom_dist);
}

void main()
{
  vec2 p = v_texCoord - center;
  float d = calc_triangle(p);
  float alpha = smoothstep(1.0, 0.0, d);
  gl_FragColor = vec4(color.rgb, color.a * alpha);
}