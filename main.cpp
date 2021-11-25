#include<iostream>
#include<fstream>
#include<string>
#include<cstdint>
#include<sstream>
#include<thread>
#include <codecvt>

#include<glad\glad.h>
#include<GLFW\glfw3.h>

#include <ft2build.h>
#include FT_FREETYPE_H 

#include "world.h"
#include "stb_image.h"
#include "utils.h"
#include "ussurgui.h"

//Объявление функций
void         framebuffer_size_callback(GLFWwindow* window, int width, int height);
void         processInput(GLFWwindow* window);

void         cursor_position_callback(GLFWwindow* window, double xpos, double ypos);
void         mouse_button_callback(GLFWwindow* window, int button, int action, int mods);
void         scroll_callback(GLFWwindow* window, double xoffset, double yoffset);

void         world_thread_func(World* w);

GLFWwindow*  init_window();

static Camera world_camera(0, 0, 1);
static double mouse_x = 0, mouse_y = 0;

//Сглаживание
static const int msaa_types[] = { 1, 2, 3, 4, 5, 9, 16 }, 
                 msaa_types_len = sizeof(msaa_types) / sizeof(msaa_types[0]);
static       int cur_msaa = msaa_types_len - 1;

static bool close_world = false;

static ugui::Widget main_button;

int main() {
    std::setlocale(LC_ALL, "");

    std::cout << "Program started" << std::endl;
    GLFWwindow *window = init_window();
    if (!window) { exit(-5); }

    float vertices[] = {
         1.0f,  1.0f, 0.0f,   1.0, 1.0, // top right
         1.0f, -1.0f, 0.0f,   1.0, 0.0, // bottom right
        -1.0f, -1.0f, 0.0f,   0.0, 0.0, // bottom left
        -1.0f,  1.0f, 0.0f,   0.0, 1.0, // top left 
    };
    unsigned int indices[] = {  // note that we start from 0!
        0, 1, 3,   // first triangle
        1, 2, 3    // second triangle
    };
    //VAO
    unsigned int VAO;
    glGenVertexArrays(1, &VAO);
    glBindVertexArray(VAO);
    //VBO
    unsigned int VBO;
    glGenBuffers(1, &VBO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
    std::cout << "VBO loaded" << std::endl;
    //EBO
    unsigned int EBO;
    glGenBuffers(1, &EBO);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);

    //Создание и компиляция шейдеров
    unsigned int shaderProgram = loadProgram("shader");

    //Локация аттрибута, размер (3 числа), тип чисел, нужна ли нормализация (-1 до 1), 
    //размер одного вертекса и откуда начинаются нужные данные в массиве
    //Позиция
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    //tex coords
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);

    //Ищем положение униформы
    int cam_pos_loc = glGetUniformLocation(shaderProgram, "cam_pos");
    int cam_scale_loc = glGetUniformLocation(shaderProgram, "cam_scale");
    int cursor_loc = glGetUniformLocation(shaderProgram, "cursor");
    int world_size_loc = glGetUniformLocation(shaderProgram, "world_size");
    int poly_size_loc = glGetUniformLocation(shaderProgram, "poly_size");
    int msaa_loc = glGetUniformLocation(shaderProgram, "msaa");
    int texture_loc = glGetUniformLocation(shaderProgram, "texture");

    //Настройки рендера
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

    //Мир
    World world = World(700, 400);
    world_camera = Camera(world.getWidth(), world.getHeight(), 600.0 / world.getHeight());
    world.spawnRandom(10000);
    unsigned char* world_texture_ptr = world.getTexture();
    int32_t world_w = world.getWidth(), world_h = world.getHeight();

    std::cout << "Starting world thread" << std::endl;
    std::thread world_thr(world_thread_func, &world);

    //Текстура
    unsigned int texture;
    glGenTextures(1, &texture);
    glBindTexture(GL_TEXTURE_2D, texture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, world_w, world_h, 0, GL_RGBA, GL_UNSIGNED_BYTE, world_texture_ptr);
    glGenerateMipmap(GL_TEXTURE_2D);

    //Инпут
    glfwSetCursorPosCallback(window, cursor_position_callback);
    glfwSetMouseButtonCallback(window, mouse_button_callback);
    glfwSetScrollCallback(window, scroll_callback);

    //Еще юниформы
    glUniform2f(world_size_loc,
        static_cast<float>(world.getWidth()),
        static_cast<float>(world.getHeight()) );

    //Регулирование ФПС
    const double max_fps = 60.01;
    const auto thread_step = std::chrono::microseconds(static_cast<int>(1000000.0 / max_fps));
    double frames_total = 0.0;
    Utils::MetricsCounter fps_counter(30);

    //FreeType
    ugui::FontInitErr error;
    ugui::Font font = ugui::Font("assets/segoeui.ttf", 14, &error);
    if (error != ugui::FontInitErr::SUCCESS) {
        std::cout << "Font is not loaded, terminating..." << std::endl;
        close_world = true;
        world_thr.join();
        glfwTerminate();
    }
    using namespace ugui;

    //Шрифт, начальное положение курсора, цвет, размер шрифта
    TextCursor text_cursor = TextCursor(&font, 10.0f, 10.0f, glm::vec3(1.0f, 1.0f, 1.0f), 14.0f);
    text_cursor.setBorders(300.0, 0.0)->saveSettings();

    Word fps( L"" ), ups(L"");
    ugui::Style style;
    main_button = Widget::createGUI( &style, 0, 0, 260, 400 );
    main_button
        .addButton(200)
        .addText(L"Кнопка")

        .addButton( 200 )
        .addText( L"Кнопка. А если здесь будет гораздо больше текста? Расскажу кринжовую стори, как моя тян вкрашилась в куколда на дарк рейв вписке, после которой она дропнула меня поменяв на другого куна, породив тем самым мою многолетнюю депрессию...", &text_cursor )
        
        .addButton( 200 )
        .addText( L"Статично-динамичный текст. FPS: ", &text_cursor )
        .addText( &fps )

        .addCheckbox( 200 )
        .addText( L"Чекбокс", &text_cursor )

        .addCheckbox( 200 )
        .addText( L"Чекбокс. Побольше текста здесь, чтоб на несколько строк. Мб даже кусок динамики UPS: ", &text_cursor )
        .addText( &ups )
        .addText( L"И еще статический.\n Переносы?\n     Хм...")

        .addCheckbox( 200 )
        .addText( L"Чисто динамический текст обычно работает безотказно" )

        .addLabel( 200 )
        .addText( L"Неинтерактивный текст. Итак, дело было в 2к17 году. Я бэкал с воркаута чилля под топовый плейлист своего айпода как вдруг в телеге мне пришел новый месседж, в котором дноклы приглашали меня залететь на пожилую хату, ведь их предки смылись на дачу и можно было всю ночь флексить на дарк рейв вписке. И бтв хоть я и являюсь социофобом, агностиком и мизантропом, ради лулзов я решил все таки посетить этот ивент. Заказав каршеринг и достав свой джул, я стал ждать в падике неподалеку, где по рофлу палил зашкварные тиктоки от инстасамки, володи xxl, в которых блоггеры диссили и шеймили друг друга. И тут спустя пол часа на мой айфон приходит голосовуха с конфы класса...", &text_cursor )
    
        .addSlider( 200 )
        .addText( L"Semen count" )
        
        .addButton( 200 )
        .addText(L"Lorem ipsum dolot sit amet.")
    ;
    /*
    main_button
        //.posModeAuto()
        .add( Widget::createButton( nullptr, 0, 0, 200 ) )
        .addText( L"Кнопка 9001. А если здесь будет гораздо больше текста? Расскажу кринжовую стори, как моя тян вкрашилась в куколда на дарк рейв вписке, после которой она дропнула меня поменяв на другого куна, породив тем самым мою многолетнюю депрессию..." )
        .add( Widget::createButton( nullptr, 0, 0, 200 ) )
        .addText( L"FPS: ", &text_cursor )
        .addText( &fps )
        .add( Widget::createCheckbox( nullptr, 0, 0, 200 ) )
        .addText( L"Я за путина" )
        .add( Widget::createCheckbox( nullptr, 0, 0, 200 ) )
        .addText( L"Считаю, что гачи ремиксы являются классикой и их стоит добавить в палату мер и весов как точку отсчета музыки." )
        .add( Widget::createLabel( nullptr, 0, 0, 200 ) )
        .addText( L"Итак, дело было в 2к17 году. Я бэкал с воркаута чилля под топовый плейлист своего айпода как вдруг в телеге мне пришел новый месседж, в котором дноклы приглашали меня залететь на пожилую хату, ведь их предки смылись на дачу и можно было всю ночь флексить на дарк рейв вписке. И бтв хоть я и являюсь социофобом, агностиком и мизантропом, ради лулзов я решил все таки посетить этот ивент. Заказав каршеринг и достав свой джул, я стал ждать в падике неподалеку, где по рофлу палил зашкварные тиктоки от инстасамки, володи xxl, в которых блоггеры диссили и шеймили друг друга. И тут спустя пол часа на мой айфон приходит голосовуха с конфы класса...", &text_cursor );
    */
    main_button.initVAO();
    //std::cout << "cnt: " << main_button.subsCount() << std::endl;

    //Цикл отрисовки
    while (!glfwWindowShouldClose(window)) {
        double frames_required = get_current_time() * max_fps;
        if (frames_required <= frames_total) {
            std::this_thread::sleep_for(thread_step);
            continue;
        } else {
            int window_width, window_height;
            glfwGetWindowSize(window, &window_width, &window_height);
            processInput(window);

            int worldy = static_cast<int>(floor(world_camera.getWorldY(mouse_y, window_height)));
            int worldx = static_cast<int>(floor(world_camera.getWorldX(mouse_x, window_width)));
            
            world_camera.updateAnims(window_height);

            glUseProgram(shaderProgram);
            glBindTexture(GL_TEXTURE_2D, texture);
            glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, world_w, world_h, GL_RGBA, GL_UNSIGNED_BYTE, world_texture_ptr);

            glUniform2f(cam_pos_loc, static_cast<float>(world_camera.getCamX()), static_cast<float>(world_camera.getCamY()));
            glUniform1f(cam_scale_loc, static_cast<float>(world_camera.getCamScale()));
            glUniform2f(poly_size_loc, static_cast<float>(window_width), static_cast<float>(window_height));
            glUniform1i(msaa_loc, msaa_types[cur_msaa]);
            glUniform3f(cursor_loc, static_cast<float>(worldx), static_cast<float>(worldy), 1.0);

            //Очистка экрана
            glClearColor(0.2f, 0.3f, 0.3f, 1.0f);
            glClear(GL_COLOR_BUFFER_BIT);

            glBindVertexArray(VAO);
            glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
            glBindVertexArray(0);
            fps.replaceText( std::to_wstring( fps_counter.getResult() ) + L" ", 0 );
            ups.replaceText( std::to_wstring( world.ups.getResult() ) + L" ", 0 );
            
            text_cursor.loadSettings()->setWindowSize( static_cast<float>(window_width), static_cast<float>(window_height) )->
                setCursor(340.0f, static_cast<float>(window_height) - 24.0f)->
                draw( L"UPS: " + std::to_wstring( world.ups.getResult() ) + L"\n" );

            main_button.setLocalHeight(static_cast<float>(window_height));
            main_button.render( &text_cursor, static_cast<float>(window_width), static_cast<float>(window_height) );
            
            glfwSwapBuffers(window);
            glfwPollEvents();

            //Заголовок окна
            glfwSetWindowTitle(window, ("EcoSim"));

            fps_counter.tick();
            frames_total++;
        }
    }
    std::cout << "Window should close\n";
    close_world = true;
    world_thr.join();
    std::cout << "Terminating GLFW...\n";
    glfwTerminate();
	return 0;
}

void framebuffer_size_callback(GLFWwindow* window, int width, int height) {
    glViewport(0, 0, width, height);
}

void processInput(GLFWwindow* window) {
    static bool prev_q = false, prev_e = false;
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        glfwSetWindowShouldClose(window, true);

    bool cur_q = glfwGetKey(window, GLFW_KEY_Q) == GLFW_PRESS,
         cur_e = glfwGetKey(window, GLFW_KEY_E) == GLFW_PRESS;
    if (cur_q != prev_q && cur_q) {
        cur_msaa = (cur_msaa - 1 + msaa_types_len) % msaa_types_len;
    }
    if (cur_e != prev_e && cur_e) {
        cur_msaa = (cur_msaa + 1) % msaa_types_len;
    }
    prev_q = cur_q;
    prev_e = cur_e;
}

GLFWwindow* init_window() {
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    //Инициализация окна
    GLFWwindow* window = glfwCreateWindow(900, 650, "EcoSim", NULL, NULL);
    if (window == NULL) {
        std::cout << "Failed to create GLFW window" << std::endl;
        glfwTerminate();
        return nullptr;
    }
    glfwMakeContextCurrent(window);
    std::cout << "Window created successfully" << std::endl;

    //Инициализация GLAD
    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
        std::cout << "Failed to initialize GLAD" << std::endl;
        return nullptr;
    }
    std::cout << "GLAD inited successfully" << std::endl;

    //Размеры окна
    glViewport(0, 0, 900, 650);
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);

    return window;
}

//Коллбек
void cursor_position_callback(GLFWwindow* window, double xpos, double ypos) {
    world_camera.cursor_callback(window, xpos, ypos);
    mouse_x = xpos;
    mouse_y = ypos;
    main_button.mouseMoved( window, static_cast<float>(xpos), static_cast<float>(ypos) );
}
void mouse_button_callback(GLFWwindow* window, int button, int action, int mods) {
    if(action == 0 || !main_button.isMouseOver())
        world_camera.mouse_buttons_callback(window, button, action, mods);
    if(action == 1) main_button.mousePressed( button );
    else if (action == 0) main_button.mouseReleased( button );
}
void scroll_callback(GLFWwindow* window, double xoffset, double yoffset) {
    if (!main_button.isMouseOver()) world_camera.scroll_callback(window, xoffset, yoffset);
    else main_button.mouseWheelMoved(static_cast<float>(yoffset * 14.0f), window);
}


struct WorkerData {
    bool *close_req, *apply, *done;
    int *tick, index, count;
    World* world;
};
//Поток мира
void world_thread_func(World* w) {
    /*const int threads_count = 4;
    bool threads_stop = false;
    int current_tick = 0;
    int threads_done[threads_count] = { 0 };
    bool threads_apply[threads_count] = { false };

    for (int i = 0; i < threads_count; i++) {
        threads_done[i] = 0;
    }*/
    //WorkerData{ &threads_stop, &(threads_apply[i]), &(threads_done[i]), 
                //&current_tick, i, threads_count, w };

    World* world = w;

    while (!close_world) {
        world->update();
    };

    std::cout << "World thread done" << std::endl;
    return;
}

void world_loop(WorkerData data) {

    bool* close_req = data.close_req, 
        *apply = data.apply, 
        *done = data.done;
    int *tick = data.tick, 
        index = data.index, 
        count = data.count;
    World* world = data.world;

    float cur_tick = 0;
    int start, end;
    bool is_last = index == (count - 1);
    while (!*close_req) {
        start = world->last_bot * index / count;
        end = is_last ? world->last_bot : world->last_bot * (index + 1) / count;
        if (*apply) {
            //world->applyRange(start, end);
            (*apply) = false;
            (*done) = true;
        } else if (*tick > cur_tick) {
            //world->updateRange(start, end);
            (*done) = true;
            (*tick)++;
        }
    }
}