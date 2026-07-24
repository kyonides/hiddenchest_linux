
uniform vec2 pos;
uniform float radius;
uniform vec4 color;
varying vec2 v_texCoord;

void main() {
  // Center of the circle
  vec2 center = pos + vec2(radius);
  // Signed distance from the circle edge
  float d = length(v_texCoord - center) - radius;
  float delta = fwidth(d);
  float alpha = smoothstep(delta, -delta, d);
  if (alpha <= 0.0) discard;
  gl_FragColor = vec4(color.rgb, color.a * alpha);
}
