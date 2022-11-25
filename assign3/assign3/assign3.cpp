/*
CSCI 420
Assignment 3 Raytracer

Name: <Your name here>
*/

#include <windows.h>
#include <stdlib.h>
#include <GL/glu.h>
#include <GL/glut.h>
#include <glm/glm/glm.hpp>
#include <glm/glm/gtx/norm.hpp>
#include <algorithm>

#include <stdio.h>
#include <string>

#include "opencv2/core/core.hpp"
#include "opencv2/core/utility.hpp"
#include "opencv2/highgui/highgui.hpp"
#include "opencv2/imgproc/imgproc.hpp"
#include "opencv2/imgcodecs/imgcodecs.hpp"

#define MAX_TRIANGLES 2000
#define MAX_SPHERES 10
#define MAX_LIGHTS 10

char *filename = 0;

// different display modes
#define MODE_DISPLAY 1
#define MODE_JPEG 2
int mode = MODE_DISPLAY;

// you may want to make these smaller for debugging purposes
#define WIDTH 1920
#define HEIGHT 1080

// the field of view of the camera
#define fov 60.0

unsigned char buffer[HEIGHT][WIDTH][3];
using Vector3 = glm::vec<3, double>;

struct Vertex
{
  Vector3 position;
  Vector3 color_diffuse;
  Vector3 color_specular;
  Vector3 normal;
  double shininess;
};

typedef struct _Triangle
{
  struct Vertex v[3];
} Triangle;

typedef struct _Sphere
{
  Vector3 position;
  Vector3 color_diffuse;
  Vector3 color_specular;
  double shininess;
  double radius;
} Sphere;

typedef struct _Light
{
  Vector3 position;
  Vector3 color;
} Light;

struct Ray
{
  Vector3 p;
  Vector3 d;
};

Triangle triangles[MAX_TRIANGLES];
Sphere spheres[MAX_SPHERES];
Light lights[MAX_LIGHTS];
Vector3 ambient_light;

int num_triangles = 0;
int num_spheres = 0;
int num_lights = 0;
double fov_radians = glm::radians(fov);
double a = (double)WIDTH / (double)HEIGHT;
auto wx = a * tan(fov_radians / 2.0);
auto wy = tan(fov_radians / 2.0);

void plot_pixel_display(int x, int y, unsigned char r, unsigned char g, unsigned char b);
void plot_pixel_jpeg(int x, int y, unsigned char r, unsigned char g, unsigned char b);
void plot_pixel(int x, int y, unsigned char r, unsigned char g, unsigned char b);
void ray_cast(int x, int y);
double hit(Ray &ray, Sphere &sphere);
Vector3 get_color_contribution(Ray &ray, Sphere &sphere);
Vector3 phong_model(Vector3 &light_color, Vector3 &color_diffuse, double l_dot_n, Vector3 &color_specular, double r_dot_v, double shininess);

Vector3 phong_model(Vector3 &light_color, Vector3 &color_diffuse, double l_dot_n, Vector3 &color_specular, double r_dot_v, double shininess)
{
  return light_color * (color_diffuse * l_dot_n + color_specular * glm::pow(r_dot_v, shininess));
}

double hit(Ray &ray, Sphere &sphere)
{
  auto a = 1.0;
  auto x0xc = ray.p - sphere.position;
  auto b = 2 * (dot(ray.d, x0xc));
  auto c = glm::length2(x0xc) - sphere.radius * sphere.radius;
  auto d = b * b - 4 * a * c;
  d = glm::sqrt(d);
  if (d < 0)
  {
    return INFINITY;
  }
  auto t0 = (-b + d) / 2;
  auto t1 = (-b - d) / 2;
  if (t0 < 0)
  {
    t0 = INFINITY;
  }
  if (t1 < 0)
  {
    t1 = INFINITY;
  }
  return std::min(t0, t1);
}

Vector3 get_color_contribution(Ray &ray, Sphere &sphere)
{
  auto a = 1.0;
  auto x0xc = ray.p - sphere.position;
  auto b = 2 * (dot(ray.d, x0xc));
  auto c = glm::length2(x0xc) - sphere.radius * sphere.radius;
  auto d = b * b - 4 * a * c;
  d = glm::sqrt(d);
  if (d < 0)
  {
    return {0, 0, 0};
  }
  auto t0 = (-b + d) / 2;
  auto t1 = (-b - d) / 2;
  if (t0 < 0)
  {
    t0 = INFINITY;
  }
  if (t1 < 0)
  {
    t1 = INFINITY;
  }
  auto t = std::min(t0, t1);
  if (t == INFINITY)
  {
    return {0, 0, 0};
  }
  auto pi = ray.p + t * ray.d;
  auto n = 1.0 / sphere.radius * (pi - sphere.position);
  auto v = ray.d * -1.0;
  Vector3 color{0, 0, 0};
  for (int i = 0; i < num_lights; i++)
  {
    auto &light = lights[i];
    Ray l{pi, glm::normalize(light.position - pi)};
    l.p += 0.001 * l.d;
    auto light_distance = glm::length(light.position - pi);
    bool blocked = false;
    for (int j = 0; j < num_spheres; j++)
    {
      auto &s = spheres[j];
      auto ret = hit(l, s);
      if (ret != INFINITY && ret < light_distance)
      {
        blocked = true;
        break;
      }
    }
    if (blocked)
    {
      continue;
    }
    auto r = 2 * dot(l.d, n) * n - l.d;
    color += phong_model(light.color, sphere.color_diffuse, glm::clamp(glm::dot(l.d, n), 0.0, 1.0), sphere.color_specular, glm::clamp(glm::dot(r, v), 0.0, 1.0), sphere.shininess);
  }
  return color;
}

void ray_cast(int x, int y)
{
  x -= WIDTH / 2;
  y -= HEIGHT / 2;
  auto dx = wx * (x) / (WIDTH / 2.0);
  auto dy = wy * (y) / (HEIGHT / 2.0);
  Ray ray = {Vector3(0, 0, 0), glm::normalize(Vector3(dx, dy, -1))};
  Vector3 color{ambient_light};
  double minSphereTime = INFINITY;
  Sphere *closest_sphere = nullptr;
  for (int i = 0; i < num_spheres; i++)
  {
    auto t = hit(ray, spheres[i]);
    if (t < minSphereTime)
    {
      t = minSphereTime;
      closest_sphere = &(spheres[i]);
    }
  }
  if (closest_sphere)
  {
    color += get_color_contribution(ray, *closest_sphere);
  }
  x += WIDTH / 2;
  y += HEIGHT / 2;
  for (int i = 0; i < 3; i++)
  {
    buffer[y][x][i] = closest_sphere ? glm::clamp(color[i], 0.0, 1.0) * 255.0f : 255;
  }
}

// MODIFY THIS FUNCTION
void draw_scene()
{
  unsigned int x, y;
  for (x = 0; x < WIDTH; x++)
  {
    for (y = 0; y < HEIGHT; y++)
    {
      ray_cast(x, y);
    }
  }
  for (x = 0; x < WIDTH; x++)
  {
    glPointSize(2.0);
    glBegin(GL_POINTS);
    for (y = 0; y < HEIGHT; y++)
    {
      plot_pixel(x, y, buffer[y][x][0], buffer[y][x][1], buffer[y][x][2]);
    }
    glEnd();
    glFlush();
  }
  printf("Done!\n");
  fflush(stdout);
}

void plot_pixel_display(int x, int y, unsigned char r, unsigned char g, unsigned char b)
{
  glColor3f(((double)r) / 256.f, ((double)g) / 256.f, ((double)b) / 256.f);
  glVertex2i(x, y);
}

void plot_pixel_jpeg(int x, int y, unsigned char r, unsigned char g, unsigned char b)
{
  buffer[HEIGHT - y - 1][x][0] = r;
  buffer[HEIGHT - y - 1][x][1] = g;
  buffer[HEIGHT - y - 1][x][2] = b;
}

void plot_pixel(int x, int y, unsigned char r, unsigned char g, unsigned char b)
{
  plot_pixel_display(x, y, r, g, b);
  if (mode == MODE_JPEG)
    plot_pixel_jpeg(x, y, r, g, b);
}

/* Write a jpg image from buffer*/
void save_jpg()
{
  if (filename == NULL)
    return;

  // Allocate a picture buffer //
  cv::Mat3b bufferBGR = cv::Mat::zeros(HEIGHT, WIDTH, CV_8UC3); // rows, cols, 3-channel 8-bit.
  printf("File to save to: %s\n", filename);

  // unsigned char buffer[HEIGHT][WIDTH][3];
  for (int r = 0; r < HEIGHT; r++)
  {
    for (int c = 0; c < WIDTH; c++)
    {
      for (int chan = 0; chan < 3; chan++)
      {
        unsigned char red = buffer[r][c][0];
        unsigned char green = buffer[r][c][1];
        unsigned char blue = buffer[r][c][2];
        bufferBGR.at<cv::Vec3b>(r, c) = cv::Vec3b(blue, green, red);
      }
    }
  }
  if (cv::imwrite(filename, bufferBGR))
  {
    printf("File saved Successfully\n");
  }
  else
  {
    printf("Error in Saving\n");
  }
}

void parse_check(char *expected, char *found)
{
  if (stricmp(expected, found))
  {
    char error[100];
    printf("Expected '%s ' found '%s '\n", expected, found);
    printf("Parse error, abnormal abortion\n");
    exit(0);
  }
}

void parse_doubles(FILE *file, char *check, Vector3 &point)
{
  double p[3];
  char str[100];
  fscanf(file, "%s", str);
  parse_check(check, str);
  fscanf(file, "%lf %lf %lf", &p[0], &p[1], &p[2]);
  for (int i = 0; i < 3; i++)
  {
    point[i] = p[i];
  }
  printf("%s %lf %lf %lf\n", check, point[0], point[1], point[2]);
}

void parse_rad(FILE *file, double *r)
{
  char str[100];
  fscanf(file, "%s", str);
  parse_check("rad:", str);
  fscanf(file, "%lf", r);
  printf("rad: %f\n", *r);
}

void parse_shi(FILE *file, double *shi)
{
  char s[100];
  fscanf(file, "%s", s);
  parse_check("shi:", s);
  fscanf(file, "%lf", shi);
  printf("shi: %f\n", *shi);
}

int loadScene(char *argv)
{
  FILE *file = fopen(argv, "r");
  int number_of_objects;
  char type[50];
  int i;
  Triangle t;
  Sphere s;
  Light l;
  fscanf(file, "%i", &number_of_objects);

  printf("number of objects: %i\n", number_of_objects);
  char str[200];

  parse_doubles(file, "amb:", ambient_light);

  for (i = 0; i < number_of_objects; i++)
  {
    fscanf(file, "%s\n", type);
    printf("%s\n", type);
    if (stricmp(type, "triangle") == 0)
    {

      printf("found triangle\n");
      int j;

      for (j = 0; j < 3; j++)
      {
        parse_doubles(file, "pos:", t.v[j].position);
        parse_doubles(file, "nor:", t.v[j].normal);
        parse_doubles(file, "dif:", t.v[j].color_diffuse);
        parse_doubles(file, "spe:", t.v[j].color_specular);
        parse_shi(file, &t.v[j].shininess);
      }

      if (num_triangles == MAX_TRIANGLES)
      {
        printf("too many triangles, you should increase MAX_TRIANGLES!\n");
        exit(0);
      }
      triangles[num_triangles++] = t;
    }
    else if (stricmp(type, "sphere") == 0)
    {
      printf("found sphere\n");

      parse_doubles(file, "pos:", s.position);
      parse_rad(file, &s.radius);
      parse_doubles(file, "dif:", s.color_diffuse);
      parse_doubles(file, "spe:", s.color_specular);
      parse_shi(file, &s.shininess);

      if (num_spheres == MAX_SPHERES)
      {
        printf("too many spheres, you should increase MAX_SPHERES!\n");
        exit(0);
      }
      spheres[num_spheres++] = s;
    }
    else if (stricmp(type, "light") == 0)
    {
      printf("found light\n");
      parse_doubles(file, "pos:", l.position);
      parse_doubles(file, "col:", l.color);

      if (num_lights == MAX_LIGHTS)
      {
        printf("too many lights, you should increase MAX_LIGHTS!\n");
        exit(0);
      }
      lights[num_lights++] = l;
    }
    else
    {
      printf("unknown type in scene description:\n%s\n", type);
      exit(0);
    }
  }
  return 0;
}

void display()
{
}

void init()
{
  glMatrixMode(GL_PROJECTION);
  glOrtho(0, WIDTH, 0, HEIGHT, 1, -1);
  glMatrixMode(GL_MODELVIEW);
  glLoadIdentity();

  glClearColor(0, 0, 0, 0);
  glClear(GL_COLOR_BUFFER_BIT);
}

void idle()
{
  // hack to make it only draw once
  static int once = 0;
  if (!once)
  {
    draw_scene();
    if (mode == MODE_JPEG)
      save_jpg();
  }
  once = 1;
}

int main(int argc, char **argv)
{
  if (argc < 2 || argc > 3)
  {
    printf("usage: %s <scenefile> [jpegname]\n", argv[0]);
    exit(0);
  }
  if (argc == 3)
  {
    mode = MODE_JPEG;
    filename = argv[2];
  }
  else if (argc == 2)
    mode = MODE_DISPLAY;

  glutInit(&argc, argv);
  loadScene(argv[1]);

  glutInitDisplayMode(GLUT_RGBA | GLUT_SINGLE);
  glutInitWindowPosition(0, 0);
  glutInitWindowSize(WIDTH, HEIGHT);
  int window = glutCreateWindow("Ray Tracer");
  glutDisplayFunc(display);
  glutIdleFunc(idle);
  init();
  glutMainLoop();
}
