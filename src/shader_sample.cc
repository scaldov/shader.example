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
#include <algorithm>
#include <glm/glm.hpp>
#include <glm/gtx/transform.hpp>
#include <glm/gtx/normal.hpp>
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
uint32_t vertexArrayObject[4];
GLuint g_vbo[3];

class Polygon{
public:
    std::vector<glm::vec3> vertices;
    std::vector<glm::vec3> vertexColors;
    std::vector<std::vector<int>> edges;
    std::vector<std::vector<int>> faces;
    std::vector<std::vector<glm::vec3>> faceColors;
    std::vector<int> indexArray;
    // non-indexed arrays
    std::vector<glm::vec3> colorArray;
    std::vector<glm::vec3> vertexArray;
    std::vector<glm::vec3> normalArray;
};

class Dodecahedron: public Polygon{
public:
    Dodecahedron() {
        vertices.resize(20);
        vertexColors.resize(20);
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
            vertexColors[i] = glm::vec3((float)rand() / RAND_MAX, (float)rand() / RAND_MAX, (float)rand() / RAND_MAX);
        }
        faceColors.resize(faces.size());
        for(int j = 0; j < faces.size(); j ++) {
            std::vector<int> &v = faces[j];
            auto &color = faceColors[j];
            color.resize(v.size());
            for(int i = 1; i < v.size() - 1; i ++) {
                auto c = glm::vec3((float)rand() / RAND_MAX, (float)rand() / RAND_MAX, (float)rand() / RAND_MAX);
                c = c * (1.0f / glm::max(glm::max(c.x, c.y), c.z));
                color[i] = c;
            }
        }
        for(int j = 0; j < faces.size(); j ++) {
            std::vector<int> &v = faces[j];
            auto &color = faceColors[j];
            for(int i = 1; i < v.size() - 1; i ++) {
                indexArray.push_back(v[0]);
                indexArray.push_back(v[i]);
                indexArray.push_back(v[i+1]);
//                colorArray.push_back(color[0]);
//                colorArray.push_back(color[i]);
//                colorArray.push_back(color[i+1]);
            }
        }
        for(int i = 0; i < indexArray.size(); i += 3) {
            glm::vec3 v0 = vertices[indexArray[i + 0]];
            glm::vec3 v1 = vertices[indexArray[i + 1]];
            glm::vec3 v2 = vertices[indexArray[i + 2]];
            glm::vec3 c0 = vertexColors[indexArray[i + 0]];
            glm::vec3 c1 = vertexColors[indexArray[i + 1]];
            glm::vec3 c2 = vertexColors[indexArray[i + 2]];
            glm::vec3 norm = glm::normalize(glm::cross(v2 - v0, v1 - v0));
            if(glm::dot(v0, norm) < 0.0) {
                norm = -norm;
                std::swap(v1, v2);
                std::swap(c1, c2);
            }
            std::cout << glm::length(norm) << " "  << norm.x << " " << norm.y << " " << norm.z << std::endl;
            vertexArray.push_back(v0);
            vertexArray.push_back(v1);
            vertexArray.push_back(v2);
            colorArray.push_back(c0);
            colorArray.push_back(c1);
            colorArray.push_back(c2);
            normalArray.push_back(norm);
            normalArray.push_back(norm);
            normalArray.push_back(norm);
        }//
        //debug axes
//        vertexArray.push_back(glm::vec3(0., 0., 0.));
//        vertexArray.push_back(glm::vec3(0.01, 0.01, 0.));
//        vertexArray.push_back(glm::vec3(1., 0., 0.));
//        colorArray.push_back(glm::vec3(1., 0., 0.));
//        colorArray.push_back(glm::vec3(1., 0., 0.));
//        colorArray.push_back(glm::vec3(1., 0., 0.));
//        vertexArray.push_back(glm::vec3(0., 0., 0.));
//        vertexArray.push_back(glm::vec3(0.01, 0.01, 0.));
//        vertexArray.push_back(glm::vec3(0., 1., 0.));
//        colorArray.push_back(glm::vec3(0., 1., 0.));
//        colorArray.push_back(glm::vec3(0., 1., 0.));
//        colorArray.push_back(glm::vec3(0., 1., 0.));
//        vertexArray.push_back(glm::vec3(0., 0., 0.));
//        vertexArray.push_back(glm::vec3(0.01, 0.01, 0.));
//        vertexArray.push_back(glm::vec3(0., 0., 1.));
//        colorArray.push_back(glm::vec3(0., 0., 1.));
//        colorArray.push_back(glm::vec3(0., 0., 1.));
//        colorArray.push_back(glm::vec3(0., 0., 1.));
        printf("indexArray triangles = %d\n", indexArray.size() / 3);
        for(auto it: vertices) std::cout << it.x << " " << it.y << " " << it.z << std::endl;
    }
};

Dodecahedron poly;

void GenPoly();

void draw() {
    static int frame = 0;
    glm::vec2 v = {0., 1.};
    glm::vec3 handpos, eyepos, lightdir;
    glm::mat4 mvp, mv;
    lightdir = {0.0, 0.0, 1.0};
    //compute MPV matrix
    handpos = glm::vec3(1.5*sin(g_phi), 1.5*cos(g_phi), 1.5*cos(g_theta));
    eyepos = glm::vec3(1., 0., 0);
    glm::mat4 projection = glm::perspective(glm::radians(90.0f), 16.0f / 9.0f, .1f, 20.0f);
    glm::mat4 view       = glm::lookAt(
                eyepos, //cam world coords
                glm::vec3(0,0,0), //cam towards to
                glm::vec3(0,0,1)  //north pole vector
                );
    //view = glm::mat4(.5);
    //projection = glm::mat4(1.);
    glm::mat4 model = glm::rotate(glm::mat4(1.0), (float)g_phi, glm::vec3(.0, .0, 1.0));
    model = glm::rotate(model, (float)g_theta, glm::vec3(0.0, 1.0, 0.0));
    //model = glm::mat4(1.0);
    //
    glClearColor(0, 0, 0, 1);
    glEnable(GL_DEPTH_TEST);
//    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    GLint gll = shader->GetUniformLocation(cx::murmur3_32("projection", 0), "projection");
    glUniformMatrix4fv(gll, 1, 0, &projection[0][0]);
    gll = shader->GetUniformLocation(cx::murmur3_32("view", 0), "view");
    glUniformMatrix4fv(gll, 1, 0, &view[0][0]);
    gll = shader->GetUniformLocation(cx::murmur3_32("model", 0), "model");
    glUniformMatrix4fv(gll, 1, 0, &model[0][0]);
    gll = shader->GetUniformLocation(cx::murmur3_32("time", 0), "time");
    glUniform1f(gll, g_time);
    gll = shader->GetUniformLocation(cx::murmur3_32("lightdir", 0), "lightdir");
    glUniform3fv(gll, 1, &lightdir.x);
    gll = shader->GetUniformLocation(cx::murmur3_32("eyepos", 0), "eyepos");
    glUniform3fv(gll, 1, &eyepos.x);
    shader->Use();
    GenPoly();
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

void GenPoly(){
    glGenBuffers(3, g_vbo);
    glBindBuffer(GL_ARRAY_BUFFER, g_vbo[0]);
    glBufferData(GL_ARRAY_BUFFER, poly.vertexArray.size() * sizeof(poly.vertexArray[0]), poly.vertexArray.data(), GL_STATIC_DRAW);
    glBindBuffer(GL_ARRAY_BUFFER, g_vbo[1]);
    glBufferData(GL_ARRAY_BUFFER, poly.colorArray.size() * sizeof(poly.colorArray[0]), poly.colorArray.data(), GL_STATIC_DRAW);
    glBindBuffer(GL_ARRAY_BUFFER, g_vbo[2]);
    glBufferData(GL_ARRAY_BUFFER, poly.normalArray.size() * sizeof(poly.normalArray[0]), poly.normalArray.data(), GL_STATIC_DRAW);
    glEnableVertexAttribArray(0);
    glEnableVertexAttribArray(1);
    glEnableVertexAttribArray(2);
    glBindBuffer(GL_ARRAY_BUFFER, g_vbo[0]);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, NULL);
    glBindBuffer(GL_ARRAY_BUFFER, g_vbo[1]);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 0, NULL);
    glBindBuffer(GL_ARRAY_BUFFER, g_vbo[2]);
    glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, 0, NULL);
//    glEnable(GL_CULL_FACE);
    glDisable(GL_CULL_FACE);
//    glCullFace(GL_FRONT);
//    glCullFace(GL_BACK);
//    glCullFace(GL_FRONT_AND_BACK);
    glDrawArrays(GL_TRIANGLES, 0, poly.vertexArray.size());
    glDisableVertexAttribArray(2);
    glDisableVertexAttribArray(1);
    glDisableVertexAttribArray(0);
}

// The MAIN function, from here we start the application and run the game loop
int main(int argc, char **argv)
{
    glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB | GLUT_DEPTH | GLUT_ALPHA);
    glutInitWindowSize(1920, 1080);
    glutInit(&argc, argv);
    glutCreateWindow("shader test");
    glutDisplayFunc(draw);
    glutTimerFunc(33, timer, 0);
    glutIdleFunc(draw);
    glutSpecialFunc(specialkeys);
    glewExperimental = GL_TRUE;
    glewInit();
    shader = new Shader("data/shader.vertex", "data/shader.fragment");
    //    GenPoly();
    glutFullScreen();
    glutMainLoop();
}
