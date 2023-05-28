#include <ft2build.h>
#include FT_FREETYPE_H
#include <GL/glut.h>
#include <math.h>
#include <algorithm>
#include <iostream>
#include <string>
#include <string.h>
#include <vector>
#include <utility>
#define STRING_SIZE 16

int WIN_HEIGHT;
int WIN_WIDTH;
int fontSize = 24;
float translationX = -1.0f;
float translationY = -1.0f;
std::vector<std::pair<int, int>> sharedInfo;


char *ROUND = new char[STRING_SIZE];
int RECEIVER_COLUMNS = 10;
int SPY_COLUMNS = 5; 
int RECEIVER_SCORE = 1;
int SPY_SCORE = 2;
std::string ROUND_WINNER = "Receiver";

FT_Library ftLibrary; // FreeType library context

FT_Face fontFace; // Font face

// Function to initialize FreeType
void initFreeType()
{
    // Initialize FreeType library
    FT_Init_FreeType(&ftLibrary); // Initialize FreeType library

    // /usr/share/fonts/truetype/ubuntu/UbuntuMono-R.ttf
    FT_New_Face(ftLibrary, "./resources/UbuntuMono-R.ttf", 0, &fontFace);

    // Set the font size (in pixels)
    FT_Set_Pixel_Sizes(fontFace, 0, fontSize);
}

void findClosestNumbers(int n, int &num1, int &num2)
{
    int sqrtN = static_cast<int>(std::sqrt(n));
    num1 = sqrtN;
    num2 = sqrtN;

    // Iterate downwards to find the closest pair
    while (num1 >= 1)
    {
        if (n % num1 == 0)
        {
            num2 = n / num1;
            break;
        }
        num1--;
    }

    // Iterate upwards to find the closest pair
    while (num2 <= n)
    {
        if (n % num2 == 0)
        {
            num1 = n / num2;
            break;
        }
        num2++;
    }

    if(num1 == 1 || num2 == 1){
        findClosestNumbers(n-1, num1, num2);
    }
}

// Function to render text
void renderText(const char *text, float x, float y, int size)
{
    x = (x + 1) * WIN_WIDTH / 2;
    y = (y + 1) * WIN_HEIGHT / 2;
    float sizeF = (float)size * std::min(WIN_WIDTH, WIN_HEIGHT) / 600;
    // fontSize = int(sizeF);
    FT_Set_Pixel_Sizes(fontFace, 0, int(sizeF));

    glMatrixMode(GL_PROJECTION);
    glPushMatrix();
    glLoadIdentity();
    gluOrtho2D(0, WIN_WIDTH, 0, WIN_HEIGHT);

    glMatrixMode(GL_MODELVIEW);
    glPushMatrix();
    glLoadIdentity();

    int textWidth = 0;
    const char *c;
    for (c = text; *c; ++c)
    {
        FT_Load_Char(fontFace, *c, FT_LOAD_RENDER);
        textWidth += (fontFace->glyph->advance.x >> 6);
    }

    float startX = x - (textWidth / 2.0f); // Calculate the starting position to center the text

    glTranslatef(startX, y, 0); // Set the position of the text

    // Enable blending for transparency
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    glPixelStorei(GL_UNPACK_ALIGNMENT, 1); // Set font rendering parameters

    // Iterate over the characters in the text
    for (c = text; *c; ++c)
    {
        // Load the glyph for the current character
        if (FT_Load_Char(fontFace, *c, FT_LOAD_RENDER))
            continue;

        FT_Bitmap *bitmap = &(fontFace->glyph->bitmap); // Access the glyph's bitmap

        for (int row = 0; row < bitmap->rows / 2; ++row)
        {
            unsigned char *topRow = bitmap->buffer + row * bitmap->width;
            unsigned char *bottomRow = bitmap->buffer + (bitmap->rows - row - 1) * bitmap->width;
            for (int col = 0; col < bitmap->width; ++col)
            {
                unsigned char temp = topRow[col];
                topRow[col] = bottomRow[col];
                bottomRow[col] = temp;
            }
        }
        fontFace->glyph->bitmap_top = bitmap->rows - fontFace->glyph->bitmap_top;

        // Render the glyph using OpenGL
        glRasterPos2f(fontFace->glyph->bitmap_left, -fontFace->glyph->bitmap_top);
        glDrawPixels(bitmap->width, bitmap->rows, GL_ALPHA, GL_UNSIGNED_BYTE, bitmap->buffer);

        glTranslatef((fontFace->glyph->advance.x >> 6) + 0 * fontSize, 0, 0); // Move the pen position
    }

    glPopMatrix();
    glMatrixMode(GL_PROJECTION);
    glPopMatrix();
}

// A function that draws a circule given its radius, and center x and y coordinates
void drawCircle(float r, float x, float y)
{

    float i = 0.0f;
    float aspectRatio_x = 1.0f;
    float aspectRatio_y = 1.0f;
    if (WIN_WIDTH < WIN_HEIGHT)
    {
        aspectRatio_y = (float)WIN_WIDTH / (float)WIN_HEIGHT;
    }
    else
    {
        aspectRatio_x = (float)WIN_HEIGHT / (float)WIN_WIDTH;
    }

    glBegin(GL_TRIANGLE_FAN);

    glVertex2f(x, y); // Center
    for (i = 0.0f; i <= 360; i++)
        glVertex2f(r * cos(M_PI * i / 180.0) * aspectRatio_x + x, r * sin(M_PI * i / 180.0) * aspectRatio_y + y);

    glEnd();
}

/* draw a rectangle of color(r,g,b) with point coordinates passed */
void drawRectangle(float x, float y, float xLength, float yLength, bool drawBorder, bool clearColor = false)
{
    if(clearColor){
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

        glColor4f(0.0f, 0.0f, 0.0f, 0.0f); // Set clear color (transparent)
    }
    glBegin(GL_QUADS); // draw a quad

    /*
     *x1,y2           *x2,y2
     *x,y
     *x1,y1           *x2,y1
     */
    float x1 = x - (0.5 * xLength);
    float x2 = x + (0.5 * xLength);
    float y1 = y - (0.5 * yLength);
    float y2 = y + (0.5 * yLength);
    // std::cout<<x1<<"\t"<<x2<<"\t"<<y1<<"\t"<<y2<<"\n";
    glVertex2f(x1, y1); // bottom left corner
    glVertex2f(x2, y1); // bottom right corner
    glVertex2f(x2, y2); // top right corner
    glVertex2f(x1, y2); // top left corner
    glEnd();
    if(clearColor){
        glDisable(GL_BLEND);
    }

    if (drawBorder)
    {
        glBegin(GL_LINE_LOOP);
        glColor3f(0.0f, 0.0f, 0.0f);
        glVertex2f(x1, y1); // bottom left corner
        glVertex2f(x2, y1); // bottom right corner
        glVertex2f(x2, y2); // top right corner
        glVertex2f(x1, y2); // top left corner
        glEnd();
    }
}

GLfloat convertColor(int value)
{
    return static_cast<GLfloat>(value) / 255.0f;
}

void applyColor(int r, int g, int b)
{
    glColor3f(static_cast<GLfloat>(r) / 255.0f, static_cast<GLfloat>(g) / 255.0f, static_cast<GLfloat>(b) / 255.0f);
}

/* display the round # at the top of the window */
void drawRound()
{
    applyColor(210, 210, 210);
    drawRectangle(0.0f, 0.8f, 0.4f, 0.2f, false);
    renderText(ROUND, 0.0f, 0.8f, 24);
}

void drawSharedMemory(int numberOfColumns)
{
    numberOfColumns = 20;
    sharedInfo = {{6,0}, {1,1}, {2, 2}, {4, 3}, {5, 2}, {7, 0}, {10, 0}, {9, 1}, {8, 3}, {3, 2}, {6,0}, {1,1}, {2, 2}, {4, 3}, {5, 2}, {7, 0}, {10, 0}, {9, 1}, {8, 3}, {3, 2}};

    int rows, columns;
    findClosestNumbers(numberOfColumns, rows, columns);
    if(rows > columns){
        std::swap(rows, columns);
    }
    float length = 0.1f;
    float startx, starty; // for the top left block
    if (rows % 2 == 0)
    {
        startx = -1 * (rows / 2) * length + (length / 2.0);
    }
    else
    {
        startx = -1 * (rows / 2) * length;
    }
    if (columns % 2 == 0)
    {
        starty = (columns / 2) * length - (length / 2.0);
    }
    else
    {
        starty = (columns / 2) * length;
    }

    // Draw Shared Memory Label
    renderText("Shared Memory", 0 , starty + length, 18);

    int cnt = 1;
    int index = 0;
    for (int j = 0; j <= columns; j++)
    {
        bool bbreak = false;
        for (int i = 0; i < rows; i++)
        {
            switch (sharedInfo[index].second)
            {
            case 0: // none
                applyColor(210, 210, 210);
                break;
            case 1: // receiver
                applyColor(255, 204, 203);
                break;
            case 2: // spy
                applyColor(0, 255, 255);
                break;
            default: // both
                applyColor(191,128,191);
                break;
            }
            drawRectangle(startx + (i * length), starty - (j * length), length, length, true);
            renderText(std::to_string(sharedInfo[index].first).c_str(), startx + (i * length), starty - (j * length), 16);
            cnt++;
            index++;
            if(cnt > numberOfColumns){
                bbreak = true;
                break;
            }
        }
        if(bbreak){
            break;
        }
    }
}

void drawReceiverAndSpyLabels(){
    applyColor(255, 204, 203);
    drawRectangle(-0.7, 0.1, 0.3, 0.15, false);
    applyColor(0, 0, 0);
    renderText("Receiver", -0.7, 0.1, 20);

    applyColor(0, 255, 255);
    drawRectangle(0.7, 0.1, 0.3, 0.15, false);
    applyColor(0, 0, 0);
    renderText("Spy", 0.7, 0.1, 20);

    // obtained columns for receiver
    renderText("# obtained columns", -0.7, -0.1, 18);
    renderText(std::to_string(RECEIVER_COLUMNS).c_str(), -0.7, -0.2, 18);

    // obtained columns for spy
    renderText("# obtained columns", 0.7, -0.1, 18);
    renderText(std::to_string(SPY_COLUMNS).c_str(), 0.7, -0.2, 18);
}

void drawScores(){
    renderText("Total Score", 0, -0.8, 16);

    glBegin(GL_LINES);              // Start drawing lines
    glVertex2f(-0.6f, -0.8f);        // First endpoint of the line
    glVertex2f(-0.2f, -0.8f);         // Second endpoint of the line
    glEnd();                        // End drawing lines

    glBegin(GL_LINES);              // Start drawing lines
    glVertex2f(0.2f, -0.8f);        // First endpoint of the line
    glVertex2f(0.6f, -0.8f);         // Second endpoint of the line
    glEnd();                        // End drawing lines

    // draw receiver score
    renderText(std::to_string(RECEIVER_SCORE).c_str(), -0.7f, -0.8f, 16);
    drawRectangle(-0.7f, -0.8f, 0.15f, 0.1f, true, true);

    // draw spy score
    renderText(std::to_string(SPY_SCORE).c_str(), 0.7f, -0.8f, 16);
    drawRectangle(0.7f, -0.8f, 0.15f, 0.1f, true, true);
}

void displayRoundWinner(){
    applyColor(210, 210, 210);
    drawRectangle(0.0f, -0.5, 0.4f, 0.2f, false);
    applyColor(0, 0, 0);
    
    renderText("Round Winner", 0.0f, -0.48f, 18);
    renderText(ROUND_WINNER.c_str(), 0.0f, -0.55f, 18);
}

// GLUT display function
void display()
{
    glClearColor(1.0f, 1.0f, 1.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);

    glColor3f(0.0f, 0.0f, 0.0f);

    // // Draw the x-axis
    // glColor3f(1.0, 0.0, 0.0); // Red color
    // glBegin(GL_LINES);
    // glVertex2f(-1.0, 0.0);
    // glVertex2f(1.0, 0.0);
    // glEnd();

    // // Draw the y-axis
    // glColor3f(0.0, 0.0, 1.0); // Blue color
    // glBegin(GL_LINES);
    // glVertex2f(0.0, -1.0);
    // glVertex2f(0.0, 1.0);
    // glEnd();

    // drawCircle(0.05, translationX, translationY);

    // Render the text at position (0, 0)
    // renderText("Hello, World! yeeeg", 0, 0, 24);
    // drawRectangle( 0.0f,0.0f, 0.1f, 0.3f);
    // drawCircle(0.05, translationX, translationY);

    std::string roundStr = "Round #";
    strcpy(ROUND, roundStr.c_str());
    drawRound();
    drawSharedMemory(20);
    drawReceiverAndSpyLabels();
    drawScores();
    displayRoundWinner();
    glutSwapBuffers();
}

// GLUT reshape function
void reshape(int width, int height)
{

    // // make the view port always square
    // if (width < height)
    // {
    //     glViewport(0, (height - width) / 2, width, width);
    //     WIN_HEIGHT = width;
    //     WIN_WIDTH = width;
    // }
    // else
    // {
    //     glViewport((width - height) / 2, 0, height, height);
    //     WIN_HEIGHT = height;
    //     WIN_WIDTH = height;
    // }
    glViewport(0, 0, width, height);
    WIN_HEIGHT = height;
    WIN_WIDTH = width;
    // float size = 24.0 * min(width, height) / 600;
    // fontSize = int(size);
}

void update(int value)
{
    translationX += 0.01f; // Adjust translation speed as needed
    if (translationX > 1.0f)
    {
        return;
    }

    glutPostRedisplay();
    glutTimerFunc(16, update, 0); // 16 milliseconds between updates (approximately 60 FPS)
}

void update2(int value)
{
    translationY += 0.01f; // Adjust translation speed as needed
    if (translationY > 1.0f)
    {
        return;
    }

    glutPostRedisplay();
    glutTimerFunc(64, update2, 0); // 16 milliseconds between updates (approximately 60 FPS)
}

// GLUT main function
int main(int argc, char **argv)
{
    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_RGB | GLUT_DOUBLE);
    glutInitWindowSize(900, 800);
    glutCreateWindow("FreeType Text Rendering");

    initFreeType();

    glutDisplayFunc(display);
    glutReshapeFunc(reshape);
    glutTimerFunc(0, update, 0);
    glutTimerFunc(1000, update2, 0);

    glutMainLoop();

    // Cleanup FreeType resources
    FT_Done_Face(fontFace);
    FT_Done_FreeType(ftLibrary);

    return 0;
}
