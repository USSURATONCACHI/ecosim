#ifndef WORLD_H
#include<cstdint>
#include<glad\glad.h>
#include<GLFW\glfw3.h>
#include<random>
#include "utils.h"
#define WORLD_H

class World;

enum class BotActionType {
	NOTHING,
	MOVE_BOT,
};
struct BotAction {
	BotActionType type;
	int new_pos;
};
class Bot {
public:
	bool exists;
	uint32_t id;
	int x, y;
	int world_width;

	BotAction current_action;
	World* world;

	Bot();
	Bot(uint32_t id, int x, int y, World* world);

	void update();
};
class World {
private:
	int32_t width;
	int32_t height;
	long current_tick;

	unsigned char* texture;

public:
	long size;
	Bot* bots;
	long* bots_grid, last_bot;
	Utils::MetricsCounter ups;

	World(int32_t w, int32_t h);

	void update();
	void spawnRandom(int count);

	unsigned char* getTexture() { return texture; };
	int getWidth()  { return width; };
	int getHeight() { return height; };
	long getCurTick() { return current_tick; };

	Bot* getBots() { return bots; };
	void setBots(Bot* ptr) { bots = ptr; };
	~World();
};
class Camera {
private: 
	//Параметры непосредственно камеры в мире
	double cam_x, cam_y, vel_x, vel_y, scale;
	//Мира
	double world_width, world_height;
	//Параметры мыши
	double prev_mouse_x, prev_mouse_y;
	//Отметки вермени
	double last_drag;
	bool lmb;

public:
	Camera(int32_t world_w, int32_t world_h, double scale);
	void cursor_callback(GLFWwindow* window, double xpos, double ypos);
	void mouse_buttons_callback(GLFWwindow* window, int button, int action, int mods);
	void scroll_callback(GLFWwindow* window, double xoffset, double yoffset);

	double getCamX();
	double getCamY();
	double getCamScale() { return scale; };
	double getVelX() { return vel_x; };
	double getVelY() { return vel_y; };

	double getWorldX(double xpix, double poly_width);
	double getWorldY(double ypix, double poly_height);

	double getVelOffsetX();
	double getVelOffsetY();

	void setScale(double s) { scale = s; };

	void updateAnims(double window_height);
};
#endif
