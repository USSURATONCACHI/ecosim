#version 330 core
out vec4 color;
in  vec2 rel_coords;

uniform vec3 background_color;
uniform vec3 border_color;
uniform vec2 rectangle_size;
uniform vec4 max_rectangle;
uniform float border_size;

uniform vec3 click_zone_color;
uniform vec3 click_zone_point;
uniform float now;

vec4 render(vec2 offset) {
    vec2 relative_coords = rel_coords * rectangle_size + offset;
    vec2 abs_coords      = gl_FragCoord.xy + offset;
    vec2 bd_dist = abs(relative_coords - rectangle_size * 0.5)- rectangle_size*0.5;
    bool is_border = (bd_dist.x > -border_size) || (bd_dist.y > (-border_size));

    float click_time_elapsed = max(min((now - click_zone_point.z), 1.0f), 0.0f);
    vec3 final_color = click_time_elapsed * background_color + (1.0f - click_time_elapsed) * click_zone_color;

    float inside_zone = float(  length(abs_coords.xy - click_zone_point.xy) < click_time_elapsed * 1500.0f  );
    vec4 out_color = vec4(is_border ? border_color : (final_color * inside_zone + (1.0f - inside_zone) * background_color), 1.0);
    vec4 no_color = vec4(0.0, 0.0, 0.0, 0.0);

    return (abs_coords.x < max_rectangle.x || 
            abs_coords.y > max_rectangle.y || 
            abs_coords.x > (max_rectangle.x + max_rectangle.z) || 
            abs_coords.y < (max_rectangle.y - max_rectangle.w) ) ? no_color : out_color;
}

void main()
{    

    color = (
	render(vec2(-0.1, -0.1)) + render(vec2(0.0, -0.1)) + render(vec2(0.1, -0.1)) +
	render(vec2(-0.1, 0.0)) + render(vec2(0.0, 0.0)) + render(vec2(0.1, 0.0)) +
	render(vec2(-0.1, 0.01)) + render(vec2(0.0, 0.1)) + render(vec2(0.1, 0.1)) ) / 9.;
}