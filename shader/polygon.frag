
uniform int sides;
uniform vec2 rect_wh;
uniform vec4 color;
uniform float radius;
varying vec2 v_texCoord;

float polygon_sdf(vec2 p, int n, float r)
{
  float a = atan(p.x, p.y);
  float b = 6.2831853 / float(n);
  float d = cos(floor(0.5 + a / b) * b - a) * length(p);
  return d - r;
}

void main()
{ 
  vec2 p = v_texCoord * 2.0 - 1.0;//- vec2(float(sides) * 0.5);
  p.x *= rect_wh.x / rect_wh.y;
  float d = polygon_sdf(p, sides, radius);
  float alpha = smoothstep(1.5 / rect_wh.y, 0.0, d);//1.0 - smoothstep(0.0, fwidth(d), d);
  if (alpha == 0.0) discard;
  gl_FragColor = vec4(color.rgb, color.a * alpha);
}