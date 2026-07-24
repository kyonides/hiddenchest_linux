
uniform vec2 pos;
uniform vec2 rect_wh;
uniform vec4 color;
uniform float radius;
varying vec2 v_texCoord;

// Calculate distance to a rounded box
float rounded_rect_sdf(vec2 p, vec2 b, float r) {
  vec2 d = abs(p) - b + vec2(r);
  return min(max(d.x, d.y), 0.0) + length(max(d, 0.0)) - r;
}

void main() {
  // Center of the rectangle
  vec2 rect_center = pos + (rect_wh * 0.5);
  vec2 center_space = v_texCoord - rect_center;
  float distance = rounded_rect_sdf(center_space, rect_wh * 0.5, radius * 100.0);
  float delta = fwidth(distance);
  float alpha = smoothstep(delta, -delta, distance);
  if (alpha == 0.0) discard;
  gl_FragColor = vec4(color.rgb, color.a * alpha);
}
