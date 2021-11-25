#ifndef UTILS_H
#define UTILS_H

#include<fstream>
#include<string>
#include<chrono>
#include<iostream>

#include<glad\glad.h>
#include<GLFW\glfw3.h>

namespace Utils {
	//����� ��� �������� ���� ������ � �������. ����������� ���� - ������� UPS � FPS �� 30 ��� � �������.
	//MetricsCounter(int frequency): frequency - ������� � ������� ����� �������� ��� ����. �������� ������� ��������
	// UPS �� 30 ��� �� �������
	//������ �������� ��������������� (���, ����������, ����) ����� �������� ����� tick()
	class MetricsCounter {
	private:
		long counters_cnt,
			*data, result;
		double result_corrected,
				*data_timings;

	public:
		MetricsCounter(int frequency);
		~MetricsCounter();

		//������� ���������� �������
		void tick();
		//��������� ��������
		int getResult() { return result; };
		//��������� � ��������� �� �����
		double getResultAC() { return result_corrected; };
	};
};

double get_current_time();
char* readFileChar(const char* filepath);
unsigned int compileShader(const char* filepath, GLenum shader_type);
unsigned int compileProgram(unsigned int vertex_shader, unsigned int fragment_shader);
unsigned int loadProgram(std::string name);

#endif