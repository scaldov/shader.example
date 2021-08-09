#include <unistd.h>
#include <iostream>
#define GLEW_STATIC
#include <GL/glew.h>
#include <GL/glut.h>
// GLFW
#include <GLFW/glfw3.h>

#include <math.h>
// Other includes
#include "shader.h"
#include <vector>
#include <glm/glm.hpp>
#include <glm/gtx/transform.hpp>
#include <glm/vec2.hpp>
#include <glm/vec4.hpp>
#include <glm/mat4x4.hpp>

#include <murmur3.hh>

// Function prototypes
void key_callback(GLFWwindow* window, int key, int scancode, int action, int mode);

// Window dimensions
const GLuint WIDTH = 800, HEIGHT = 600;

float quad_va[][3] = {{1, -1, 0}, {1, 1, 0}, {-1, 1, 0}, {-1, -1, 0}};
float quad_ca[][3] = {{1, 0, 0}, {0, 1, 0}, {0, 0, 1}, {1, 1, 1}};
int quad_ia[] = {0, 1, 2, 2, 3, 0};
Shader *shader;

float g_time = 0.0;
double g_phi = 0.0;
double g_theta = 0.0;

class Polygon{
public:
    std::vector<glm::vec3> vertices;
    std::vector<glm::vec3> colors;
    std::vector<std::vector<int>> edges;
    std::vector<std::vector<int>> faces;
    std::vector<int> indexArray;
    // non-indexed arrays
    std::vector<std::vector<glm::vec3>> colorArray;
    std::vector<std::vector<glm::vec3>> vertexArray;
    std::vector<std::vector<glm::vec3>> normalArray;
};

class Dodecahedron: public Polygon{
public:
    Dodecahedron() {
        vertices.resize(20);
        colors.resize(20);
        float phi = (1.0 + sqrt(5.0)) / 2.0;
        float a = 0.5, b = 0.5 / phi, c = 0.5 * (2.0 - phi);
        vertices[0] = { c,  0,  a};
        vertices[1] = {-c,  0,  a};
        vertices[2] = {-b,  b,  b};
        vertices[3] = { 0,  a,  c};
        vertices[4] = { b,  b,  b};
        vertices[5] = { b, -b,  b};
        vertices[6] = { 0, -a,  c};
        vertices[7] = {-b, -b,  b};
        vertices[8] = { c,  0, -a};
        vertices[9] = {-c,  0, -a};
        vertices[10] = {-b, -b, -b};
        vertices[11] = { 0, -a, -c};
        vertices[12] = { b, -b, -b};
        vertices[13] = { b,  b, -b};
        vertices[14] = { 0,  a, -c};
        vertices[15] = {-b,  b, -b};
        vertices[16] = { a,  c,  0};
        vertices[17] = {-a,  c,  0};
        vertices[18] = {-a, -c,  0};
        vertices[19] = { a, -c,  0};
        printf("vertices.size = %d\n", vertices.size());
        edges = {
            {  0,  1 }, {  0,  4 }, {  0,  5 }, {  1,  2 }, {  1,  7 },
            {  2,  3 }, {  2, 17 }, {  3,  4 }, {  3, 14 }, {  4, 16 },
            {  5,  6 }, {  5, 19 }, {  6,  7 }, {  6, 11 }, {  7, 18 },
            {  8,  9 }, {  8, 12 }, {  8, 13 }, {  9, 10 }, {  9, 15 },
            { 10, 11 }, { 10, 18 }, { 11, 12 }, { 12, 19 }, { 13, 14 },
            { 13, 16 }, { 14, 15 }, { 15, 17 }, { 16, 19 }, { 17, 18 }
        };
        printf("edges.size = %d\n", edges.size());
        faces = {
            { 4,  3,  2,  1,  0},
            { 7,  6,  5,  0,  1},
            {12, 11, 10,  9,  8},
            {15, 14, 13,  8,  9},
            {14,  3,  4, 16, 13},
            { 3, 14, 15, 17,  2},
            {11,  6,  7, 18, 10},
            { 6, 11, 12, 19,  5},
            { 4,  0,  5, 19, 16},
            {12,  8, 13, 16, 19},
            {15,  9, 10, 18, 17},
            { 7,  1,  2, 17, 18}
        };
        printf("faces.size = %d\n", faces.size());
        for(int i = 0; i < vertices.size(); i ++) {
            colors[i] = glm::vec4((float)rand() / RAND_MAX, (float)rand() / RAND_MAX, (float)rand() / RAND_MAX, 1.0);
        }
        for(int j = 0; j < faces.size(); j ++) {
            std::vector<int> &v = faces[j];
            for(int i = 1; i < v.size() - 1; i ++) {
                indexArray.push_back(v[0]);
                indexArray.push_back(v[i]);
                indexArray.push_back(v[i+1]);
            }
        }
        printf("indexArray triangles = %d\n", indexArray.size() / 3);
        for(auto it: vertices) std::cout << it.x << " " << it.y << " " << it.z << std::endl;
    }
};

Dodecahedron poly;

void draw() {
    static int frame = 0;
    glm::vec2 v = {0., 1.};
    glm::mat4 mvp, mv;
    //compute MPV matrix
    glm::mat4 Projection = glm::perspective(glm::radians(45.0f), 16.0f / 9.0f, 0.1f, 100.0f);
    glm::mat4 View       = glm::lookAt(
                glm::vec3(1.5*sin(g_phi), 1.5*cos(g_phi), 1.5*cos(g_theta)), //cam world coords
                glm::vec3(0,0,0), //cam towards to
                glm::vec3(0,1,0)  //north pole vector
                );
    glm::mat4 Model = glm::mat4(1.0f);
    mv = View * Model;
    mvp = Projection * mv;
    //
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    GLint gll = shader->GetUniformLocation(cx::murmur3_32("mvp", 0), "mvp");
    glUniformMatrix4fv(gll, 1, 0, &mvp[0][0]);
    gll = shader->GetUniformLocation(cx::murmur3_32("mv", 0), "mv");
    glUniformMatrix4fv(gll, 1, 0, &mv[0][0]);
    gll = shader->GetUniformLocation(cx::murmur3_32("time", 0), "time");
    glUniform1f(gll, g_time);
    shader->Use();
    glEnableClientState(GL_VERTEX_ARRAY);
    glEnableClientState(GL_COLOR_ARRAY);
    glVertexPointer(3, GL_FLOAT, 0, &poly.vertices[0]);
    glColorPointer(3, GL_FLOAT, 0, &poly.colors[0]);
    glDrawElements(GL_TRIANGLES, poly.indexArray.size(), GL_UNSIGNED_INT, &poly.indexArray[0]);
    glDisableClientState(GL_VERTEX_ARRAY);
    glDisableClientState(GL_COLOR_ARRAY);
    glutSwapBuffers();
    g_phi += 0.01;
    g_theta += 0.003;
    g_time += 0.1;
    printf("f = %d\n", frame);
    frame += 1;
    usleep(30000);
}

void timer(int value) {
    g_phi += 0.01;
    g_theta += 0.003;
    //    glutTimerFunc(33, timer, 0);
}

void specialkeys(int key, int x, int y) {

}

// The MAIN function, from here we start the application and run the game loop
int main(int argc, char **argv)
{
    glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB);
    glutInitWindowSize(1920, 1080);
    glutInit(&argc, argv);
    glutCreateWindow("shader test");
    glutDisplayFunc(draw);
    glutTimerFunc(33, timer, 0);
    glutIdleFunc(draw);
    glutSpecialFunc(specialkeys);
    glClearColor(0, 0, 0, 1);
    glEnable(GL_DEPTH_TEST);
    glewInit();
    shader = new Shader("shader.vertex", "shader.fragment");
    glutFullScreen();
    glutMainLoop();
}
