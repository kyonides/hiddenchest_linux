
uniform sampler2D texture; // 0
//uniform vec2 resolution;
uniform float vx_offset;
varying vec2 v_texCoord;

void main() 
{ //vec2 uv = resolution.xy;//gl_TexCoord[0].xy;
  vec3 tc = vec3(1.0, 0.0, 0.0);
  if (uv.x < (vx_offset-0.005))
  {
    vec3 pixcol = texture2D(texture, v_texCoord).rgb;//uv).rgb;
    vec3 colors[3];
    colors[0] = vec3(0.,0.,1.);
    colors[1] = vec3(1.,1.,0.);
    colors[2] = vec3(1.,0.,0.);
    float lum = (pixcol.r+pixcol.g+pixcol.b)/3.;
    int ix = (lum < 0.5)? 0:1;
    tc = mix(colors[ix],colors[ix+1],(lum-float(ix)*0.5)/0.5);
  }
  else if (uv.x>=(vx_offset+0.005))
  {
    tc = texture2D(texture, v_texCoord).rgb;//uv).rgb;
  }
  gl_FragColor = vec4(tc, 1.0);
}