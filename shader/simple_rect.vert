uniform mat4 projMat;
uniform vec2 translation;
attribute vec2 position;
varying vec2 v_texCoord;

void main()
{
  vec2 p = position + translation;
  gl_Position = projMat * vec4(p, 0, 1);
  v_texCoord = p;
}