#include "ussurgui.h"
using namespace ugui;

//Заранее выделенный массив памяти для разбивки букв в слова
static const int WORD_BUFFER_SIZE = 128;

//Шейдер для всех шрифтов одинаковый, а эти данные не меняются после загрузки
static FontShaderWrapper FONT_SHADER = FontShaderWrapper();

static const std::wstring LETTERS = L"абвгдеёжзийклмнопрстуфхцчшщьыъэюяАБВГДЕЁЖЗИЙКЛМНОПРСТУФХЦЧШЩЬЭЪЭЮЯabcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ";
static const wchar_t BR_CHAR = L"\n"[0];

//Шейдер для элементов
static bool UGUI_SHADER_INITED = false;
static unsigned int US_ID;
static unsigned int US_BGCOL, US_BDCOL, US_RECTSIZE, US_BDSIZE, US_CLZ_COLOR, US_CLZ_POINT, US_NOW, US_MAXRECT; 
//Шейдер для ползунка
static bool UGUI_SLIDER_SHADER_INITED = false;
static unsigned int UGSS_ID, UGSS_BGCOL, UGSS_BDCOL, UGSS_RECTSIZE, UGSS_BDSIZE, UGSS_MAXRECT, UGSS_SLPAR;

//TEMP
void cout_binary( int n );

Texture::Texture() :
    texture_id( 0 ), width( 0.0f ), height( 0.0f ),
    offset_x ( 0.0f ), offset_y( 0.0f ),
    start_x( 0.0f ), start_y( 0.0f ), end_x( 0.0f ), end_y( 0.0f ),
    font_size( 0.0f ), vertical_shift( 0.0f ), top_offset( 0.0f ),
    rows_count( 0 ), is_copied( false ) {};

Texture::Texture( unsigned int tex_id, float w, float h, float offsety, float offsetx, float startx, float starty,
    float endx, float endy, float fsize, float vert_shift, float top_offs, int rowscount ) :
    texture_id( tex_id ), width( w ), height( h ),
    offset_x ( offsetx ), offset_y( offsety ),
    start_x( startx ), start_y( starty ), end_x( endx ), end_y( endy ),
    font_size( fsize ), vertical_shift( vert_shift ), top_offset( top_offs ),
    rows_count( rowscount ), is_copied( false ) {}

Texture::Texture( const Texture &t ) :
    texture_id( t.texture_id ), width( t.width ), height( t.height ),
    offset_x ( t.offset_x ), offset_y( t.offset_y ),
    start_x( t.start_x ), start_y( t.start_y ), end_x( t.end_x ), end_y( t.end_y ),
    font_size( t.font_size ), vertical_shift( t.vertical_shift ), top_offset( t.top_offset ),
    rows_count( t.rows_count ), is_copied( true ) {
};

Texture::Texture(Texture *copy_from, bool make_original) : 
    texture_id(copy_from->texture_id), width( copy_from->width ), height( copy_from->height),
    offset_x ( copy_from->offset_x ), offset_y( copy_from->offset_y ), 
    start_x( copy_from->start_x ), start_y( copy_from->start_y ), end_x( copy_from->end_x ), end_y( copy_from->end_y ),
    font_size( copy_from->font_size ), vertical_shift( copy_from->vertical_shift ), top_offset( copy_from->top_offset ),
    rows_count( copy_from->rows_count ), is_copied(!make_original)
{
    if(make_original) copy_from->is_copied = true;
}
Texture::~Texture() {
    //if(!is_copied) glDeleteTextures(1, &texture_id);
}

//Создание шрифта
Font::Font(std::string filepath, unsigned int font_size, FontInitErr* error_buffer) {
    charmap = std::map<wchar_t, Character>();
    vao = 0;
    vbo = 0;

    FT_Library lib;
    FT_Face face;

    if (FT_Init_FreeType(&lib)) {
        std::cout << "Font init error: Could not init FreeType Library" << std::endl;
        *error_buffer = FontInitErr::FT_LIB_INIT_ERR;
        return;
    }

    //Загрузка шрифта
    FT_Error error = FT_New_Face(lib, filepath.c_str(), 0, &face);
    if (error == FT_Err_Unknown_File_Format) {
        std::cout << "Font init error: Unknown File Format" << std::endl;
        *error_buffer = FontInitErr::UNKNOWN_FILE_FORMAT;
    }
    else if (error) {
        std::cout << "Font init error: Could not load file" << std::endl;
        *error_buffer = FontInitErr::FILE_LOAD_ERR;
    }

    //Настрой очка
    if (FT_Set_Pixel_Sizes(face, 0, font_size)) {
        std::cout << "Font init error: Failed to set pixel size" << std::endl;
        *error_buffer = FontInitErr::PIXEL_SIZE_ERR;
    }

    glPixelStorei(GL_UNPACK_ALIGNMENT, 1); // disable byte-alignment restriction

    for (wchar_t c = 0; c < 256; c++) {
        loadChar(c, face);
    }
    std::wstring additional = L"абвгдеёжзийклмнопрстуфхцчшщьыъэюяАБВГДЕЁЖЗИЙКЛМНОПРСТУФХЦЧШЩЬЭЪЭЮЯ" + std::wstring(L"") + static_cast<wchar_t>(L"✔"[0]);
    for (int i = 0; i < additional.length(); i++) {
        wchar_t c = additional[i];
        loadChar(c, face);
    }

    FT_Done_Face(face);
    FT_Done_FreeType(lib);
    std::cout << "Font loaded successfully" << std::endl;

    if (!FONT_SHADER.inited) {
        FONT_SHADER.id = loadProgram("font");
        FONT_SHADER.text_color = glGetUniformLocation( FONT_SHADER.id, "textColor"     );
        FONT_SHADER.max_rectangle = glGetUniformLocation( FONT_SHADER.id, "max_rectangle" );

        FONT_SHADER.setMaxRect( 0.0f, 0.0f, 100000.0f, 100000.0f );
    }

    //Инициализируем VAO, VBO для последующего использования
    glGenVertexArrays(1, &vao);
    glGenBuffers(1, &vbo);
    glBindVertexArray(vao);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(float) * 6 * 4, NULL, GL_DYNAMIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, 4 * sizeof(float), 0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);

    this->font_size = static_cast<float>(font_size);

    *error_buffer = FontInitErr::SUCCESS;
}
Font::~Font() {
    std::map<wchar_t, Character>::iterator it = charmap.begin();
    //Очистка использованной символами памяти
    while (it != charmap.end()) {
        Character ch = it->second;
        delete[] ch.buffer;
        glDeleteTextures(1, &ch.texture_id);
        it++;
    }
    if(vbo) glDeleteBuffers(1, &vbo);
}

//Отрисовка строки text по два треугольника (один прямоугольник) на каждый символ

//Рендер отдельного символа
void Font::renderChar(Character ch, float x, float y, float scale, float window_width, float window_height) {
    //Распологаем полигон для символа
    float xpos = x + ch.bearing_x * scale;
    float ypos = y - (ch.height - ch.bearing_y) * scale;

    float w = ch.width * scale;
    float h = ch.height * scale;

    float xmin = xpos * 2.0f / window_width - 1.0f,
        ymin = ypos * 2.0f / window_height - 1.0f,
        xmax = (xpos + w) * 2.0f / window_width - 1.0f,
        ymax = (ypos + h) * 2.0f / window_height - 1.0f;


    float vertices[6][4] = {
        { xmin, ymax, 0.0f, 0.0f },
        { xmin, ymin, 0.0f, 1.0f },
        { xmax, ymin, 1.0f, 1.0f },

        { xmin, ymax, 0.0f, 0.0f },
        { xmax, ymin, 1.0f, 1.0f },
        { xmax, ymax, 1.0f, 0.0f }
    };
    //Отрисовываем полигон
    glBindTexture(GL_TEXTURE_2D, ch.texture_id);

    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(vertices), vertices);
    glBindBuffer(GL_ARRAY_BUFFER, 0);

    glDrawArrays(GL_TRIANGLES, 0, 6);
}
//Попытка загрузить символ c в выводимый вид из шрифта
void Font::loadChar(wchar_t c, FT_Face face) {
    //Если не получается загрузить символ из шрифта - скипаем
    if (FT_Load_Char(face, c, FT_LOAD_RENDER)) return;
    //Выделяем текстурку в видеопамяти под символ
    unsigned int texture;
    glGenTextures(1, &texture);
    glBindTexture(GL_TEXTURE_2D, texture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RED, face->glyph->bitmap.width, face->glyph->bitmap.rows, 
                 0, GL_RED, GL_UNSIGNED_BYTE, face->glyph->bitmap.buffer );

    //Параматры этой текстуры (приличную часть этого кода я скопировал из документации по freetype)
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    //Выделяем также и текстуру в оперативке (для последующей сборки символов в текстуры строк)
    long size = face->glyph->bitmap.width * face->glyph->bitmap.rows;
    unsigned char* pixbuf = new unsigned char[size];
    for (long i = 0; i < size; i++) pixbuf[i] = face->glyph->bitmap.buffer[i]; 

    //Ну и собираем все нужные данные в структуру, затем запоминаем
    Character character = {
        texture,
        static_cast<float>(face->glyph->bitmap.width), 
        static_cast<float>(face->glyph->bitmap.rows),
        static_cast<float>(face->glyph->bitmap_left),
        static_cast<float>(face->glyph->bitmap_top),
        static_cast<float>(face->glyph->advance.x / 64.0),
        pixbuf
    };
    //if(character.advance < (character.width + 0.5f)) character.advance = character.width + 0.5f;
    getCharmap()->insert(std::pair<wchar_t, Character>(c, character));
}
//В буффер to в прямоугольник с началом в f_x, f_y и размерами f_w, f_h копируются данные из буффера from размером f_w, f_h
void ugui::concatBuffers(unsigned char* from, unsigned char* to, int f_w, int f_h, int t_w, int t_h, float f_x, float f_y, float scale) {
    for (float y = f_y; y < (f_y + f_h*scale); y += 1) {
        int temp_y = static_cast<int>(y);
        for (float x = f_x; x < (f_x + f_w*scale); x += 1) {
            int temp_x = static_cast<int>(x);
            if (temp_x < 0 || temp_x >= t_w || temp_y < 0 || temp_y >= t_h) continue;
            long to_id = static_cast<int>(temp_y) * t_w + static_cast<int>(temp_x),
                from_id = static_cast<int>((y - f_y) / scale) * f_w + static_cast<int>((x - f_x) / scale);
            to[to_id] = (static_cast<int>(to[to_id]) + from[from_id]) > 255 ? 255 : (to[to_id] + from[from_id]);
        }
    }
}


//TextCursor
TextCursor::TextCursor() :
    rows_distance(5.0f), font_size(0.0f),
    start_x(0.0f), start_y(0.0f),
    cursor_x(0.0f), cursor_y(0.0f), scale(1.0f),
    max_width(0.0f), max_height(0.0f), start_line(0),
    color(glm::vec3(1.0f, 1.0f, 1.0f)), font(nullptr), window_width(1.0f), window_height(1.0),
    rows_width_buffer()
{
    for (int i = 0; i < 13; i++) settings_save[i] = 0.0f;
    word_buf = new wchar_t[WORD_BUFFER_SIZE];
};
TextCursor::~TextCursor() {
    delete[] word_buf;
}
TextCursor::TextCursor(Font* font, float x, float y, glm::vec3 font_color, float font_size) :
    rows_distance(5.0f), font_size(font_size),
    start_x(x), start_y(y),
    cursor_x(0.0f), cursor_y(0.0f), scale(font_size / font->getFontSize()),
    max_width(0.0f), max_height(0.0f), start_line(0),
    color(glm::vec3(1.0, 1.0, 1.0)), font(font), window_width(1.0f), window_height(1.0),
    rows_width_buffer()
{
    for (int i = 0; i < 13; i++) settings_save[i] = 0.0f;
    word_buf = new wchar_t[WORD_BUFFER_SIZE];
};

TextCursor* TextCursor::setFont(Font* f) {
    font = f;
    if (!font) return this;
    setFontSize(font_size);
    return this;
}
TextCursor* TextCursor::setFontSize(float size) {
    this->font_size = size;
    if (!font) return this;
    scale = size / font->getFontSize();
    return this;
}
TextCursor* TextCursor::setScale(float s) {
    if (font) font_size = font->getFontSize() * s;
    else      font_size = s * font_size / scale;
    scale = s;
    return this;
}

TextCursor* TextCursor::setCursor(float x, float y) {
    start_x = x; 
    start_y = y; 
    cursor_x = 0; 
    cursor_y = 0;
    return this;
}
TextCursor* TextCursor::setBorders(float width, float height) {
    max_width = width; 
    max_height = height;
    return this;
}

TextCursor* TextCursor::saveSettings() {
    settings_save[ 0] = rows_distance;
    settings_save[ 1] = font_size;
    settings_save[ 2] = start_x;
    settings_save[ 3] = start_y;
    settings_save[ 4] = cursor_x;
    settings_save[ 5] = cursor_y;
    settings_save[ 6] = scale;
    settings_save[ 7] = max_width;
    settings_save[ 8] = max_height;
    settings_save[ 9] = static_cast<float>(start_line);
    settings_save[10] = color.r;
    settings_save[11] = color.g;
    settings_save[12] = color.b;
    return this;
}
TextCursor* TextCursor::loadSettings() {
    rows_distance = settings_save[0];
    font_size     = settings_save[1];
    start_x       = settings_save[2];
    start_y       = settings_save[3];
    cursor_x      = settings_save[4];
    cursor_y      = settings_save[5];
    scale         = settings_save[6];
    max_width     = settings_save[7];
    max_height    = settings_save[8];
    start_line    = static_cast<int>(settings_save[9]);
    color         = glm::vec3(settings_save[10],
                              settings_save[11],
                              settings_save[12]);
    return this;
}

TextCursor* TextCursor::draw(std::wstring text, float *getHeight) {
    if (!font) return this;
    //Базовые настройки рендера
    glUseProgram( FONT_SHADER.id );
    FONT_SHADER.setTextColor(color);
    glActiveTexture(GL_TEXTURE0);
    glBindVertexArray(font->vao);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    float start_cy = cursor_y;

    bool prev_is_space = false;
    int current_letter_num = 0;
    float word_width = 0.0;
    for (int i = 0; i <= text.length(); i++) {
        wchar_t c = i == text.length() ? 0 : text.at(i);
        bool is_space = c == 32;

        //Если пробелы сменяются символами - слово прерывается и его надо вывести
        bool word_continuing = !(is_space ^ prev_is_space) && i != text.length();

        if (!word_continuing || current_letter_num == (WORD_BUFFER_SIZE - 1) || i == text.length()) {
            //Если слово выходит за пределы строки и его можно целиком уместить на строку - перенос
            if ( max_width && cursor_x + word_width > max_width && word_width <= max_width ) {
                cursor_y -= font_size + rows_distance;
                cursor_x = 0;
            }

            //Вывод слова посимвольно
            for (int j = 0; j < current_letter_num; j++) {
                wchar_t w_c = word_buf[j];
                if (w_c == BR_CHAR) {
                    cursor_y -= font_size + rows_distance;
                    cursor_x = 0;
                    continue;
                }

                Character ch = font->getCharmap()->at(w_c);
                

                font->renderChar(ch, start_x + cursor_x, start_y + cursor_y, scale, window_width, window_height);
                //Смещаемся на место следующего символа
                cursor_x += ch.advance * scale;
                
                //Если следующая буква не влезает - перенос.
                if (max_width && cursor_x > max_width) {
                    cursor_y -= font_size + rows_distance;
                    cursor_x = 0;
                }
            }
            current_letter_num = 0;
            word_width = 0.0f;
        }
        word_buf[current_letter_num] = c;
        Character temp_ch = font->getCharmap()->at(c);
        current_letter_num++;
        word_width += temp_ch.advance * scale;

        prev_is_space = is_space;
    }

    //Сброс настроек
    glBindVertexArray(0);
    glBindTexture(GL_TEXTURE_2D, 0);
    glDisable(GL_BLEND);
    if(getHeight) *getHeight = cursor_y - start_cy + font_size;
    return this;
}
TextCursor* TextCursor::moveCursor(std::wstring text) {
    if (!font) return this;

    bool prev_is_space = false;
    int current_letter_num = 0;
    float word_width = 0.0;
    for (int i = 0; i <= text.length(); i++) {
        wchar_t c = i == text.length() ? 0 : text.at(i);
        bool is_space = c == 32;

        //Если пробелы сменяются символами - слово прерывается и его надо вывести
        bool word_continuing = !(is_space ^ prev_is_space) && i != text.length();

        if (!word_continuing || current_letter_num == (WORD_BUFFER_SIZE - 1) || i == text.length()) {
            //Если слово выходит за пределы строки и его можно целиком уместить на строку - перенос
            if (max_width && cursor_x + word_width > max_width && word_width <= max_width) {
                cursor_y -= font_size + rows_distance;
                cursor_x = 0;
            }
            for (int j = 0; j < current_letter_num; j++) {
                wchar_t w_c = word_buf[j];
                if (w_c == BR_CHAR) {
                    cursor_y -= font_size + rows_distance;
                    cursor_x = 0;
                    continue;
                }
                Character ch = font->getCharmap()->at(w_c);

                //Смещаемся на место следующего символа
                cursor_x += ch.advance;

                //Если следующая буква не влезает - перенос.
                if (max_width && cursor_x > max_width) {
                    cursor_y -= font_size + rows_distance;
                    cursor_x = 0;
                }
            }
            current_letter_num = 0;
            word_width = 0.0f;
        }
        word_buf[current_letter_num] = c;
        Character temp_ch = font->getCharmap()->at(c);
        current_letter_num++;
        word_width += temp_ch.advance;

        prev_is_space = is_space;
    }
    return this;
}

Texture     TextCursor::buildTexture(std::wstring text, bool centered) {
    if (!font) return Texture();

    //С какого места начинается текст на текстуре
    if (centered) cursor_x = 0.0f;
    float tx_curx = cursor_x, tx_cury = cursor_y;
    cursor_y = 0.0f;
    //Вычисление размеров текстуры
    float min_x = 1000000.0, min_y = 1000000.0, max_x = 0.0, max_y = 0.0, max_tof = 0.0;
    int rows_count = 0;
    bool prev_is_space = false;
    int current_letter_num = 0;
    float word_width = 0.0, row_width = 0.0;

    for (int i = 0; i <= text.length(); i++) {
        wchar_t c = i == text.length() ? 0 : text.at(i);
        bool is_space = c == 32;

        //Если пробелы сменяются символами - слово прерывается и его надо вывести
        bool word_continuing = !(is_space ^ prev_is_space) && i != text.length();

        if (!word_continuing || current_letter_num == (WORD_BUFFER_SIZE - 1) || i == text.length()) {
            //Если слово выходит за пределы строки и его можно целиком уместить на строку - перенос
            if (max_width && cursor_x + word_width > max_width && word_width <= max_width) {
                cursor_y -= font_size + rows_distance;
                cursor_x = 0;

                if (centered) {
                    rows_width_buffer[rows_count] = row_width;
                    row_width = 0.0f;
                }
                rows_count++;
            }
            for (int j = 0; j < current_letter_num; j++) {
                wchar_t w_c = word_buf[j];
                if(j == 0 && w_c == (L" "[0]) && cursor_x == 0.0f && cursor_y != 0.0f) continue;
                if (w_c == BR_CHAR) {
                    cursor_y -= font_size + rows_distance;
                    cursor_x = 0;

                    if (centered) {
                        rows_width_buffer[rows_count] = row_width;
                        row_width = 0.0f;
                    }
                    rows_count++;
                    word_width = 0.0f;
                    continue;
                }
                Character ch = font->getCharmap()->at(w_c);
                float char_maxy = cursor_y + ch.bearing_y * scale,
                      char_miny = cursor_y + ch.bearing_y * scale - ch.height * scale,
                      char_minx = cursor_x, 
                      char_maxx = cursor_x + ch.advance * scale;

                if (rows_count == 0 && w_c != 32 && ch.bearing_y * scale > max_tof)
                    max_tof = ch.bearing_y * scale;

                if (char_miny < min_y) min_y = char_miny;
                if (char_minx < min_x) min_x = char_minx;
                if (char_maxy > max_y) max_y = char_maxy;
                if (char_maxx > max_x) max_x = char_maxx;

                //Смещаемся на место следующего символа
                cursor_x += ch.advance * scale;
                row_width += ch.advance * scale;
                if (i == text.length()) {
                    rows_width_buffer[rows_count] = row_width;
                }
                //Если следующая буква не влезает - перенос.
                if (max_width && cursor_x > max_width) {
                    cursor_y -= font_size + rows_distance;
                    cursor_x = 0;

                    if (centered) {
                        rows_width_buffer[rows_count] = row_width;
                        row_width = 0.0f;
                    }
                    rows_count++;
                }
            }
            current_letter_num = 0;
            word_width = 0.0f;
        }
        word_buf[current_letter_num] = c;
        Character temp_ch = font->getCharmap()->at(c);
        current_letter_num++;
        word_width += temp_ch.advance * scale;

        prev_is_space = is_space;
    }

    //В каком месте текст заканчивается
    float tx_endx = cursor_x, tx_endy = cursor_y;
    cursor_x = tx_curx;
    cursor_y = 0.0;

    int tex_w = static_cast<int>(centered ? max_width : (max_x - min_x + tx_curx)),
        tex_h = static_cast<int>(max_y - min_y);
    long size = tex_w * tex_h;
    unsigned char* buffer = new unsigned char[size];
    for (long i = 0L; i < size; i++) buffer[i] = 0; 

    prev_is_space = false;
    word_width = 0.0;
    current_letter_num = 0;
    rows_count = 0;
    //Создание текстуры
    for (int i = 0; i <= text.length(); i++) {
        wchar_t c = i == text.length() ? 0 : text.at(i);
        bool is_space = c == 32;

        //Если пробелы сменяются символами - слово прерывается и его надо вывести
        bool word_continuing = !(is_space ^ prev_is_space) && i != text.length();

        if (!word_continuing || current_letter_num == (WORD_BUFFER_SIZE - 1) || i == text.length()) {
            //Если слово выходит за пределы строки и его можно целиком уместить на строку - перенос
            if (max_width && cursor_x + word_width > max_width && word_width <= max_width) {
                cursor_y -= font_size + rows_distance;
                cursor_x = 0;
                rows_count++;
            }
            for (int j = 0; j < current_letter_num; j++) {
                wchar_t w_c = word_buf[j];
                if (j == 0 && w_c == (L" "[0]) && cursor_x == 0.0f && cursor_y != 0.0f) continue;
                if (w_c == BR_CHAR) {
                    cursor_y -= font_size + rows_distance;
                    cursor_x = 0;
                    rows_count++;
                    word_width = 0.0f;
                    continue;
                }
                Character ch = font->getCharmap()->at(w_c);
                if(!centered) 
                    concatBuffers(ch.buffer, buffer, static_cast<int>(ch.width), static_cast<int>(ch.height), 
                        static_cast<int>(tex_w), static_cast<int>(tex_h), cursor_x, tex_h - ch.bearing_y * scale + min_y - cursor_y, scale);
                else 
                    concatBuffers(ch.buffer, buffer, static_cast<int>(ch.width), static_cast<int>(ch.height), 
                        static_cast<int>(tex_w), static_cast<int>(tex_h), cursor_x + (max_width - rows_width_buffer[rows_count]) * 0.5f, tex_h - ch.bearing_y * scale + min_y - cursor_y, scale );

                //Смещаемся на место следующего символа
                cursor_x += ch.advance * scale;

                //Если следующая буква не влезает - перенос.
                if (max_width && cursor_x > max_width) {
                    cursor_y -= font_size + rows_distance;
                    cursor_x = 0;
                    rows_count++;
                }
            }
            current_letter_num = 0;
            word_width = 0.0f;
        }
        word_buf[current_letter_num] = c;
        Character temp_ch = font->getCharmap()->at(c);
        current_letter_num++;
        word_width += temp_ch.advance * scale;

        prev_is_space = is_space;
    }

    cursor_y += tx_cury + tx_endy;

    //Загружаем текстуру в видеопамять
    unsigned int texture;
    glGenTextures(1, &texture);
    glBindTexture(GL_TEXTURE_2D, texture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RED, tex_w, tex_h, 0, GL_RED, GL_UNSIGNED_BYTE, buffer);

    //Параметры текстуры
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    //Перерисовка отдельных частей текстуры не имеет смысла, ибо каждый символ может иметь 
    //уникальную ширину, - этот буффер больше не понадобится
    delete[] buffer;

    return { texture, static_cast<float>(tex_w), static_cast<float>(tex_h), 
             0, 0, tx_curx, tx_cury, tx_endx, tx_endy, font->getFontSize(), min_y, max_tof, rows_count };
}

TextCursor* TextCursor::draw(Texture *text, float *getHeight ) {
    if (!font) return this;
    bool is_inline = cursor_x <= text->start_x;
    float scale = text->font_size/font->getFontSize();
    float start_cy = cursor_y;
    
    glUseProgram( FONT_SHADER.id );
    FONT_SHADER.setTextColor(color);
    glActiveTexture(GL_TEXTURE0);
    glBindVertexArray(font->vao);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    //std::cout << "OUT: " << cursor_x << " " << cursor_y << " " << text->end_y << " " << 0 << " " << 0 << std::endl;
    int offset = static_cast<int>(text->top_offset);
    float inl_shift = is_inline ? 0.0f : (font_size + rows_distance);
    //cursor_y += (is_inline ? (text->top_offset) : -(rows_distance + text->top_offset)) * scale;
    float y = start_y + cursor_y + offset - inl_shift;
    cursor_y += text->end_y - text->start_y - inl_shift;
    cursor_x = text->end_x*scale;

    

    //Расположение полигона на экране
    float xmin = start_x * 2.0f / window_width - 1.0f,
          xmax = (start_x + text->width * scale) * 2.0f / window_width - 1.0f,
          ymin = (y - text->height * scale) * 2.0f / window_height - 1.0f,
          ymax = (y) * 2.0f / window_height - 1.0f;

    float vertices[6][4] = {
        { xmin, ymax, 0.0f, 0.0f },
        { xmin, ymin, 0.0f, 1.0f },
        { xmax, ymin, 1.0f, 1.0f },

        { xmin, ymax, 0.0f, 0.0f },
        { xmax, ymin, 1.0f, 1.0f },
        { xmax, ymax, 1.0f, 0.0f },
    };
    //Рендер текстуры на полигон
    glBindTexture(GL_TEXTURE_2D, text->texture_id);
    glBindBuffer(GL_ARRAY_BUFFER, font->vbo);
    glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(vertices), vertices);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glDrawArrays(GL_TRIANGLES, 0, 6);
    //Сброс настроек
    glBindVertexArray(0);
    glBindTexture(GL_TEXTURE_2D, 0);
    glDisable(GL_BLEND);

    if (getHeight) *getHeight = cursor_y - start_cy + font_size;
    return this;
}

TextCursor* TextCursor::draw(Word *word, float *getHeight ) {
    if (word->state == WordState::DYNAMIC) draw(word->text, getHeight);
    else draw(word->texture, getHeight);
        
    return this;
}
TextCursor* TextCursor::draw( WordList *list ) {
    list->render(this);
    return this;
}

//Word
Word::Word(std::wstring text, TextCursor *cursor, bool is_static) {
    if (!cursor && is_static) {
        state = WordState::NULL_WORD;
        text = L"";
        texture = nullptr;
        return;
    }

    if (is_static) {
        state = WordState::TEXTURE_OWNED;
        this->text = L"";
        texture = new Texture;
        *texture = cursor->buildTexture( text );
    } else {
        state = WordState::DYNAMIC;
        this->text = text;
        texture = nullptr;
    }
}
Word::Word(Texture t)  {
    state = WordState::TEXTURE_OWNED;
    this->text = L"";
    texture = new Texture;
    *texture = t;
}
Word::Word(Texture *t) {
    state = WordState::TEXTURE_REFERENCE;
    this->text = L"";
    texture = t;
}

void Word::append     (std::wstring ap_text) {
    if( state != WordState::DYNAMIC ) return;
    text += ap_text;
}
void Word::replaceText(std::wstring rep_text, int from) {
    if (state != WordState::DYNAMIC) return;
    text = text.substr(0, from) + rep_text;
}
void Word::removeTo   (int to_symbol) {
    if (state != WordState::DYNAMIC) return;
    text = text.substr( 0, to_symbol );
}
void Word::removeBy   ( int by_symbols ) {
    if (state != WordState::DYNAMIC) return;
    text = text.substr( 0, text.length() - by_symbols );
}

int       Word::getLength() { return state == WordState::DYNAMIC ? static_cast<int>(text.length()) : 0; };
WordState Word::getState()  { return state; };

Word::~Word(){
    if(state == WordState::TEXTURE_OWNED) delete texture;
}


//Style
Style::Style(): anim_start(0), anim_end(0), anim_time(0.0f) {
    //8*3*3*3 = 216
    colors = new float[WIDGET_STATES_COUNT * COLORS_COUNT * 3];
    sizes = new float[WIDGET_STATES_COUNT * SIZES_COUNT];

    //Цветовая палитра по умолчанию
    setColor( WT_ANY, WBS_ANY, WidgetColors::BACKGROUND, glm::vec3( 0.9, 0.9, 0.9 ) );
    setColor( WT_ANY, WBS_ANY, WidgetColors::BORDER,     glm::vec3( 0.8, 0.8, 0.8 ) );
    setColor( WT_ANY, WBS_ANY, WidgetColors::FONT,       glm::vec3( 0.1, 0.1, 0.1 ) );
    setColor( WT_ANY, WBS_ANY, WidgetColors::CLICK_ZONE, glm::vec3( 0.98, 0.98, 0.98 ) );

    //Размер рамки, отступа и шрифта по умолчанию
    setSize( WT_ANY, WBS_ANY, WidgetSizes::BORDER, 1.0 );
    setSize( WT_ANY, WBS_ANY, WidgetSizes::FONT, 14.0f );

    setSize( WT_ANY, WBS_ANY, WidgetSizes::PADDING_L, 5.0f );
    setSize( WT_ANY, WBS_ANY, WidgetSizes::PADDING_R, 5.0f );
    setSize( WT_ANY, WBS_ANY, WidgetSizes::PADDING_T, 5.0f );
    setSize( WT_ANY, WBS_ANY, WidgetSizes::PADDING_B, 8.0f );

    setSize( WT_ANY, WBS_ANY, WidgetSizes::MARGIN_L, 30.0f );
    setSize( WT_ANY, WBS_ANY, WidgetSizes::MARGIN_R, 30.0f );
    setSize( WT_ANY, WBS_ANY, WidgetSizes::MARGIN_T, 15.0f );
    setSize( WT_ANY, WBS_ANY, WidgetSizes::MARGIN_B, 0.0f );

    setSize( WT_ANY, WBS_ANY, WidgetSizes::LINES_DIST, 4.0f );

    //Цвета при наведении по умолчанию
    setColor( WT_ANY, HOV_TRUE, WidgetColors::FONT, glm::vec3( 0.3, 0.3, 0.8 ) );
    setColor( WT_ANY, HOV_TRUE, WidgetColors::BORDER, glm::vec3( 0.3, 0.3, 0.8 ) );

    //WT_BACKGROUND
    setColor( WT_BACKGROUND, WBS_ANY, WidgetColors::BORDER,     glm::vec3( 0.9f, 0.9f, 0.9f ) );
    setColor( WT_BACKGROUND, WBS_ANY, WidgetColors::CLICK_ZONE, glm::vec3( 0.9f, 0.9f, 0.9f ) ); //При клике ничего происходить не должно
    setSize ( WT_BACKGROUND, WBS_ANY, WidgetSizes::BORDER,      0.0f); //У фона не должно быть рамки
    //WT_BUTTON при нажатии
    setColor( WT_BUTTON, PRS_TRUE, WidgetColors::FONT, glm::vec3( 0.4, 0.4, 0.9 ) );
    setColor( WT_BUTTON, PRS_TRUE, WidgetColors::BORDER, glm::vec3( 0.4, 0.4, 0.9 ) );
    //WT_CHECKBOX
    setColor( WT_CHECKBOX, HOV_FALSE | ACT_TRUE, WidgetColors::FONT, glm::vec3( 0.1, 0.1, 0.1 ) );
    setColor( WT_CHECKBOX, WBS_ANY, WidgetColors::BORDER, glm::vec3( 0.75, 0.75, 0.75 ) );
    setColor( WT_CHECKBOX, ACT_TRUE, WidgetColors::BORDER, glm::vec3( 0.3, 0.3, 0.8 ) );
    setSize ( WT_CHECKBOX, ACT_TRUE, WidgetSizes::BORDER, 15.0 );
    //WT_LABEL
    setColor( WT_LABEL, WBS_ANY, WidgetColors::FONT, glm::vec3( 0.1, 0.1, 0.1 ) );
    //WT_SLIDER
    setSize ( WT_SLIDER, WBS_ANY, WidgetSizes::BORDER, 5.0f );
    setColor( WT_SLIDER, WBS_ANY, WidgetColors::BORDER, glm::vec3( 0.7, 0.7, 0.7 ) );

};
Style::~Style() {
    delete[] colors;
    delete[] sizes;
}

int Style::getStateColorID( WidgetType type, int state ) {
    return (widgetTypeToInt(type) << WBS_COUNT) + (state & WBS_RIGHT_PART);
}

void Style::setColor( WidgetType type, int state, WidgetColors color_type, glm::vec3 color ) {
    int c_type = static_cast<int>(color_type), //Color type
        w_type = static_cast<int>(type);       //Widget type
    int states_count = 1 << WBS_COUNT;
    
    for(int i = 0; i < WT_COUNT; i++) {
        int cur_type = 1 << i; //Текущий тип элемента
        if( (w_type & cur_type) == 0 ) continue;

        for(int j = 0; j < states_count; j++) {
            //j перебирает все состояния одного типа видджета и если совпадает - записывает цвет.
            //Этот if проверяет совпадение всех параметров сразу, учитывая возможность обоих состояний одновременно.
            if( (state & WBS_RIGHT_PART ^ j) & (state >> WBS_PART_SIZE) ) continue;
            int cur_id = ((i * states_count + j) * COLORS_COUNT + c_type) * 3;
            colors[cur_id + 0] = color.r;
            colors[cur_id + 1] = color.g;
            colors[cur_id + 2] = color.b;
        }
    }
}
void Style::setSize ( WidgetType type, int state, WidgetSizes size_type, float size ) {
    int s_type = static_cast<int>(size_type), //Color type
        w_type = static_cast<int>(type);       //Widget type
    int states_count = WIDGET_STATES_COUNT / WT_COUNT;

    for (int i = 0; i < WT_COUNT; i++) {
        int cur_type = 1 << i; //Текущий тип элемента
        if ((w_type & cur_type) == 0) continue;

        for (int j = 0; j < states_count; j++) {
            //Если совпадают все параметры - присвоить цвет
            if ( (state & WBS_RIGHT_PART ^ j) & (state >> WBS_PART_SIZE) ) continue;
            int cur_id = (widgetTypeToInt(cur_type) * states_count + j) * SIZES_COUNT + s_type;
            sizes[cur_id] = size;
        }
    }
}

glm::vec3 Style::getColor( WidgetType type, int state, WidgetColors color_type ) {
    int cur_id = ( (widgetTypeToInt(type) * (1 << WBS_COUNT) + (state & WBS_CUR_BITS)) * COLORS_COUNT + static_cast<int>(color_type)) * 3;
    return glm::vec3(colors[cur_id], colors[cur_id + 1], colors[cur_id + 2]);
}
float Style::getSize( WidgetType type, int state, WidgetSizes size_type ) {
    int cur_id = (widgetTypeToInt(type) * (1 << WBS_COUNT) + (state & WBS_CUR_BITS)) * SIZES_COUNT + static_cast<int>(size_type);
    return sizes[cur_id];
}

glm::vec3 Style::getColorInterp( int id_start, int id_end, WidgetColors color_type, float interp ) {
    int type = static_cast<int>(color_type);
    return getColor( id_end * COLORS_COUNT + type ) * interp + getColor( id_start * COLORS_COUNT + type ) * (1.0f - interp);
}
float Style::getSizeInterp( int id_start, int id_end, WidgetSizes size_type, float interp ) {
    int type = static_cast<int>(size_type);
    return getSize( id_end * SIZES_COUNT + type ) * interp + getSize( id_start * SIZES_COUNT + type ) * (1.0f - interp);
}

void Style::setAnimParams( int id_start, int id_end, float interp ) {
    anim_start = id_start;
    anim_end = id_end;
    anim_time = interp;
}
glm::vec3 Style::getColor( WidgetColors color_type ) {
    return getColorInterp(anim_start, anim_end, color_type, anim_time);
}
float     Style::getSize( WidgetSizes size_type ) {
    return getSizeInterp( anim_start, anim_end, size_type, anim_time );
}

glm::vec3 Style::getColor(int id) {
    id *= 3;
    return glm::vec3( colors[id], colors[id + 1], colors[id + 2] );
}
float Style::getSize( int id ) {
    return sizes[id];
}

int ugui::widgetTypeToInt( int type ) {
    switch(type) {
    case WT_BUTTON:     return 0;
    case WT_CHECKBOX:   return 1;
    case WT_LABEL:      return 2;
    case WT_BACKGROUND: return 3;
    default:            return 4;
    }
}

//WordList
WordList::WordList() : cur_height(0.0f), words(std::vector<Word*>()), states(std::vector<unsigned char>()) {}

void WordList::add( Word w ) {
    Word *ptr = new Word(w);
    w.setState(WordState::NULL_WORD);
    words.push_back( ptr );
    states.push_back( 0b11 );
}
void WordList::add( Word *w, bool is_owned ) {
    words.push_back( w );
    states.push_back( is_owned ? 0b11 : 0b10 );
}

void WordList::enableWord(int id)    { states[id] = states[id] | 0b10; }
void WordList::disableWord( int id ) { states[id] = states[id] & 0b11111101; }
void WordList::toggleWord( int id )  { states[id] = states[id] ^ 0b10; }

void WordList::render(TextCursor *cursor) {
    float start_cy = cursor->getCursorY();
    for (int i = 0; i < words.size(); i++) {
        if((states[i] & 0b10) == 0) continue;
        cursor->draw(words[i]);
    }
    cur_height = -cursor->getCursorY() + start_cy + cursor->getFontSize();
}

int WordList::length() {
    return static_cast<int>(words.size());
}
Word* WordList::getElem(int id) {
    return words[id];
}

WordList::~WordList() {
    for(int i = 0; i < words.size(); i++) 
        if((states[i] & 0b01) != 0) delete words[i];
}

//Widget
Widget::Widget( Widget *parent, WidgetType type, float xpos, float ypos, float w, float h, Style *el_style) :
    type(type),

    local_x(xpos), local_y(ypos),
    last_rx(xpos), last_ry(ypos),

    local_w(w), local_h(h),
    last_rw(w), last_rh(h),
    last_subs_h(0.0f),

    mouse_hovering(false), mouse_over(false),
    mouse_pressing(false), is_focused(false),
    is_shown(true), is_active( false ),

    scroll_offset(0.0f),
    scroll_speed(0.0f),
    scroll_start(0.0f),

    anim_start( -1.0 ), anim_state_from( Style::getStateColorID(type, 0) ), anim_state_to( anim_state_from ),
    click_x( 0.0f ), click_y( 0.0f ), click_time( -100.0f ),

    label(WordList()),

    sub_widgets( std::vector<Widget *>() ),
    sub_widgets_owned( std::vector<bool>() ),
    parent_widget( parent ),

    subs_position_mode(WidgetPosMode::RELATIVE),
    is_last_in_row(false),

    style(el_style), style_inherited( el_style ? false : true ),

    vao(0), vbo(0), ebo(0),

    lmb(false), rmb(false),
    last_mouse_move(0.0), mouse_x(0.0f), mouse_y(0.0f),

    last_window( nullptr ), mouse_recalc_required( false )
{
    if(style_inherited) {
        style_inherited = true;
        style = parent ? parent->style : nullptr;
    }

    if(!UGUI_SHADER_INITED) {
        UGUI_SHADER_INITED = true;
        US_ID = loadProgram( "gui" );
        US_MAXRECT   = glGetUniformLocation( US_ID, "max_rectangle"    );
        US_BGCOL     = glGetUniformLocation( US_ID, "background_color" );
        US_BDCOL     = glGetUniformLocation( US_ID, "border_color"     );
        US_RECTSIZE  = glGetUniformLocation( US_ID, "rectangle_size"   );
        US_BDSIZE    = glGetUniformLocation( US_ID, "border_size"      );
        US_CLZ_COLOR = glGetUniformLocation( US_ID, "click_zone_color" );
        US_CLZ_POINT = glGetUniformLocation( US_ID, "click_zone_point" );
        US_NOW       = glGetUniformLocation( US_ID, "now"              );
    }
}
Widget::Widget() :
    type( WT_NULL ),
    local_x( 0.0f ), local_y( 0.0f ),
    last_rx( 0.0f ), last_ry( 0.0f ),
    local_w( 0.0f ), local_h( 0.0f ),
    last_rw( 0.0f ), last_rh( 0.0f ),
    last_subs_h(0.0f),

    mouse_hovering( false ), mouse_over( false ),
    mouse_pressing( false ), is_focused( false ),
    is_shown( false ), is_active(false),

    scroll_offset( 0.0f ),
    scroll_speed( 0.0f ),
    scroll_start( 0.0f ),

    anim_start( -1.0 ), anim_state_from( 0 ), anim_state_to( anim_state_from ),
    click_x( 0.0f ), click_y( 0.0f ), click_time( -100.0f ),

    label( WordList() ),

    sub_widgets( std::vector<Widget *>() ),
    sub_widgets_owned( std::vector<bool>() ),
    parent_widget( nullptr ),

    subs_position_mode( WidgetPosMode::RELATIVE ),
    is_last_in_row( false ),

    style( nullptr ), style_inherited( false ),

    vao( 0 ), vbo( 0 ), ebo( 0 ),

    lmb( false ), rmb( false ),
    last_mouse_move( 0.0 ), mouse_x( 0.0f ), mouse_y( 0.0f ),

    last_window(nullptr), mouse_recalc_required(false)
{}

void Widget::inheritStyle( Style *new_style ) {
    style = new_style;
    //Все младшие элементы должны тоже его унаследовать (если у них не собственный)
    for(int i = 0; i < sub_widgets.size(); i++) if(sub_widgets[i]->style_inherited) sub_widgets[i]->inheritStyle(new_style);
}

Widget Widget::createGUI( Style *s, float x, float y, float width, float height ) {
    return Widget(nullptr, WT_BACKGROUND, x, y, width, height, s);
}

Widget& Widget::addButton( float w, float h, Style *s) {
    add(Widget(this, WidgetType::WT_BUTTON, 0.0f, 0.0f, w, h, s));
    return *this;
}
Widget& Widget::addCheckbox( float w, float h, Style *s ) {
    add( Widget( this, WidgetType::WT_CHECKBOX, 0.0f, 0.0f, w, h, s ) );
    return *this;
}
Widget& Widget::addLabel( float w, float h, Style *s ) {
    add( Widget( this, WidgetType::WT_LABEL, 0.0f, 0.0f, w, h, s ) );
    return *this;
}
Widget& Widget::addSlider( float w, float h, Style *s ) {
    add( Widget( this, WidgetType::WT_SLIDER, 0.0f, 0.0f, w, h, s ) );
    return *this;
}

Widget& Widget::add( Widget *child ) {
    if(!child) return *this;
    sub_widgets.push_back(child);
    sub_widgets_owned.push_back(false);
    if(child->parent_widget != this) child->setParent(this);

    return *this;
}
Widget& Widget::add( Widget  child ) {
    Widget *ptr = new Widget();
    *ptr = child;
    sub_widgets.push_back( ptr );
    sub_widgets_owned.push_back( true );
    if (ptr->parent_widget != this) ptr->setParent( this );

    return *this;
}
Widget& Widget::addText(std::wstring text, TextCursor *cursor ) {
    Widget *sub_w = sub_widgets[sub_widgets.size() - 1];
    updateAnim();
    sub_w->style->setAnimParams(anim_state_from, anim_state_to, 1.0f);
    if(cursor) {
        if(label.length() == 0) cursor->setCursor( 0.0f, 0.0f );
        cursor->
            setBorders( sub_w->local_w -
            sub_w->style->getSize( WidgetSizes::BORDER ) -
            sub_w->style->getSize( WidgetSizes::PADDING_R ) -
            sub_w->style->getSize( WidgetSizes::PADDING_L ) )->
            setRowsDist( style->getSize( WidgetSizes::LINES_DIST ) );
    } 
    if(sub_w->type == WT_CHECKBOX && cursor && label.length() == 0) cursor->moveCursorPix( sub_w->style->getSize(WidgetSizes::FONT) * 1.4f, 0.0f );
    
    Word w( text, cursor, cursor ? true : false );
    sub_w->label.add( w );
    w.setState(WordState::NULL_WORD);
    return *this;
}
Widget& Widget::addText( Word *word ) {
    sub_widgets[sub_widgets.size() - 1]->label.add( word );
    return *this;
}

void Widget::setParent(Widget *parent) {
    parent_widget = parent;
    if(style_inherited) style = parent->style;
}


void Widget::initVAO() {
    for(int i = 0; i < sub_widgets.size(); i++) sub_widgets[i]->initVAO();
    if(vao) return;

    //Инициализируем VAO, VBO для последующего использования
    glGenVertexArrays( 1, &vao );
    glGenBuffers( 1, &vbo );

    glBindVertexArray( vao );
    glBindBuffer( GL_ARRAY_BUFFER, vbo );
    glBufferData( GL_ARRAY_BUFFER, sizeof( float ) * 6 * 4, NULL, GL_DYNAMIC_DRAW );
    glEnableVertexAttribArray( 0 );
    glVertexAttribPointer( 0, 4, GL_FLOAT, GL_FALSE, 4 * sizeof( float ), 0 );
    glBindBuffer( GL_ARRAY_BUFFER, 0 );
    glBindVertexArray( 0 );
}

void Widget::render(TextCursor *cursor, float window_width, float window_height, float x_offset, float y_offset, glm::vec4 parent_rect) {
    if(!style) return;

    //Определяем, самый первый ли это виджет в дереве
    bool is_highest_widget = parent_rect.x == 0 && parent_rect.y == 0 && parent_rect.z == 0 && parent_rect.w == 0;

    if(is_highest_widget) 
        parent_rect = glm::vec4(0.0, 0.0, 100000.0, 100000.0);
    

    last_rx = local_x + x_offset;
    last_ry = local_y + y_offset;

    updateStyleParams();
    double now = get_current_time();
    glm::vec3 bg_color = style->getColor( WidgetColors::BACKGROUND ),
              bd_color = style->getColor( WidgetColors::BORDER     ),
              ft_color = style->getColor( WidgetColors::FONT       ),
              cz_color = style->getColor( WidgetColors::CLICK_ZONE );
    float bdsize   = style->getSize( WidgetSizes::BORDER     ),
          fontsize = style->getSize( WidgetSizes::FONT       ),
          rowsdist = style->getSize( WidgetSizes::LINES_DIST ),

          padding_r = style->getSize( WidgetSizes::PADDING_R ),
          padding_l = style->getSize( WidgetSizes::PADDING_L ),
          padding_t = style->getSize( WidgetSizes::PADDING_T ),
          padding_b = style->getSize( WidgetSizes::PADDING_B );
    
    //Полигон элемента
    float xmin, ymin, xmax, ymax;
    if(type == WT_CHECKBOX) {
        float cb_y = window_height - (last_ry + padding_t + fontsize), //checkbox y
              cb_x = last_rx + padding_l;
        xmin = cb_x   * 2.0f / window_width - 1.0f;
        ymin = (cb_y + fontsize) * 2.0f / window_height - 1.0f;
        xmax = (cb_x + fontsize) * 2.0f / window_width - 1.0f;
        ymax = (cb_y) * 2.0f / window_height - 1.0f;
    } else if(type == WT_SLIDER) {
        xmin = (last_rx + padding_l) * 2.0f / window_width - 1.0f;
        ymin = (window_height - (last_ry + padding_t )) * 2.0f / window_height - 1.0f;
        xmax = (last_rx + last_rw - padding_r) * 2.0f / window_width - 1.0f;
        ymax = (window_height - (last_ry + padding_t + fontsize )) * 2.0f / window_height - 1.0f;
    } else {
        xmin = last_rx * 2.0f / window_width - 1.0f;
        ymin = (window_height - last_ry) * 2.0f / window_height - 1.0f;
        xmax = (last_rx + last_rw) * 2.0f / window_width - 1.0f;
        ymax = (window_height - (last_ry + last_rh)) * 2.0f / window_height - 1.0f;
    }
    //std::cout << "Type: x" << type << " xmin: " << xmin << " xmax: " << xmax << " ymin: " << ymin << " ymax: " << ymax << std::endl;

    float vertices[6][4] = {
        { xmin, ymax, 0.0f, 0.0f },
        { xmin, ymin, 0.0f, 1.0f },
        { xmax, ymin, 1.0f, 1.0f },

        { xmin, ymax, 0.0f, 0.0f },
        { xmax, ymin, 1.0f, 1.0f },
        { xmax, ymax, 1.0f, 0.0f }
    };
    if(type == WT_SLIDER) {
    
    } 
    if(type != WT_LABEL) {
        //Отрисовываем полигон
        glUseProgram( US_ID );
        glBindVertexArray( vao );
        glEnable( GL_BLEND );
        glBlendFunc( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA );
        glUniform3f( US_BGCOL, bg_color.x, bg_color.y, bg_color.z );
        glUniform3f( US_BDCOL, bd_color.x, bd_color.y, bd_color.z );

        if (type == WT_CHECKBOX) glUniform2f( US_RECTSIZE, fontsize, fontsize );
        else if(type == WT_SLIDER) glUniform2f( US_RECTSIZE, last_rw, bdsize );
        else glUniform2f( US_RECTSIZE, last_rw, last_rh );

        glUniform1f( US_BDSIZE, bdsize );
        //Анимация клика
        glUniform3f( US_CLZ_COLOR, cz_color.r, cz_color.g, cz_color.b );
        glUniform3f( US_CLZ_POINT, click_x, window_height - click_y, click_time );
        glUniform1f( US_NOW, static_cast<GLfloat>(now) );


        glUniform4f( US_MAXRECT, parent_rect.x, window_height - parent_rect.y, parent_rect.z, parent_rect.w );

        glBindBuffer( GL_ARRAY_BUFFER, vbo );
        glBufferSubData( GL_ARRAY_BUFFER, 0, sizeof( vertices ), vertices );
       
        glDrawArrays( GL_TRIANGLES, 0, 6 );
        glBindBuffer( GL_ARRAY_BUFFER, 0 );
    }
    
    if(type == WT_CHECKBOX) bdsize = 0.0f;

    cursor->
        setColor( ft_color )->
        setFontSize( fontsize )->
        setRowsDist( rowsdist )->
        setCursor( last_rx + padding_l + bdsize, window_height - (last_ry + bdsize + padding_t + cursor->getFontSize()) );
    float start_cursor_y = cursor->getCursorY();
    float w_border = last_rw - padding_l - padding_r - bdsize * 2.0f + 0.1f; //width border
    glUseProgram( FONT_SHADER.id );
    parent_rect.y = window_height - parent_rect.y;
    FONT_SHADER.setMaxRect( parent_rect );
    if(type == WT_CHECKBOX)
        cursor->
        moveCursorPix(fontsize + 5.0f, 0.0f)->
        setBorders( w_border, last_rh )->
        draw(&label);
    else if(type == WT_LABEL)
        cursor->
        setBorders( w_border, last_rh )->
        draw(&label);
    else if (type == WT_SLIDER)
        cursor->
        moveCursorPix( 0.0f, -fontsize - rowsdist )->
        setBorders( w_border, last_rh )->
        draw( &label );
    else if(type != WT_BACKGROUND)
        cursor->
        setBorders( w_border, last_rh - padding_t - padding_b - bdsize * 2 )->
        draw(&label);
    if(type == WT_SLIDER) start_cursor_y += bdsize + rowsdist;
    if(local_h == 0.0f) {
        last_rh = start_cursor_y - cursor->getCursorY() + fontsize + padding_t + padding_b + bdsize * 2.0f;
        //std::cout << "Lastrh: " << last_rh << " start cy: " << start_cursor_y << " end cy: " << cursor->getCursorY() << std::endl;
    }
    glUseProgram(US_ID);
    
    float offset_y = last_ry + getScroll(), offset_x = last_rx;
    glm::vec4 prect = glm::vec4( last_rx, last_ry, last_rw, last_rh );
    float subs_height = padding_t + padding_b;
    //

    for (int i = 0; i < sub_widgets.size(); i++) {
        Widget *child = sub_widgets[i];
        if(!child->is_shown) continue;
        float ml = child->style->getSize( WidgetSizes::MARGIN_L ),
              mt = child->style->getSize( WidgetSizes::MARGIN_T ),
              mb = child->style->getSize( WidgetSizes::MARGIN_B );
        child->updateStyleParams();
        child->render( cursor, window_width, window_height, 
            offset_x + ml, 
            offset_y + mt,
            prect );
        if(subs_position_mode == WidgetPosMode::RELATIVE) {
            offset_y += child->last_rh + child->local_y + mt + mb;
            subs_height += child->last_rh + child->local_y + mt + mb;
        } else {
            float cur_h = offset_y + child->last_rh + child->local_y + mt + mb;
            subs_height = subs_height > cur_h ? subs_height : cur_h;
        }
    }
    last_subs_h = subs_height;

    if(is_highest_widget) {
        glUseProgram( 0 );
        glBindVertexArray( 0 );
        glBindBuffer( GL_ARRAY_BUFFER, 0 );
        glDisable( GL_BLEND );
    }

    if ( abs( getScrollVel() ) > 0 ) {
        mouseMoved( last_window, mouse_x, mouse_y );
        updateScroll();
    }
}

bool Widget::mouseMoved( GLFWwindow *window, float pos_x, float pos_y, bool highest, bool over_parent) {
    //Сохраняются актуальные данные
    mouse_x = pos_x;
    mouse_y = pos_y;
    last_window = window;
    mouse_recalc_required = false;
    //Находится ли курсор внутри виджета
    bool mouse_in = pos_x >= last_rx && pos_x <= (last_rx + last_rw) && pos_y >= last_ry && pos_y <= (last_ry + last_rh) && over_parent;
    bool mouse_hover = mouse_in;
    //Если мышь пересекается с одним из младших элементов - она не переcекается с этим
    bool result = false;
    for(int i = 0; i < sub_widgets.size(); i++) {
        //Спрашиваем у всей ветки ниже про курсор.
        bool sub_intersected = sub_widgets[i]->mouseMoved( window, pos_x, pos_y, false, mouse_in );
        mouse_hover = mouse_hover && !sub_intersected;
        result = result || sub_intersected;
    };

    bool ret_value; //Обозначает то, какой курсор нужно поставить (Зависит от этого и младших элементов)
    switch (type) {
        case ugui::WT_BACKGROUND: ret_value = result; break;
        case ugui::WT_LABEL:      ret_value = result; break;
        default:                  ret_value = mouse_hover || result; break;
    }
    //Если есть пересечение - поставить курсор пальца
    if(highest) glfwSetCursor( window, glfwCreateStandardCursor( ret_value ? 0x00036004 : 0x00036001 ) );

    //Если мышь вышла за элемент или наоборот зашла - поменять анимации
    if( mouse_in != mouse_over || mouse_hover != mouse_hovering) {
        mouse_over = mouse_in;
        mouse_hovering = mouse_hover;
        updateAnim();
    }

    return ret_value;
}
void Widget::mousePressed ( int button, bool over_parent ) {
    //Оповестить об этом всю ветку
    for (int i = 0; i < sub_widgets.size(); i++) sub_widgets[i]->mousePressed( button );
    //Записать значения
    if(button == 0) {
        lmb = true;
        //Нажать элемент если мышь стоит над элементом
        if (mouse_hovering && over_parent){
            mouse_pressing = true;
            if(type == WT_CHECKBOX) is_active = !is_active;
            click_x = mouse_x;
            click_y = mouse_y;
            click_time = static_cast<float>(get_current_time());
            updateAnim();
        }
    } else if(button == 1) rmb = true;

};
void Widget::mouseReleased( int button ) {
    //Оповестить об этом всю ветку
    for (int i = 0; i < sub_widgets.size(); i++) sub_widgets[i]->mouseReleased( button );
    if (button == 0) {
        lmb = false;
        //Отжать элемент
        mouse_pressing = type == WT_CHECKBOX ? mouse_pressing : false;
        updateAnim();
    } else if (button == 1) rmb = false;
}
void Widget::mouseWheelMoved( float l, GLFWwindow *w ) {
    float now = static_cast<float>(get_current_time()),
          cur_scroll = getScroll(),
          cur_scrvel = getScrollVel();
    scroll_offset = cur_scroll + l;
    scroll_speed = cur_scrvel * 0.375 + l * 15.0f;
    scroll_start = now;
    mouseMoved(w, mouse_x, mouse_y);
    last_window = w;
    mouse_recalc_required = true;
} 

float Widget::getScroll() {
    double dt = get_current_time() - scroll_start;
    double offset = (1.0f - 1.0f / powf( 250.f, dt )) * scroll_speed / log( 8.0f );
    return scroll_offset + offset;
}
float Widget::getScrollVel() {
    double dt = get_current_time() - scroll_start;
    return log(250.0f) * scroll_speed / (3.0f * log(2.0f) * powf(250.0f, dt));
}
void Widget::updateScroll() {
    float cur_scroll = getScroll();
    if(cur_scroll > 0.0f) {
        float ds = cur_scroll * 0.1f;
        scroll_speed -= ds >= 1.0f ? ds : 1.0f;
    } else if(cur_scroll < (last_rh - last_subs_h)) {
        float ds = (last_rh - last_subs_h - cur_scroll) * 0.1f;
        scroll_speed += ds >= 1.0f ? ds : 1.0f;
    }
        
}

void Widget::updateAnim() {
    double cur_time = get_current_time();
    float anim_elapsed = static_cast<float>(cur_time - anim_start);
    anim_start = anim_elapsed >= 0.2f ? cur_time : (cur_time + anim_elapsed - 0.2f);
    anim_state_from = anim_state_to;
    anim_state_to = Style::getStateColorID( type, (mouse_hovering ? HOV_TRUE : HOV_FALSE) | 
                                                  (mouse_pressing ? PRS_TRUE : PRS_FALSE) |
                                                  (is_focused ? FOC_TRUE : FOC_FALSE) |
                                                  (is_active ? ACT_TRUE : ACT_FALSE) );
    style->setAnimParams(anim_state_from, anim_state_to, anim_elapsed > 0.2f ? 1.0f : (anim_elapsed * 5.0f) );
}
void Widget::updateStyleParams() {
    double cur_time = get_current_time();
    float anim_elapsed = static_cast<float>(cur_time - anim_start);
    style->setAnimParams( anim_state_from, anim_state_to, anim_elapsed > 0.2f ? 1.0f : (anim_elapsed * 5.0f) );
}

Widget::~Widget() {
    if(vao) glDeleteBuffers( 1, &vbo );
    for(int i = 0; i < sub_widgets.size(); i++) 
        if(sub_widgets_owned[i]) 
            delete sub_widgets[i];
}

void cout_binary(int n) {
    bool started = false;
    for(int i = 31; i >= 0; i--)
        if((1 << i & n) != 0 || started) {
            started = true;
            std::cout << ((1 << i & n) >> i);
        }
}