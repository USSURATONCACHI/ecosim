#include<iostream>
#include<cstdint>
#include<chrono>
#include<random>
#include "world.h"
#include "utils.h"

World::World(int32_t w, int32_t h): width(w), height(h), size(w*h), last_bot(0), current_tick(0), ups(30) {
	uint64_t size = static_cast<uint64_t>(w) * h;
	texture = new unsigned char[size * 4];

	for (int y = 0; y < h; y++) {
		for (int x = 0; x < w; x++) {
			int32_t id = (y * w + x)*4;
			texture[id    ] = 0;
			texture[id + 1] = y*100/h + 64;
			texture[id + 2] = 0;
			texture[id + 3] = 255;
		}
	}

	bots = new Bot[size];
	for (int i = 0; i < size; i++) {
		bots[i] = Bot();
	}

	bots_grid = new long[size];
	for (int i = 0; i < size; i++) {
		bots_grid[i] = size;
	}
}

void World::update() {
	//Обновляем
	for (int i = 0; i < last_bot; i++) {
		Bot* bot = &bots[i];
		if (bot->exists) bot->update();
	}
	//Применяем действия
	long old_id, new_id;
	for (int i = 0; i < last_bot; i++) {
		Bot* bot = &bots[i];
		if (bot->exists) {
			switch (bot->current_action.type) {
			case BotActionType::NOTHING:break;
			case BotActionType::MOVE_BOT:
				new_id = bot->current_action.new_pos;
				old_id = bot->id;

				bots_grid[old_id] = size;
				bots_grid[new_id] = i;

				bot->id = new_id;

				//Айди пикселя на текстуре
				old_id *= 4;
				new_id *= 4;

				texture[old_id] = 0;
				texture[old_id + 1] = bot->y * 100 / height + 64;
				texture[old_id + 2] = 0;

				texture[new_id] = 255;
				texture[new_id + 1] = 64;
				texture[new_id + 2] = 255;
				break;

			default: break;
			}
		}
	}

	current_tick++;
	ups.tick();
}

void World::spawnRandom(int count) {
	std::random_device rd;
	std::mt19937 mersenne(static_cast<int>(get_current_time()*100000.0));

	int bot_x, bot_y, bot_id, bots_count = 0;

	bot_x = mersenne() % width;
	bot_y = mersenne() % height;
	bot_id = bot_y * width + bot_x;

	bool gen = false;

	do {
		if (gen) {
			bot_x = (bot_x + (mersenne() % 5) - 2) % width;
			bot_y += -2 + mersenne() % 5;
			bot_id = bot_y * width + bot_x;
			gen = bot_y < 0 || bot_y >= height || bots_grid[bot_id] < size;
			continue;
		}
		long check = bots_grid[bot_id];
		if (check == size || !((&bots[check])->exists)) {
			bots[last_bot] = Bot(bot_id, bot_x, bot_y, this);
			bots_grid[bot_id] = last_bot;
			
			long tex_id = bot_id * 4;
			texture[tex_id    ] = 255;
			texture[tex_id + 1] = 64;
			texture[tex_id + 2] = 255;
			bots_count++;
			last_bot++;

			gen = true;
			continue;
		} else {
			gen = true;
			continue;
		}
	} while (bots_count < count);
}

World::~World() {
	delete[] texture;
	delete[] bots;
	delete[] bots_grid;
} 

//Bot
Bot::Bot() : exists(false), id(0), x(0), y(0), world(nullptr), world_width(0),
		current_action({BotActionType::NOTHING, 0}) {
}
Bot::Bot(uint32_t id, int x, int y, World* world) : exists(true), id(id), x(x), y(y), world(world),
		current_action({ BotActionType::NOTHING, world->size}) {
	world_width = world->getWidth();
}
void Bot::update() {
	int new_x = x + 1;
	if (new_x >= world_width) new_x -= world_width;
	int new_id = y * world_width + new_x;
	if(world->bots_grid[new_id] < world->size)
		current_action.type = BotActionType::NOTHING;
	else {
		x = new_x;
		current_action = { BotActionType::MOVE_BOT, new_id };
	}
}

//Camera
Camera::Camera(int32_t world_w, int32_t world_h, double scale = 1.0): 
		scale(scale), vel_x(0.0), vel_y(0.0), prev_mouse_x(0.0), prev_mouse_y(0.0), lmb(false),
		world_width(world_w), world_height(world_h), last_drag(get_current_time())
{
	cam_x = static_cast<double>(world_w) / 2.0;
	cam_y = static_cast<double>(world_h) / 2.0;
}

void   Camera::cursor_callback(GLFWwindow* window, double xpos, double ypos) {
	double now = get_current_time();
	double dx = (xpos - prev_mouse_x) / scale,
		   dy = (ypos - prev_mouse_y) / scale,
		   dt = now - last_drag;

	if (lmb) {
		cam_x -= dx;
		cam_y += dy;

		vel_x = -dx / dt;
		vel_y = dy / dt;

		last_drag = now;
	}

	prev_mouse_x = xpos;
	prev_mouse_y = ypos;
};

void   Camera::mouse_buttons_callback(GLFWwindow* window, int button, int action, int mods) {
	if (button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_PRESS) {
		cam_x = (*this).getCamX();
		cam_y = (*this).getCamY();
		lmb = true;
		vel_x = 0;
		vel_y = 0;
	}
	else if (button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_RELEASE) {
		lmb = false;
		double now = get_current_time(),
			   dt = now - last_drag;
		last_drag = now;
		if (dt >= 0.03) {
			vel_x = 0.0;
			vel_y = 0.0;
		}
	}

};

void   Camera::scroll_callback(GLFWwindow* window, double xoffset, double yoffset) {
	int window_width, window_height;
	glfwGetWindowSize(window, &window_width, &window_height);

	double cur_wx = (*this).getWorldX(prev_mouse_x, window_width),
		   cur_wy = (*this).getWorldY(prev_mouse_y, window_height);

	scale *= pow(1.1, yoffset);

	double delta_x = cur_wx - (*this).getWorldX(prev_mouse_x, window_width),
		   delta_y = (*this).getWorldY(prev_mouse_y, window_height) - cur_wy;

	cam_x += delta_x;
	cam_y -= delta_y;
}

double Camera::getWorldX(double xpix, double poly_width) {
	double wx = (xpix - poly_width / 2.0) / scale + (*this).getCamX();
	return fmod(fmod(wx, world_width) + world_width, world_width);
}
double Camera::getWorldY(double ypix, double poly_height) {
	return -(ypix - poly_height / 2.0) / scale + (*this).getCamY();
}

double Camera::getCamX() {
	return fmod(fmod(cam_x + getVelOffsetX(), world_width) + world_width, world_width);
}
double Camera::getCamY() {
	return cam_y + getVelOffsetY();
}

double Camera::getVelOffsetX() {
	double dt = get_current_time() - last_drag;
	double offset = (1.0 - 1.0/pow(250, dt)) * vel_x / log(8);
	return lmb ? 0.0 : offset;
}
double Camera::getVelOffsetY() {
	double dt = get_current_time() - last_drag;
	double offset = (1.0 - 1.0 / pow(250, dt)) * vel_y / log(8);
	return lmb ? 0.0 : offset;
}

void   Camera::updateAnims(double window_height) {
	double world_center = window_height / 2.0 + ((*this).getCamY() - world_height / 2.0) * scale, 
		   world_dist = world_center - window_height / 2.0,
		   max_dist = window_height / 2.0 + world_height * scale / 2.0 - 7.0;

	if (abs(world_dist) > max_dist) {
		double delta = abs(world_dist) - max_dist;
		delta /= scale;
		if (world_dist > 0) {
			cam_y -= delta / 3.0;
		} else {
			cam_y += delta / 3.0;
		}
	}

}