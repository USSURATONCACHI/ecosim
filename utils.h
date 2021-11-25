#ifndef UTILS_H
#define UTILS_H

#include<fstream>
#include<string>
#include<chrono>
#include<iostream>

#include<glad\glad.h>
#include<GLFW\glfw3.h>

namespace Utils {
	//Класс для подсчета чего угодно в секунду. Изначальная цель - подсчет UPS и FPS по 30 раз в секунду.
	//MetricsCounter(int frequency): frequency - частота с которой нужно замерять что либо. Например измерят значение
	// UPS по 30 раз за секунду
	//Каждую итерацию подсчитываемого (тик, обновление, кадр) нужно вызывать метод tick()
	class MetricsCounter {
	private:
		long counters_cnt,
			*data, result;
		double result_corrected,
				*data_timings;

	public:
		MetricsCounter(int frequency);
		~MetricsCounter();

		//Подсчет единичного события
		void tick();
		//Результат подсчета
		int getResult() { return result; };
		//Результат с поправкой на время
		double getResultAC() { return result_corrected; };
	};
};

double get_current_time();
char* readFileChar(const char* filepath);
unsigned int compileShader(const char* filepath, GLenum shader_type);
unsigned int compileProgram(unsigned int vertex_shader, unsigned int fragment_shader);
unsigned int loadProgram(std::string name);

#endif