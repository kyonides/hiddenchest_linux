
uniform sampler2D texture;
uniform vec2 resolution;
varying vec2 v_texCoord;
const vec3 GRAY = vec3(0.299, 0.587, 0.114);

void main()
{
  vec4 origin_color = texture2D(texture, v_texCoord);
  float gray = dot(origin_color.rgb, GRAY);
  origin_color.rgb = mix(origin_color.rgb, vec3(gray), 1.0);
  vec2 position = (gl_FragCoord.xy / resolution.xy) - vec2(0.5);
  float dist = length(position);
  float radius = 0.5;
  float softness = 0.02;
  float vignette = smoothstep(radius, radius - softness, dist);
  origin_color.rgb = origin_color.rgb - (1.0 - vignette);
  gl_FragColor = origin_color;
}