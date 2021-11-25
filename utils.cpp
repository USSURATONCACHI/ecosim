#include "utils.h"

static const std::chrono::time_point<std::chrono::system_clock> PROGRAM_START = std::chrono::system_clock::now();

Utils::MetricsCounter::MetricsCounter(int frequency) : counters_cnt(frequency), result(0), result_corrected(0.0) {
	//Счетчики событий
	data = new long[frequency];
	//Когда их нужно сбрасывать (раз в секунду по таймингу)
	data_timings = new double[frequency];

	double step = 1.0 / frequency,
		   now = get_current_time();
	for (int i = 0; i < frequency; i++) {
		data[i] = 0;
		data_timings[i] = now + i*step + 1.0;
	}
}
//Считаем все что нужно
void Utils::MetricsCounter::tick() {
	double now = get_current_time();
	for (int i = 0; i < counters_cnt; i++) {
		//Засчитываем событие
		data[i]++;
		//Если какой либо из счетчиков пора сбрасывать
		if (now >= data_timings[i]) {
			//Записываем результаты (обычные и с поправкой)
			result = data[i];
			result_corrected = static_cast<double>(data[i]) / (now - data_timings[i] + 1.0);
			//Сбрасываем и выставляем новый таймер
			data[i] = 0;
			data_timings[i] += 1.0;
		}
	}
};
Utils::MetricsCounter::~MetricsCounter() {
	delete[] data;
	delete[] data_timings;
}

//Возвращает время, прошедшее с запуска программы в секундах
double get_current_time() {
	std::chrono::time_point<std::chrono::system_clock> now = std::chrono::system_clock::now();
	auto duration = now - PROGRAM_START;
	return std::chrono::duration_cast<std::chrono::microseconds>(duration).count() / 1000000.0;
}

char* readFileChar(const char* filepath) {
    char* buffer;

    std::ifstream t;
    int length;
    t.open(filepath);           // open input file
    t.seekg(0, std::ios::end);    // go to the end
    length = t.tellg();           // report location (this is the length)
    t.seekg(0, std::ios::beg);    // go back to the beginning
    buffer = new char[length];    // allocate memory for a buffer of appropriate dimension
    t.read(buffer, length);       // read the whole file into the buffer
    t.clear();
    t.seekg(0, std::ios::beg);
    int lines_count = std::count(std::istreambuf_iterator<char>(t),
        std::istreambuf_iterator<char>(), '\n');
    t.close();                    // close file handle

    //Ставим ноль в этой позиции, ибо в этом месте начинается некий символ
    buffer[length - lines_count] = 0;

    return buffer;
}

unsigned int compileShader(const char* filepath, GLenum shader_type) {
    GLchar const* source[] = { readFileChar(filepath) };
    std::cout << filepath << " read" << std::endl;

    unsigned int shader;
    shader = glCreateShader(shader_type);
    glShaderSource(shader, 1, source, NULL);
    glCompileShader(shader);

    int  success;
    char infoLog[2048];
    glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
    if (!success) {
        glGetShaderInfoLog(shader, 2048, NULL, infoLog);
        std::cout << "Shader " << filepath << " compilation error:\n" << infoLog << std::endl;
        exit(-2);
    }

    std::cout << "Shader " << filepath << " compiled" << std::endl;

    return shader;
}
unsigned int compileProgram(unsigned int vertex_shader, unsigned int fragment_shader) {
    unsigned int program;
    program = glCreateProgram();
    glAttachShader(program, vertex_shader);
    glAttachShader(program, fragment_shader);
    glLinkProgram(program);

    int  success;
    char infoLog[2048];
    glGetProgramiv(program, GL_LINK_STATUS, &success);
    if (!success) {
        glGetProgramInfoLog(program, 512, NULL, infoLog);
        std::cout << "Shaders linking error:\n" << infoLog << std::endl;
        exit(-3);
    }
    std::cout << "Program linked" << std::endl;
    return program;
}
unsigned int loadProgram(std::string name) {
    unsigned int vertexShader = compileShader(("assets/" + name + ".vert").c_str(), GL_VERTEX_SHADER);
    unsigned int fragmentShader = compileShader(("assets/" + name + ".frag").c_str(), GL_FRAGMENT_SHADER);
    unsigned int shaderProgram = compileProgram(vertexShader, fragmentShader);
    glUseProgram(shaderProgram);
    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);

    return shaderProgram;
}