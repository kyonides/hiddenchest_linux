
uniform sampler2D texture;
varying vec2 v_texCoord;
const vec3 GRAY = vec3(0.299, 0.587, 0.114);
uniform lowp vec4 c1;
uniform lowp vec4 c2;
uniform vec2 origin;
uniform vec2 resolution;

void main()
{
  vec4 texColor = texture2D(texture, v_texCoord);
  if (texColor.a == 0.0) {
    gl_FragColor = mix(texColor, vec4(1.0, 1.0, 1.0, 1.0), 0.0);
    return;
  }
  float gray = dot(texColor.rgb, GRAY);
  texColor.rgb = mix(texColor.rgb, vec3(gray), 0.9);
  vec2 st = gl_FragCoord.xy / resolution.xy;
  float mix_value = distance(st, origin);
  vec4 mixed = mix(c1, c2, mix_value);
  gl_FragColor = mix(texColor, mixed, 0.4);
}
