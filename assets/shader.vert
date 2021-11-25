#version 330 core

layout (location = 0) in vec3 v_pos;
layout (location = 1) in vec2 v_tex_coords;

out vec2 tex_coords;

void main()
{   
    tex_coords = v_tex_coords;
    gl_Position = vec4(v_pos.x, v_pos.y, v_pos.z, 1.0);
}