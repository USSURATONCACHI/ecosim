#ifndef USSURGUI_H
#define USSURGUI_H

#include <iostream>
#include <map>
#include <algorithm>
#include <iterator>
#include <vector>
#include <glm/glm.hpp>
#include <ft2build.h>
#include FT_FREETYPE_H 

#include<glad\glad.h>
#include<GLFW\glfw3.h>

#include "utils.h"

static const int ROWS_BUFFER_SIZE = 100;


namespace ugui {
    class FontShaderWrapper {
    public:
        bool inited;
        unsigned int id,
            text_color,
            max_rectangle;

        FontShaderWrapper() : inited(false), id(0), text_color(0), max_rectangle(0) {}

        void setTextColor( glm::vec3 color ) { glUniform3f( text_color, color.r, color.g, color.b ); }
        void setMaxRect( glm::vec4 rect ) { glUniform4f(max_rectangle, rect.x, rect.y, rect.z, rect.w); }
        void setMaxRect( float x, float y, float z, float w ) { glUniform4f( max_rectangle, x, y, z, w ); }
    };

    class WidgetShaderWrapper {
    public:
        bool inited;
        unsigned int id,
            background_color,
            border_color,
            rectangle_size,
            border_size,
            click_zone_color,
            click_zone_point,
            now_time,
            max_rectangle;

        WidgetShaderWrapper(): inited(false) {}
        /*WidgetShaderWrapper(std::string shader_name) {
            id = loadProgram(shader_name);
            background_color = 
        };*/
            //US_BGCOL, US_BDCOL, US_RECTSIZE, US_BDSIZE, US_CLZ_COLOR, US_CLZ_POINT, US_NOW, US_MAXRECT
    };

    enum WidgetType {
        WT_NULL       = 0b00000000,
        WT_BUTTON     = 0b00000001,
        WT_CHECKBOX   = 0b00000010,
        WT_LABEL      = 0b00000100,
        WT_BACKGROUND = 0b00001000,
        WT_SLIDER     = 0b00010000,

        WT_ANY        = 0b11111111,//Для Style, не используется непосредственно Widget
        WT_COUNT      = 5, //Количество типов
    };
    /*
    Совмещаются через |. Левые восемь бит обозначают оба состояния сразу. 
    0 - оба сразу (и true и false), 1 - то, что в правой части.
    В правой части текущие данные. 1 - true, 0 - false.
    Позволяет проверить совпадение всех параметров в трех возможных вариантах лишь четыремя
    побитовыми операциями и упрощает некоторые вычисления.*/
    enum WidgetBoolState {
        HOV_FALSE = 0b0000000100000000,
        HOV_TRUE  = 0b0000000100000001,

        PRS_FALSE = HOV_FALSE << 1,
        PRS_TRUE  = HOV_TRUE  << 1,

        FOC_FALSE = HOV_FALSE << 2,
        FOC_TRUE  = HOV_TRUE  << 3,

        ACT_FALSE = HOV_FALSE << 3,
        ACT_TRUE  = HOV_TRUE  << 3,

        WBS_ANY = 0,

        //Константы для более удобного ведения проверок
        WBS_COUNT = 4,                       //Количество параметров
        WBS_CUR_BITS = (1 << WBS_COUNT) - 1, //Все биты параметров равны одному
        WBS_LEFT_PART  = 255 << 8,           //Все биты левой части
        WBS_RIGHT_PART = 255,                //Все биты правой части
        WBS_PART_SIZE = 8,                   //Размер одной части (8 бит по умолчанию - запас под 8 параметров)
    };
    enum class WidgetColors {
        BACKGROUND = 0,
        BORDER,
        FONT,
        CLICK_ZONE,

        COUNT,
    };
    enum class WidgetSizes {
        BORDER = 0,
        FONT,
        LINES_DIST,
        
        PADDING_L,
        PADDING_R,
        PADDING_T,
        PADDING_B,

        MARGIN_L,
        MARGIN_R,
        MARGIN_T,
        MARGIN_B,

        COUNT,
    };
    //Для Style
    static const int WIDGET_STATES_COUNT = WT_COUNT << WBS_COUNT;          //Кол-во типов виджета и его состояний (4 булеана)
    static const int COLORS_COUNT = static_cast<int>(WidgetColors::COUNT); //Три цвета - цвет фона, рамки и текста
    static const int SIZES_COUNT = static_cast<int>(WidgetSizes::COUNT);   //Размер рамки, размер шрифта

    class Word;
    class Style;
    class Widget;
    class WordList;
    enum WidgetType;

    enum class FontInitErr {
        SUCCESS,
        FT_LIB_INIT_ERR,
        UNKNOWN_FILE_FORMAT,
        FILE_LOAD_ERR,
        PIXEL_SIZE_ERR,
    };

    struct Character {
        unsigned int texture_id;    //Айди текстуры опенгл
        float width, height;        //Размер текстуры
        float bearing_x, bearing_y, advance;

        unsigned char* buffer;
    };

    class Texture {
    public:
        unsigned int texture_id;            //Айди текстуры опенгл
        float width, height;    //Ширина, высота текстуры в пикселях, количество строк, которые текстура содержит
        float offset_y, offset_x;           //Размеры в единицах текста
        float start_x, start_y, end_x, end_y, font_size, vertical_shift, top_offset;

        int rows_count;
        bool is_copied;

        Texture();
        Texture(unsigned int tex_id, float w, float h, float offsety, float offsetx, float startx, float starty, 
                float endx, float endy, float fsize, float vert_shift, float top_offs, int rowscount);
        Texture(const Texture &t);
        Texture(Texture *copy_from, bool make_original = false);
        ~Texture();
    };

    class Font {
    private:
        std::map<wchar_t, Character> charmap;
        float font_size;

    public:
        unsigned int vao, vbo;

        Font(std::string filepath, unsigned int font_size, FontInitErr *error_buffer);
        ~Font();

        //void renderText(std::wstring text, float x, float y, float scale, glm::vec3 color, float window_width, float window_height, float max_width = 0, float start_x = -1000000.0, float rows_distace = 5);
        void renderChar(Character ch, float x, float y, float scale, float win_w, float win_h);
        void loadChar(wchar_t c, FT_Face face);

        //Texture buildTexture(std::wstring text, float max_width = 0, float rows_distace = 5);
        /*void drawTexture(Texture* tex, float x, float y, float window_width, float window_height, glm::vec3 color, float scale = 1.0);
        */

        float getFontSize() { return font_size; };

        std::map<wchar_t, Character>* getCharmap() { return &charmap; };
    };

    //Класс для удобного вывода текста
    //Содержит параметры вывода текста и место, позволяет вместе выводить динамический и статический текст
    class TextCursor {
    private:
        float settings_save[13], rows_width_buffer[ROWS_BUFFER_SIZE];
        float rows_distance, font_size;
        float start_x, start_y;
        float cursor_x, cursor_y, scale;
        float max_width, max_height;
        int start_line;

        float window_width, window_height;

        glm::vec3 color;
        Font* font;
        wchar_t* word_buf;

    public:
        TextCursor();
        ~TextCursor();
        TextCursor(Font* font, float x, float y, glm::vec3 font_color, float font_size);

        TextCursor* setCursor(float x, float y);
        TextCursor* setBorders(float width, float height = 0.0);
        TextCursor* setScale(float s);
        TextCursor* setFont(Font* f);
        TextCursor* setFontSize(float size);
        TextCursor* setColor(glm::vec3 col) { color = col; return this; };
        TextCursor* setWindowSize(float w, float h) { window_width = w; window_height = h; return this; };
        TextCursor* setRowsDist(float rd) { rows_distance = rd; return this; };

        TextCursor* resetBorders() { max_width = 0.0f; max_height = 0.0f; };

        TextCursor* saveSettings();
        TextCursor* loadSettings();

        TextCursor* draw(std::wstring text, float *getHeight = nullptr);
        TextCursor* draw(Texture* texture, float *getHeight = nullptr );
        TextCursor* draw(Word* word, float *getHeight = nullptr );
        TextCursor* draw( WordList *list );

        TextCursor* moveCursor(std::wstring text);

        TextCursor* moveCursorPix(float xpix, float ypix) { cursor_x += xpix; cursor_y += ypix; return this; };
        
        Texture     buildTexture(std::wstring text, bool centered = false);

        unsigned int getVAO() { return font->vao; };
        unsigned int getVBO() { return font->vbo; };

        float getFontSize() { return font_size; }; 
        float getCursorY() { return cursor_y; }
        float getCursorX() { return cursor_x; }

        friend class Word;
    };

    //Записывает данные буффера from в определенный квадрат буффера to
    void concatBuffers(unsigned char* from, unsigned char* to, int f_w, int f_h, int t_w, int t_h, float f_x, float f_y, float scale = 1.0);


    enum class WordState {
        NULL_WORD,
        DYNAMIC,
        TEXTURE_OWNED,
        TEXTURE_REFERENCE,
    };
    class Word {
    private:
        WordState state;
        std::wstring text;
        Texture *texture;

    public:
        Word(std::wstring text, TextCursor *cursor = nullptr, bool is_static = false);
        Word(Texture t);    //Статический
        Word(Texture *t);   //Статический со ссылкой на текстуру

        void append     (std::wstring ap_text); //Добавить текста (только если динамический)
        void replaceText(std::wstring rep_text, int from);         //Заменить кусок текста
        void removeTo   (int to_symbol);                           //Уменьшить текст до конкретного символа
        void removeBy   (int by_symbols);                          //Уменьшить текст на кол-во символов

        void setState(WordState s) { state = s; };

        int       getLength();
        WordState getState();

        ~Word();

        friend class Widget;
        friend class TextCursor;
    };


    class Style {
    private:
        float *colors, //3 флоата на цвет
              *sizes;

        int anim_start, anim_end;
        float anim_time;

    public:
        Style();
        ~Style();

        static int getStateColorID( WidgetType type, int state);
        //Установить цвет для типов виджета type (можно совмещать несколько через "|" ) в состоянии
        //подпадающем под hover, press и focus
        void setColor(WidgetType type, int state, WidgetColors color_type, glm::vec3 color);
        //Аналогично, но с размером
        void setSize (WidgetType type, int state, WidgetSizes size_type, float size);

        glm::vec3 getColor(WidgetType type, int state, WidgetColors color_type);
        glm::vec3 getColor(int id);

        float getSize(WidgetType type, int state, WidgetSizes size_type );
        float getSize(int id);

        glm::vec3 getColorInterp(int id_start, int id_end, WidgetColors color_type, float interp);
        float     getSizeInterp( int id_start, int id_end, WidgetSizes size_type, float interp );

        void setAnimParams(int id_start, int id_end, float interp);
        glm::vec3 getColor(WidgetColors color_type);
        float     getSize(WidgetSizes size_type );
    };

    //Набор Word, чтобы отрисовывать все разом
    class WordList {
    private:
        std::vector<Word*> words;   //Список Word для рендера
        std::vector<unsigned char> states; //Байт булеанов. Последний - владеет ли лист этим Word, второй - включено ли Word
        float cur_height;   //Высота всего списка в пикселях. Пересчитывается каждый кадр

    public:
        WordList();
        ~WordList();

        void add(Word w);
        void add(Word *w, bool is_owned = false);

        void render(TextCursor *cursor);

        int length();
        void enableWord(int id);
        void disableWord(int id);
        void toggleWord(int id);

        Word* getElem(int id);
        bool isElemOwned(int id);
        float getLastHeight() { return cur_height; }
    };
    enum class WidgetPosMode {
        RELATIVE,
        ABSOLUTE,
    };
    
    class Widget {
    private:
        WidgetType type;

        float local_x, local_y;  //X и Y относительно родительского элемента
        float last_rx, last_ry;  //Абсолютный Х и Y, высчитываются во время рендера

        float local_w, local_h;  //Ширина и высота, если указаны
        float last_rw, last_rh;  //Последне значение размеров при рендере. Если local_w и local_h не равны нулю - эти и локальные значения равны.
        float last_subs_h; //Внутренный размер
        
        bool mouse_hovering; //Мышь над элементом и его не закрывают другие виджеты
        bool mouse_over;     //Мышь в пределах виджета
        bool mouse_pressing; //Мышь сейчас нажата на элементе.
        bool is_focused;
        bool is_shown;   //Если false - элемент скрыт с экрана 
        bool is_active;  //Пока только для чекбокса

        float scroll_offset, //Позиция скролла
              scroll_speed,  //Скорость
              scroll_start;  //Время его начала в секундах

        //Анимация перехода цветов
        double anim_start;                  //Время старта анимации
        int anim_state_from, anim_state_to; //ID состояний элемента в начале и в конце анимации
        float click_x, click_y, click_time; //Позиция и время последнего нажатия по элементу

        WordList label; // Название/текст на элементе

        std::vector<Widget*> sub_widgets;    //Дочерние элементы (ветка)
        std::vector<bool> sub_widgets_owned; //Какие из них во владении, а не переданы готовой ссылкой
        Widget *parent_widget;               //Родительский элемент (если есть)

        WidgetPosMode subs_position_mode;  //Режим позиционирования
        bool is_last_in_row;               //Переносить ли следующий после этого элемент на новую строку

        Style *style;           //Стиль откуда берутся цвета
        bool style_inherited;   //Унаследован ли он

        unsigned int vao, vbo, ebo; //Штуки OpenGL

        bool lmb, rmb;           //Левая и правая кнопки мыши
        double last_mouse_move;  //Время движения в секундах
        float mouse_x, mouse_y;  //Положение мыши

        GLFWwindow *last_window;
        bool mouse_recalc_required;

        void inheritStyle(Style *new_style);

    public:

        Widget( Widget *parent, WidgetType type, float xpos, float ypos, float w, float h, Style *el_style );
        Widget();

        static Widget createGUI(Style *s, float x, float y, float width, float height = 0.0f);

        float getScale();

        Widget& addButton  ( float w = 0.0f, float h = 0.0f, Style *s = nullptr );
        Widget& addCheckbox( float w = 0.0f, float h = 0.0f, Style *s = nullptr );
        Widget& addLabel   ( float w = 0.0f, float h = 0.0f, Style *s = nullptr );
        Widget& addSlider  ( float w = 0.0f, float h = 0.0f, Style *s = nullptr );

        Widget& add( Widget child );
        Widget& add( Widget *child );
        Widget& addText( Word *w );
        Widget& addText( Word w );
        Widget& addText( std::wstring text, TextCursor *cursor = nullptr );

        Widget& subsPos(WidgetPosMode mode) { subs_position_mode = mode; return *this; };

        //Widget& posModeAuto();
        //Widget& posModeManual();
        //Widget& br();

        void setParent(Widget *new_parent);

        void initVAO();
        void render(TextCursor *cursor, float widnow_width, float window_height, float x_offset = 0.0f, float y_offset = 0.0f, glm::vec4 parent_rect = glm::vec4(0.0f, 0.0f, 0.0f, 0.0f));

        bool mouseMoved     (GLFWwindow* window, float pos_x, float pos_y, bool highest = true, bool over_parent = true); //Возвращает true, если мышь над каким либо из элементов
        void mousePressed   ( int button, bool over_parent = true );
        void mouseReleased  (int button);
        void mouseWheelMoved(float l, GLFWwindow *w );

        void updateAnim();
        void updateStyleParams();

        float getScroll();
        float getScrollVel();
        void updateScroll();

        ~Widget();

        //Геттеры и сеттеры
        float getLocalX()      { return local_x; }
        float getLocalY()      { return local_y; }
        float getLocalWidth()  { return local_w; }
        float getLocalHeight() { return local_h; }

        void setLocalX     ( float x ) { local_x = x; }
        void setLocalY     ( float y ) { local_y = y; }
        void setLocalWidth ( float w ) { local_w = w; last_rw = w; }
        void setLocalHeight( float h ) { local_h = h; last_rh = h; }

        bool isMouseOver()     { return mouse_over;     }
        bool isMouseHovering() { return mouse_hovering; }
        bool isMousePressing() { return mouse_pressing; }
        bool isFocused()       { return is_focused;     }
        bool isShown()         { return is_shown;       }

        long subsCount() { return sub_widgets.size(); }
    };

    int widgetTypeToInt(int type);

}

#endif