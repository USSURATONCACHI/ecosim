#version 330 core
out vec4 color;
in  vec2 rel_coords;

uniform vec3 background_color;
uniform vec3 border_color;
uniform vec2 rectangle_size;
uniform vec4 max_rectangle;
uniform float border_size;

//is slider, cursor_pos, marks_count;
uniform vec3 slider_params; 

vec4 render(vec2 offset) {
    vec2 relative_coords = rel_coords * rectangle_size + offset;
    vec2 abs_coords      = gl_FragCoord.xy + offset;
    
    float half_height = rectangle_Size.y / 2.0;
    vec2 sym_coords = vec2(relative_coords.x, abs(relative_coords.y - half_height));

    vec4 no_color = vec4(0.0);

    return (abs_coords.x < max_rectangle.x || 
            abs_coords.y > max_rectangle.y || 
            abs_coords.x > (max_rectangle.x + max_rectangle.z) || 
            abs_coords.y < (max_rectangle.y - max_rectangle.w) ||
            sym_coords.y > (border_size / 2.0)) ? no_color : background_color;
}

void main()
{    

    color = (
	render(vec2(-0.1, -0.1)) + render(vec2(0.0, -0.1)) + render(vec2(0.1, -0.1)) +
	render(vec2(-0.1, 0.0)) + render(vec2(0.0, 0.0)) + render(vec2(0.1, 0.0)) +
	render(vec2(-0.1, 0.01)) + render(vec2(0.0, 0.1)) + render(vec2(0.1, 0.1)) ) / 9.;
}