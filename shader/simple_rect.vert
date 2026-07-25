
uniform mat4 projMat;
attribute vec2 position;
varying vec2 v_texCoord;

void main()
{
  gl_Position = projMat * vec4(position, 0, 1);
  v_texCoord = position;
}
