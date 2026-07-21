
uniform sampler2D texture;
varying vec2 v_texCoord;

void main()
{
  vec4 color = texture2D(texture, v_texCoord);
  float heat = dot(color.rgb, vec3(0.299, 0.587, 0.114));
  vec3 thermal;
  // Cold: Blue
  thermal.b = smoothstep(0.5, 0.0, heat);
  // Warm: Purple / Red
  thermal.r = smoothstep(0.3, 0.7, heat);
  // Hot: Yellow / White
  thermal.g = smoothstep(0.5, 0.9, heat) * smoothstep(1.0, 0.7, heat);
  if (heat > 0.9) {
    thermal.g = heat;
  }
  gl_FragColor = vec4(thermal, color.a);
}
