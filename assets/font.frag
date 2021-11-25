#version 330 core
in vec2 TexCoords;
out vec4 color;

uniform sampler2D text;
uniform vec3 textColor;
uniform vec4 max_rectangle;

void main()
{    
    vec4 sampled = vec4(1.0, 1.0, 1.0, texture2D(text, TexCoords).r);
    color = vec4(textColor, 1.0) * sampled * float( !(
             gl_FragCoord.x < max_rectangle.x || 
             gl_FragCoord.y > max_rectangle.y || 
             gl_FragCoord.x > (max_rectangle.x + max_rectangle.z) || 
             gl_FragCoord.y < (max_rectangle.y - max_rectangle.w) ) );
}