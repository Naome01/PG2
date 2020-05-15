#pragma once
#include "surface.h"
#include "camera.h"
#include "pch.h"
#include "utils.h"
#include "objloader.h"
#include "matrix4x4.h"
#include "glutils.h"
#include "mymath.h"
#include "vertex.h"
#include "texture.h"

class Rasterizer
{
public:
	Rasterizer(const int width, const int height, const float fov_y, const Vector3 view_from, const Vector3 view_at, float near_plane, float far_plane);
	~Rasterizer();

	int InitDeviceAndScene(const char* filename);
	void handle_mouse();
	int InitDevice();
	int InitScenePBR(const char* filename);
	int FinishSetup();
	void LoadEnvTextures(std::vector<const char*> files);
	void LoadBrdfIntTexture(const char* file);
	int InitShaderProgram();
	void InitFrameBuffers();
	int MainLoop( Vector3 lightPos, bool rotate);
	int ReleaseDeviceAndScene();
	void Resize(int width, int height);
	void LoadIrradianceTexture(const char* file);
	void BufferTexturesToShader();

	bool check_gl(const GLenum error = glGetError());

	//void GLAPIENTRY gl_callback(GLenum source, GLenum type, GLuint id, GLenum severity, GLsizei length, const GLchar * message, const void * user_param);
	//void framebuffer_resize_callback(GLFWwindow * window, int width, int height);
	//char * LoadShader(const char * file_name);
	//GLint CheckShader(const GLenum shader);

	void LoadScene(const std::string file_name);
	int Ui();

	GLuint vao = 0;
	GLuint vbo = 0;
	GLuint shadow_vao = 0;
	GLuint shadow_vbo = 0;
	GLuint fbo = 0;
	GLuint rbo_color = 0;
	GLuint rbo_depth = 0;
	int no_triangles = 0;
	bool pressedMouse = false;
	GLuint vertex_shader;
	GLuint fragment_shader;
	GLuint shader_program;

	GLuint shadow_vertex_shader;
	GLuint shadow_fragment_shader;
	GLuint shadow_program;

	GLFWwindow* window;

	GLuint tex_brdf_map_{ 0 };
	GLuint tex_env_map_{ 0 };
	GLuint tex_ir_map_{ 0 };

	GLuint64 handle_brdf_map_{ 0 };
	GLuint64 handle_env_map_{ 0 };
	GLuint64 handle_ir_map_{ 0 };

private:
	std::vector<Surface*> surfaces_;
	std::vector<Material*> materials_;

	//RTcontext context = { 0 };
	//RTbuffer outputBuffer = { 0 };
	//RTvariable focal_length;
	//RTvariable view_from;
	//RTvariable M_c_w;

	Camera camera;
	float fov;

	bool unify_normals_{ true };

};