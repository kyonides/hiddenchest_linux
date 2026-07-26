
uniform vec2 center;
uniform vec2 rect_wh;
uniform vec4 color;
uniform float sides;
uniform float radius;
uniform float angle;
varying vec2 v_texCoord;
const float PI = 3.14159265;

float polygon_sdf(vec2 p, float n, float r)
{
  float rad = radians(angle);
  float c = cos(rad);
  float s = sin(rad);
  p = vec2(p.x * c - p.y * s, p.x * s + p.y * c);
  float a = atan(p.x, p.y);
  float b = PI * 2.0 / n;
  float d = cos(floor(0.5 + a / b) * b - a) * length(p);
  return d - r;
}

void main()
{ 
  vec2 p = v_texCoord - center;
  float d = polygon_sdf(p, sides, radius);
  float alpha = smoothstep(0.5, 0.0, d);
  if (alpha == 0.0) discard;
  gl_FragColor = vec4(color.rgb, color.a * alpha);
}