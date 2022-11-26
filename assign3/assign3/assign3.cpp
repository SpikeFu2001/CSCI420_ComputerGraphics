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
#define EPSILON 0.000000001
#define MAX_RAY_DEPTH 100
#define REFLECT_OFFSET 0.0001

char *filename = 0;

// different display modes
#define MODE_DISPLAY 1
#define MODE_JPEG 2
int mode = MODE_DISPLAY;

// you may want to make these smaller for debugging purposes
#define WIDTH 1280
#define HEIGHT 960

// the field of view of the camera
#define fov 60.0

unsigned char buffer[HEIGHT][WIDTH][3];
unsigned char screen[HEIGHT][WIDTH][3];
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
Vector3 ray_cast(Ray &ray, size_t depth);
double hit(Ray &ray, Sphere &sphere);
double hit(Ray &ray, Triangle &triangle);
Vector3 get_color_of_ray(Ray &ray, Sphere &sphere, size_t depth);
Vector3 get_color_of_ray(Ray &ray, Triangle &triangle, size_t depth);
Vector3 phong_model(Vector3 &light_color, Vector3 &color_diffuse, Vector3 &l, Vector3 &n, Vector3 &color_specular, Vector3 &r, Vector3 &v, double shininess);
Vector3 ShadowRay(Vector3 &pi, Vector3 &n, Vector3 &color_diffuse, Vector3 &color_specular, Vector3 &v, double shininess);

Vector3 ShadowRay(Vector3 &pi, Vector3 &n, Vector3 &color_diffuse, Vector3 &color_specular, Vector3 &v, double shininess)
{
  Vector3 color{0, 0, 0};
  for (int i = 0; i < num_lights; i++)
  {
    auto &light = lights[i];
    Ray l{pi, glm::normalize(light.position - pi)};
    l.p += REFLECT_OFFSET * l.d;
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
    for (int j = 0; j < num_triangles; j++)
    {
      auto &tri = triangles[j];
      auto ret = hit(l, tri);
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
    auto r = glm::reflect(l.d, n) * -1.0;
    color += phong_model(light.color, color_diffuse, l.d, n, color_specular, r, v, shininess);
  }
  return color;
}

Vector3 phong_model(Vector3 &light_color, Vector3 &color_diffuse, Vector3 &l, Vector3 &n, Vector3 &color_specular, Vector3 &r, Vector3 &v, double shininess)
{
  return light_color * (color_diffuse * glm::clamp(glm::dot(l, n), 0.0, 1.0) + color_specular * glm::pow(glm::clamp(glm::dot(r, v), 0.0, 1.0), shininess));
}

double hit(Ray &ray, Triangle &triangle)
{
  auto &A = triangle.v[0].position;
  auto &B = triangle.v[1].position;
  auto &C = triangle.v[2].position;
  auto &NA = triangle.v[0].normal;
  auto &NB = triangle.v[1].normal;
  auto &NC = triangle.v[2].normal;
  auto AB = B - A;
  auto AC = C - A;
  auto n = glm::normalize(glm::cross(AB, AC));
  auto d = glm::dot(n, A);
  auto n_dot_d = glm::dot(n, ray.d);

  if (glm::epsilonEqual(n_dot_d, 0.0, EPSILON))
  {
    return INFINITY;
  }

  auto t = (d - glm::dot(n, ray.p)) / n_dot_d;
  if (t < 0.0)
  {
    return INFINITY;
  }

  auto Q = ray.p + ray.d * t;

  auto QBC = glm::dot(glm::cross(C - B, Q - B), n);
  if (QBC < 0)
  {
    return INFINITY;
  }

  auto AQC = glm::dot(glm::cross(A - C, Q - C), n);
  if (AQC < 0)
  {
    return INFINITY;
  }

  auto ABQ = glm::dot(glm::cross(B - A, Q - A), n);
  if (ABQ < 0)
  {
    return INFINITY;
  }

  return t;
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

Vector3 get_color_of_ray(Ray &ray, Sphere &sphere, size_t depth = 0)
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
  Vector3 r = glm::reflect(v, n) * -1.0;
  auto local = ShadowRay(pi, n, sphere.color_diffuse, sphere.color_specular, v, sphere.shininess);
  if (glm::all(glm::equal(local, Vector3{0, 0, 0})))
  {
    return local;
  }
  Ray reflect_ray = {pi + REFLECT_OFFSET * r, r};
  auto reflect = ray_cast(reflect_ray, depth + 1);
  if (glm::all(glm::equal(reflect, Vector3{0, 0, 0})))
  {
    return local;
  }
  return (Vector3({1.0, 1.0, 1.0}) - sphere.color_specular) * local + sphere.color_specular * reflect;
}

Vector3 get_color_of_ray(Ray &ray, Triangle &triangle, size_t depth = 0)
{
  auto &A = triangle.v[0].position;
  auto &B = triangle.v[1].position;
  auto &C = triangle.v[2].position;
  auto &NA = triangle.v[0].normal;
  auto &NB = triangle.v[1].normal;
  auto &NC = triangle.v[2].normal;
  auto color_diffuse_A = triangle.v[0].color_diffuse;
  auto color_diffuse_B = triangle.v[1].color_diffuse;
  auto color_diffuse_C = triangle.v[2].color_diffuse;
  auto color_specular_A = triangle.v[0].color_specular;
  auto color_specular_B = triangle.v[1].color_specular;
  auto color_specular_C = triangle.v[2].color_specular;

  auto AB = B - A;
  auto AC = C - A;
  auto n = glm::normalize(glm::cross(AB, AC));
  auto d = glm::dot(n, A);
  auto n_dot_d = glm::dot(n, ray.d);
  Vector3 color{0, 0, 0};

  if (glm::epsilonEqual(n_dot_d, 0.0, EPSILON))
  {
    return color;
  }

  auto t = (d - glm::dot(n, ray.p)) / n_dot_d;
  if (t < 0.0)
  {
    return color;
  }

  auto Q = ray.p + ray.d * t;

  auto QBC = glm::dot(glm::cross(C - B, Q - B), n);
  if (QBC < 0)
  {
    return color;
  }

  auto AQC = glm::dot(glm::cross(A - C, Q - C), n);
  if (AQC < 0)
  {
    return color;
  }

  auto ABQ = glm::dot(glm::cross(B - A, Q - A), n);
  if (ABQ < 0)
  {
    return color;
  }

  auto ABC = glm::dot(glm::cross(AB, AC), n);

  auto alpha = QBC / ABC;
  auto beta = AQC / ABC;
  auto gamma = ABQ / ABC;

  auto v = ray.d * -1.0;
  auto diffuse = alpha * color_diffuse_A + beta * color_diffuse_B + gamma * color_diffuse_C;
  auto specular = alpha * color_specular_A + beta * color_specular_B + gamma * color_specular_C;
  auto shininess = alpha * triangle.v[0].shininess + beta * triangle.v[1].shininess + gamma * triangle.v[2].shininess;
  Vector3 r = glm::reflect(v, n) * -1.0;
  n = glm::normalize(alpha * NA + beta * NB + gamma * NC);
  auto local = ShadowRay(Q, n, diffuse, specular, v, shininess);
  if (glm::all(glm::equal(local, Vector3{0, 0, 0})))
  {
    return local;
  }
  Ray reflect_ray = {Q + REFLECT_OFFSET * r, r};
  auto reflect = ray_cast(reflect_ray, depth + 1);
  if (glm::all(glm::equal(reflect, Vector3{0, 0, 0})))
  {
    return local;
  }
  return (Vector3({1, 1, 1}) - specular) * local + specular * reflect;
}

void ray_cast(int x, int y)
{
  x -= WIDTH / 2;
  y -= HEIGHT / 2;
  auto dx = wx * (x) / (WIDTH / 2.0);
  auto dy = wy * (y) / (HEIGHT / 2.0);
  Ray ray = {Vector3(0, 0, 0), glm::normalize(Vector3(dx, dy, -1))};
  Vector3 color{ambient_light};
  double min_sphere_t = INFINITY;
  Sphere *closest_sphere = nullptr;
  for (int i = 0; i < num_spheres; i++)
  {
    auto t = hit(ray, spheres[i]);
    if (t < min_sphere_t)
    {
      min_sphere_t = t;
      closest_sphere = &(spheres[i]);
    }
  }
  double min_triangle_t = INFINITY;
  Triangle *closest_triangle = nullptr;
  for (int i = 0; i < num_triangles; i++)
  {
    auto t = hit(ray, triangles[i]);
    if (t < min_triangle_t)
    {
      min_triangle_t = t;
      closest_triangle = &(triangles[i]);
    }
  }
  if (min_triangle_t < min_sphere_t && closest_triangle)
  {
    color += get_color_of_ray(ray, *closest_triangle);
  }
  else if (closest_sphere)
  {
    color += get_color_of_ray(ray, *closest_sphere);
  }
  x += WIDTH / 2;
  y += HEIGHT / 2;
  for (int i = 0; i < 3; i++)
  {
    if (closest_sphere || closest_triangle)
      screen[y][x][i] = glm::clamp(color[i], 0.0, 1.0) * 255.0f;
    else
      screen[y][x][i] = 255;
  }
}

Vector3 ray_cast(Ray &ray, size_t depth)
{
  if (depth >= MAX_RAY_DEPTH)
  {
    return {0, 0, 0};
  }
  Vector3 color{ambient_light};
  double min_sphere_t = INFINITY;
  Sphere *closest_sphere = nullptr;
  for (int i = 0; i < num_spheres; i++)
  {
    auto t = hit(ray, spheres[i]);
    if (t < min_sphere_t)
    {
      min_sphere_t = t;
      closest_sphere = &(spheres[i]);
    }
  }
  double min_triangle_t = INFINITY;
  Triangle *closest_triangle = nullptr;
  for (int i = 0; i < num_triangles; i++)
  {
    auto t = hit(ray, triangles[i]);
    if (t < min_triangle_t)
    {
      min_triangle_t = t;
      closest_triangle = &(triangles[i]);
    }
  }
  if (min_triangle_t < min_sphere_t && closest_triangle)
  {
    color += get_color_of_ray(ray, *closest_triangle, depth);
  }
  else if (closest_sphere)
  {
    color += get_color_of_ray(ray, *closest_sphere, depth);
  }
  if (closest_sphere || closest_triangle)
    return glm::clamp(color, 0.0, 1.0);
  else
    return {1.0, 1.0, 1.0};
  return color;
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
      plot_pixel(x, y, screen[y][x][0], screen[y][x][1], screen[y][x][2]);
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
