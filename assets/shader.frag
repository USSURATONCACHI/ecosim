#version 330 core

out vec3 FragColor;
in vec2 tex_coords;

uniform vec2 cam_pos;
uniform float cam_scale;
uniform vec3 cursor;

uniform vec2 world_size;
uniform vec2 poly_size;
uniform int msaa;

uniform sampler2D texture;

vec2 fXY;

float cube(float x) {
	return x * x * x;
}
float interp1(float x) {
	return 1. - abs(cube(x * 2. - 1.));
}

vec3 render(vec2 localPos) {
	//Позиция пикселя относительно камеры
	vec2 pos = gl_FragCoord.xy - poly_size.xy/2 + localPos;
	//Оно же в клетках мира
    vec2 world_pos = pos/cam_scale + cam_pos;
    
	//Находится ли фрагмент внутри мира
    bool inside_world = world_pos.y > 0 && world_pos.y < world_size.y;

	//Получение исходного цвета
    ivec2 world_pos_ranged = ivec2( floor(fract(world_pos.x / world_size.x)*world_size.x), floor(world_pos.y));
	vec3 color = inside_world ? texelFetch(texture, world_pos_ranged, 0).rgb : vec3(0.);
	
	//Затенение краёв клеточек
	float klc = pow(interp1(fract(world_pos.x))*interp1(fract(world_pos.y)), 0.2);
	//Нужно ли рисовать сетку
	float is_big_enough = min(max(cam_scale - 3.0, 0.0), 5.0)/5.0;
	//Рассчет и применение затемнения
	klc = 1 - (1-klc)*is_big_enough;
	color.rgb *= klc;

	//Столбы света по краям мира
	float lc_brightness = 0.333333 / cam_scale;
	//Координата пикселя в клетках в нужном диапазоне (от центра отсчет)
	float world_xr = mod(mod(world_pos.x + world_size.x/2.0, world_size.x) + world_size.x, world_size.x);
	//Дистанция до столба света
	float lc_distance = abs( world_xr - world_size.x/2.0 );
	//Дистанция фрагмента по вертикали от центра мира
	float lc_vertical_distance = abs( world_pos.y - world_size.y/2.0 )/world_size.y;

	//Яркость столба в этом фрагменте
	float lc_active = (1.0 - lc_distance/lc_brightness) * float(lc_distance <= lc_brightness) *
					  float(lc_vertical_distance <= 1.0) * (1.0 - pow(lc_vertical_distance, 4.0));

	//Нужно ли подсвечивать курсор
	ivec2 delta = world_pos_ranged - ivec2(cursor.xy);
	ivec2 world_wi = ivec2(world_size.x, 0.0);
	float is_cursor = float(length(delta) < cursor.z || 
						    length(delta + world_wi) < cursor.z || 
							length(delta - world_wi) < cursor.z) * 0.2 * float(inside_world);

    return color + lc_active + is_cursor;
}


// MAIN
void main() {
fXY = floor(gl_FragCoord.xy);

float bxy = int(gl_FragCoord.x + gl_FragCoord.y) & 1;
float nbxy = 1. - bxy;

int MSAA = msaa;
// NAA x1
///=========
if (MSAA == 1) {
	FragColor = render(vec2(0.0));
} else
// MSAA x2
///=========
if (MSAA == 2) {
	FragColor = (render(vec2(0.33 * nbxy, 0.)) + render(vec2(0.33 * bxy, 0.33))) / 2.;
} else
// MSAA x3
///=========
if (MSAA == 3) {
	FragColor = (render(vec2(0.66 * nbxy, 0.)) + render(vec2(0.66 * bxy, 0.66)) + render(vec2(0.33, 0.33))) / 3.;
} else
// MSAA x4
///=========
if (MSAA == 4) { // rotate grid
	FragColor = (render(vec2(0.33, 0.1)) + render(vec2(0.9, 0.33)) + render(vec2(0.66, 0.9)) + render(vec2(0.1, 0.66))) / 4.;
} else
// MSAA x5
///=========
if (MSAA == 5) { // centre rotate grid
	FragColor = (render(vec2(0.33, 0.2)) + render(vec2(0.8, 0.33)) + render(vec2(0.66, 0.8)) + render(vec2(0.2, 0.66)) + render(vec2(0.5, 0.5))) / 5.;
} else
// SSAA x9
///=========
if (MSAA == 9) {  // centre grid 3x3
	FragColor = (
	render(vec2(0.17, 0.2)) + render(vec2(0.17, 0.83)) + render(vec2(0.83, 0.17)) + render(vec2(0.83, 0.83)) + 
	render(vec2(0.5, 0.17)) + render(vec2(0.17, 0.5)) + render(vec2(0.5, 0.83)) + render(vec2(0.83, 0.5)) + 
	render(vec2(0.5, 0.5)) * 2.) / 10.;
} else
// SSAA x16
///=========
if (MSAA == 16) { // classic grid 4x4
	FragColor = (
	render(vec2(0.00, 0.00)) + render(vec2(0.25, 0.00)) + render(vec2(0.50, 0.00)) + render(vec2(0.75, 0.00)) + 
	render(vec2(0.00, 0.25)) + render(vec2(0.25, 0.25)) + render(vec2(0.50, 0.25)) + render(vec2(0.75, 0.25)) + 
	render(vec2(0.00, 0.50)) + render(vec2(0.25, 0.50)) + render(vec2(0.50, 0.50)) + render(vec2(0.75, 0.50)) + 
	render(vec2(0.00, 0.75)) + render(vec2(0.25, 0.75)) + render(vec2(0.50, 0.75)) + render(vec2(0.75, 0.75))) / 16.;
}
}
///============================================================///
// 256 65536 4294967296