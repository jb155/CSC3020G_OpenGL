#version 330 core

uniform mat4 mat4Loc;

in vec3 position;

void main()
{
    gl_Position = mat4Loc * vec4(position,1.0f);
}
