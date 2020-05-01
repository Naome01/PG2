#include "pch.h"
#include "rasterizer.h"


void CreateBindlessTexture(GLuint& texture, GLuint64& handle, const int width, const int height, unsigned char* data)
{
	glGenTextures(1, &texture);
	glBindTexture(GL_TEXTURE_2D, texture); // bind empty texture object to the target
	// set the texture wrapping/filtering options
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	// copy data from the host buffer
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_BGR, GL_UNSIGNED_BYTE, data);
	glGenerateMipmap(GL_TEXTURE_2D);
	//glBindTexture(GL_TEXTURE_2D, 0); // unbind the newly created texture from the target
	handle = glGetTextureHandleARB(texture); // produces a handle representing the texture in a shader function
	glMakeTextureHandleResidentARB(handle);
}
Rasterizer::Rasterizer(const int width, const int height, const float fov_y, const Vector3 view_from, const Vector3 view_at, float near_plane, float far_plane)
{
	camera = Camera(width, height, fov_y, view_from, view_at, near_plane, far_plane);
	fov = fov_y;
}

Rasterizer::~Rasterizer()
{
	ReleaseDeviceAndScene();
}

bool Rasterizer::check_gl(const GLenum error)
{
	if (error != GL_NO_ERROR)
	{
		//const GLubyte * error_str;
		//error_str = gluErrorString( error );
		//printf( "OpenGL error: %s\n", error_str );

		return false;
	}

	return true;
}

/* glfw callback */
void glfw_callback(const int error, const char* description)
{
	printf("GLFW Error (%d): %s\n", error, description);
}

/* OpenGL messaging callback */
void GLAPIENTRY gl_callback(GLenum source, GLenum type, GLuint id, GLenum severity, GLsizei length, const GLchar* message, const void* user_param)
{
	printf("GL %s type = 0x%x, severity = 0x%x, message = %s\n",
		(type == GL_DEBUG_TYPE_ERROR ? "Error" : "Message"),
		type, severity, message);
}

/* invoked when window is resized */
void framebuffer_resize_callback(GLFWwindow* window, int width, int height)
{
	glViewport(0, 0, width, height);

	Rasterizer* rasterizer = (Rasterizer*)glfwGetWindowUserPointer(window);
	rasterizer->Resize(width, height);
}

void Rasterizer::Resize(int width, int height)
{
	camera.Update(width, height); // we need to update camera parameters as well
	// delete custom FBO with old width and height dimensions
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	glDeleteRenderbuffers(1, &rbo_color);
	glDeleteRenderbuffers(1, &rbo_depth);
	glDeleteFramebuffers(1, &fbo);
	InitFrameBuffers();
}

/* load shader code from text file */
char* LoadShader(const char* file_name)
{
	FILE* file = fopen(file_name, "rt");

	if (file == NULL)
	{
		printf("IO error: File '%s' not found.\n", file_name);

		return NULL;
	}

	size_t file_size = static_cast<size_t>(GetFileSize64(file_name));
	char* shader = NULL;

	if (file_size < 1)
	{
		printf("Shader error: File '%s' is empty.\n", file_name);
	}
	else
	{
		/* v glShaderSource nezadáváme v posledním parametru délku,
		takže øetìzec musí být null terminated, proto +1 a reset na 0*/
		shader = new char[file_size + 1];
		memset(shader, 0, sizeof(*shader) * (file_size + 1));

		size_t bytes = 0; // poèet již naètených bytù

		do
		{
			bytes += fread(shader, sizeof(char), file_size, file);
		} while (!feof(file) && (bytes < file_size));

		if (!feof(file) && (bytes != file_size))
		{
			printf("IO error: Unexpected end of file '%s' encountered.\n", file_name);
		}
	}

	fclose(file);
	file = NULL;

	return shader;
}

/* check shader for completeness */
GLint CheckShader(const GLenum shader)
{
	GLint status = 0;
	glGetShaderiv(shader, GL_COMPILE_STATUS, &status);

	printf("Shader compilation %s.\n", (status == GL_TRUE) ? "was successful" : "FAILED");

	if (status == GL_FALSE)
	{
		int info_length = 0;
		glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &info_length);
		char* info_log = new char[info_length];
		memset(info_log, 0, sizeof(*info_log) * info_length);
		glGetShaderInfoLog(shader, info_length, &info_length, info_log);

		printf("Error log: %s\n", info_log);

		SAFE_DELETE_ARRAY(info_log);
	}

	return status;
}

void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods)
{
	if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS)
		glfwSetWindowShouldClose(window, GLFW_TRUE);
	
}

int Rasterizer::InitDeviceAndScene(const char* filename)
{
	glfwSetErrorCallback(glfw_callback);

	if (!glfwInit())
	{
		return(EXIT_FAILURE);
	}

	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 6);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
	glfwWindowHint(GLFW_SAMPLES, 8);
	glfwWindowHint(GLFW_RESIZABLE, GL_TRUE);
	glfwWindowHint(GLFW_DOUBLEBUFFER, GL_TRUE);
	//glfwWindowHint(GLFW_SRGB_CAPABLE, GLFW_TRUE);

	window = glfwCreateWindow(camera.width_, camera.height_, "PG2 OpenGL", nullptr, nullptr);
	if (!window)
	{
		glfwTerminate();
		return EXIT_FAILURE;
	}


	glfwSetKeyCallback(window, key_callback);

	glfwSetWindowUserPointer(window, this);
	glfwSetFramebufferSizeCallback(window, framebuffer_resize_callback);
	glfwMakeContextCurrent(window);

	if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
	{
		if (!gladLoadGL())
		{
			return EXIT_FAILURE;
		}
	}

	glEnable(GL_DEBUG_OUTPUT);
	glDebugMessageCallback(gl_callback, nullptr);

	printf("OpenGL %s, ", glGetString(GL_VERSION));
	printf("%s", glGetString(GL_RENDERER));
	printf(" (%s)\n", glGetString(GL_VENDOR));
	printf("GLSL %s\n", glGetString(GL_SHADING_LANGUAGE_VERSION));

	check_gl();

	glEnable(GL_MULTISAMPLE);
	glEnable(GL_FRAMEBUFFER_SRGB);
	// map from the range of NDC coordinates <-1.0, 1.0>^2 to <0, width> x <0, height>
	glViewport(0, 0, camera.width_, camera.height_);


	const int no_surfaces = LoadOBJ(filename, surfaces_, materials_);
	this->no_triangles = 0;

	for (auto surface : surfaces_)
	{
		this->no_triangles += surface->no_triangles();
	}
	
	std::vector<MyVertex> vertices;
	std::vector<int> indices;
	// surfaces loop
	int k = 0, l = 0;
	for (auto surface : surfaces_)
	{

		// triangles loop
		for (int i = 0; i < surface->no_triangles(); ++i, ++l)
		{
			Triangle& triangle = surface->get_triangle(i);

			//// vertices loop
			for (int j = 0; j < 3; ++j, ++k)
			{
				const Vertex& vertex = triangle.vertex(j);
				Material* m = surface->get_material();
				int m_index = m->material_index;

				vertices.push_back(MyVertex(vertex, m_index, m->ambient_, m->specular_));

			}

		} // end of triangles loop

	} // end of surfaces loop
	vertex_shader = glCreateShader(GL_VERTEX_SHADER);
	const char* vertex_shader_source = LoadShader("basic_shader.vert");

	glShaderSource(vertex_shader, 1, &vertex_shader_source, nullptr);
	glCompileShader(vertex_shader);
	SAFE_DELETE_ARRAY(vertex_shader_source);
	CheckShader(vertex_shader);

	fragment_shader = glCreateShader(GL_FRAGMENT_SHADER);
	const char* fragment_shader_source = LoadShader("basic_shader.frag");

	glShaderSource(fragment_shader, 1, &fragment_shader_source, nullptr);
	glCompileShader(fragment_shader);
	SAFE_DELETE_ARRAY(fragment_shader_source);
	CheckShader(fragment_shader);

	shader_program = glCreateProgram();
	glAttachShader(shader_program, vertex_shader);
	glAttachShader(shader_program, fragment_shader);
	glLinkProgram(shader_program);
	// TODO check linking	const int no_vertices = no_triangles * 3; //count of points
	const int size = (vertices.size() * sizeof(MyVertex)); // count of elements in vector * size of one element = size of whole array
	const int vertex_stride = sizeof(MyVertex); // size of one MyVertex
	// optional index array
	vao = 0;
	glGenVertexArrays(1, &vao);
	glBindVertexArray(vao);
	vbo = 0;
	glGenBuffers(1, &vbo); // generate vertex buffer object (one of OpenGL objects) and get the unique ID corresponding to that buffer
	glBindBuffer(GL_ARRAY_BUFFER, vbo); // bind the newly created buffer to the GL_ARRAY_BUFFER target
	glBufferData(GL_ARRAY_BUFFER, (vertices.size() * sizeof(MyVertex)), vertices.data(), GL_STATIC_DRAW); // copies the previously defined vertex data into the buffer's memory

	// vertex position
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, vertex_stride, (void*)0);
	glEnableVertexAttribArray(0);

	// normal
	glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, vertex_stride, (void*)(sizeof(float) * 3));
	glEnableVertexAttribArray(1);

	// color
	glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, vertex_stride, (void*)(sizeof(float) * 6));
	glEnableVertexAttribArray(2);

	// vertex texture coordinates
	glVertexAttribPointer(3, 2, GL_FLOAT, GL_FALSE, vertex_stride, (void*)(sizeof(float) * 9));
	glEnableVertexAttribArray(3);

	//// ambient
	//glVertexAttribPointer(6, 3, GL_FLOAT, GL_FALSE, vertex_stride, (void*)(sizeof(float) * 11));
	//glEnableVertexAttribArray(2);

	//// specular
	//glVertexAttribPointer(7, 3, GL_FLOAT, GL_FALSE, vertex_stride, (void*)(sizeof(float) * 14));
	//glEnableVertexAttribArray(2);

	//material index
	glVertexAttribIPointer(5, 1, GL_INT, vertex_stride, (void*)(sizeof(int) * 17));
	glEnableVertexAttribArray(5);


	GLMaterial* gl_materials = new GLMaterial[materials_.size()];
	int m = 0;
	for (const auto& material : materials_) {
		Texture3u* tex_diffuse = material->texture(Material::kDiffuseMapSlot);
		if (tex_diffuse) {
			GLuint id = 0;
			CreateBindlessTexture(id, gl_materials[m].tex_diffuse_handle, tex_diffuse->width(), tex_diffuse->height(), (GLubyte*)tex_diffuse->data());
			gl_materials[m].diffuse = Color3f({ 1.0f, 1.0f, 1.0f }); // white diffuse color
			gl_materials[m].ambient = material->ambient_;
			gl_materials[m].specular = material->specular_;
		}
		else {
			GLuint id = 0;
			GLubyte data[] = { 255, 255, 255, 255 }; // opaque white
			CreateBindlessTexture(id, gl_materials[m].tex_diffuse_handle, 1, 1, data); // white texture
			gl_materials[m].diffuse = material->diffuse();
			gl_materials[m].ambient = material->ambient_;
			gl_materials[m].specular = material->specular_;
		}
		m++;
	}
	
	GLuint ssbo_materials = 0;
	glGenBuffers(1, &ssbo_materials);
	glBindBuffer(GL_SHADER_STORAGE_BUFFER, ssbo_materials);
	const GLsizeiptr gl_materials_size = sizeof(GLMaterial) * materials_.size();
	glBufferData(GL_SHADER_STORAGE_BUFFER, gl_materials_size, gl_materials, GL_STATIC_DRAW);
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, ssbo_materials);
	glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);


	//GLuint ebo = 0; // optional buffer of indices
	//glGenBuffers( 1, &ebo );
	//glBindBuffer( GL_ELEMENT_ARRAY_BUFFER, ebo );
	//glBufferData( GL_ELEMENT_ARRAY_BUFFER, (indices.size() * sizeof(int)), indices.data(), GL_STATIC_DRAW );

	glPointSize(2.0f);
	glLineWidth(1.0f);
	glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

	glEnable(GL_DEPTH_TEST);
	glDepthFunc(GL_LESS);
	InitFrameBuffers();
	//main loop
	//release device

	return EXIT_SUCCESS;
}
int Rasterizer::InitDeviceAndScenePBR(const char* filename)
{
	glfwSetErrorCallback(glfw_callback);

	if (!glfwInit())
	{
		return(EXIT_FAILURE);
	}

	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 6);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
	glfwWindowHint(GLFW_SAMPLES, 8);
	glfwWindowHint(GLFW_RESIZABLE, GL_TRUE);
	glfwWindowHint(GLFW_DOUBLEBUFFER, GL_TRUE);
	//glfwWindowHint(GLFW_SRGB_CAPABLE, GLFW_TRUE);

	window = glfwCreateWindow(camera.width_, camera.height_, "PG2 OpenGL", nullptr, nullptr);
	if (!window)
	{
		glfwTerminate();
		return EXIT_FAILURE;
	}


	glfwSetKeyCallback(window, key_callback);

	glfwSetWindowUserPointer(window, this);
	glfwSetFramebufferSizeCallback(window, framebuffer_resize_callback);
	glfwMakeContextCurrent(window);

	if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
	{
		if (!gladLoadGL())
		{
			return EXIT_FAILURE;
		}
	}

	glEnable(GL_DEBUG_OUTPUT);
	glDebugMessageCallback(gl_callback, nullptr);

	printf("OpenGL %s, ", glGetString(GL_VERSION));
	printf("%s", glGetString(GL_RENDERER));
	printf(" (%s)\n", glGetString(GL_VENDOR));
	printf("GLSL %s\n", glGetString(GL_SHADING_LANGUAGE_VERSION));

	check_gl();

	glEnable(GL_MULTISAMPLE);
	glEnable(GL_FRAMEBUFFER_SRGB);
	// map from the range of NDC coordinates <-1.0, 1.0>^2 to <0, width> x <0, height>
	glViewport(0, 0, camera.width_, camera.height_);


	const int no_surfaces = LoadOBJ(filename, surfaces_, materials_);
	this->no_triangles = 0;

	for (auto surface : surfaces_)
	{
		this->no_triangles += surface->no_triangles();
	}

	std::vector<MyVertex> vertices;
	std::vector<int> indices;
	// surfaces loop
	int k = 0, l = 0;
	for (auto surface : surfaces_)
	{

		// triangles loop
		for (int i = 0; i < surface->no_triangles(); ++i, ++l)
		{
			Triangle& triangle = surface->get_triangle(i);

			//// vertices loop
			for (int j = 0; j < 3; ++j, ++k)
			{
				const Vertex& vertex = triangle.vertex(j);
				Material* m = surface->get_material();
				int m_index = m->material_index;

				vertices.push_back(MyVertex(vertex, m_index, m->ambient_, m->specular_));

			}

		} // end of triangles loop

	} // end of surfaces loop
	vertex_shader = glCreateShader(GL_VERTEX_SHADER);
	const char* vertex_shader_source = LoadShader("basic_shader_PBR.vert");

	glShaderSource(vertex_shader, 1, &vertex_shader_source, nullptr);
	glCompileShader(vertex_shader);
	SAFE_DELETE_ARRAY(vertex_shader_source);
	CheckShader(vertex_shader);

	fragment_shader = glCreateShader(GL_FRAGMENT_SHADER);
	const char* fragment_shader_source = LoadShader("basic_shader_PBR.frag");

	glShaderSource(fragment_shader, 1, &fragment_shader_source, nullptr);
	glCompileShader(fragment_shader);
	SAFE_DELETE_ARRAY(fragment_shader_source);
	CheckShader(fragment_shader);

	shader_program = glCreateProgram();
	glAttachShader(shader_program, vertex_shader);
	glAttachShader(shader_program, fragment_shader);
	glLinkProgram(shader_program);
	const int size = (vertices.size() * sizeof(MyVertex)); // count of elements in vector * size of one element = size of whole array
	const int vertex_stride = sizeof(MyVertex); // size of one MyVertex
	// optional index array
	vao = 0;
	glGenVertexArrays(1, &vao);
	glBindVertexArray(vao);
	vbo = 0;
	glGenBuffers(1, &vbo); // generate vertex buffer object (one of OpenGL objects) and get the unique ID corresponding to that buffer
	glBindBuffer(GL_ARRAY_BUFFER, vbo); // bind the newly created buffer to the GL_ARRAY_BUFFER target
	glBufferData(GL_ARRAY_BUFFER, (vertices.size() * sizeof(MyVertex)), vertices.data(), GL_STATIC_DRAW); // copies the previously defined vertex data into the buffer's memory

	// vertex position
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, vertex_stride, (void*)0);
	glEnableVertexAttribArray(0);

	// normal
	glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, vertex_stride, (void*)(sizeof(float) * 3));
	glEnableVertexAttribArray(1);

	// color
	glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, vertex_stride, (void*)(sizeof(float) * 6));
	glEnableVertexAttribArray(2);

	// vertex texture coordinates
	glVertexAttribPointer(3, 2, GL_FLOAT, GL_FALSE, vertex_stride, (void*)(sizeof(float) * 9));
	glEnableVertexAttribArray(3);

	//// ambient
	//glVertexAttribPointer(6, 3, GL_FLOAT, GL_FALSE, vertex_stride, (void*)(sizeof(float) * 11));
	//glEnableVertexAttribArray(2);

	//// specular
	//glVertexAttribPointer(7, 3, GL_FLOAT, GL_FALSE, vertex_stride, (void*)(sizeof(float) * 14));
	//glEnableVertexAttribArray(2);

	//material index
	glVertexAttribIPointer(5, 1, GL_INT, vertex_stride, (void*)(sizeof(int) * 17));
	glEnableVertexAttribArray(5);

	/*PBR shader*/
	GLMaterialPBR* gl_materials = new GLMaterialPBR[materials_.size()];
	int m = 0;
	for (const auto& material : materials_) {
		Texture3u* tex_diffuse = material->texture(Material::kDiffuseMapSlot);
		if (tex_diffuse) {
			GLuint id = 0;
			CreateBindlessTexture(id, gl_materials[m].tex_diffuse_handle, tex_diffuse->width(), tex_diffuse->height(), (GLubyte*)tex_diffuse->data());
			gl_materials[m].diffuse = Color3f({ 1.0f, 1.0f, 1.0f }); // white diffuse color
		}
		else {
			GLuint id = 0;
			GLubyte data[] = { 255, 255, 255, 255 }; // opaque white
			CreateBindlessTexture(id, gl_materials[m].tex_diffuse_handle, 1, 1, data); // white texture
			gl_materials[m].diffuse = material->diffuse_;
		}
		Texture3u* tex_norm = material->texture(Material::kNormalMapSlot);
		gl_materials[m].normal = Color3f({ 1.f, 1.f, 1.f });
		if (tex_norm) {
			GLuint id = 0;
			CreateBindlessTexture(id, gl_materials[m].tex_normal_handle, tex_norm->width(), tex_norm->height(), (GLubyte*)tex_norm->data());
		}
		else {
			GLuint id = 0;
			GLubyte data[] = { 255, 255, 255, 255 }; // opaque white
			CreateBindlessTexture(id, gl_materials[m].tex_normal_handle, 1, 1, data); // white texture
		}
		Texture3u* tex_rma = material->texture(Material::kRMAMapSlot);
		gl_materials[m].rma = Color3f({ material->roughness_, material->metallicness, 1.0 });
		if (tex_rma) {
			GLuint id = 0;
			CreateBindlessTexture(id, gl_materials[m].tex_rma_handle, tex_rma->width(), tex_rma->height(), (GLubyte*)tex_rma->data());
		}
		else {
			GLuint id = 0;
			GLubyte data[] = { 255, 255, 255, 255 }; // opaque white
			CreateBindlessTexture(id, gl_materials[m].tex_rma_handle, 1, 1, data); // white texture
		}
		m++;
	}
	GLuint ssbo_materials = 0;
	glGenBuffers(1, &ssbo_materials);
	glBindBuffer(GL_SHADER_STORAGE_BUFFER, ssbo_materials);
	const GLsizeiptr gl_materials_size = sizeof(GLMaterial) * materials_.size();
	glBufferData(GL_SHADER_STORAGE_BUFFER, gl_materials_size, gl_materials, GL_STATIC_DRAW);
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, ssbo_materials);
	glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);


	glPointSize(2.0f);
	glLineWidth(1.0f);
	glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

	glEnable(GL_DEPTH_TEST);
	glDepthFunc(GL_LESS);
	InitFrameBuffers();
	//main loop
	//release device

	return EXIT_SUCCESS;
}

int Rasterizer::InitShaderProgram()
{

	glEnable(GL_DEBUG_OUTPUT);
	glDebugMessageCallback(gl_callback, nullptr);

	glEnable(GL_MULTISAMPLE);

	// map from the range of NDC coordinates <-1.0, 1.0>^2 to <0, width> x <0, height>
	glViewport(0, 0, camera.width_, camera.height_);

	GLfloat vertices[] =
	{
		1.0f, 1.0f, // vertex 1 : p1.x, p1.y top-right
		-1.0f, 1.0f,  // vertex 0 : p0.x, p0.y top-left
		-1.0f, -1.0f,// vertex 2 : p2.x, p2.y bottom-left
		1.0f, -1.0f //vertex 2 : p3.x, p3.y bottom-right
	};
	const int no_vertices = 4;
	const int vertex_stride = sizeof(vertices) / no_vertices;

	shadow_vao = 0;
	glGenVertexArrays(1, &shadow_vao);
	glBindVertexArray(shadow_vao);
	shadow_vbo = 0;
	glGenBuffers(1, &shadow_vbo); // generate vertex buffer object (one of OpenGL objects) and get the unique ID corresponding to that buffer
	glBindBuffer(GL_ARRAY_BUFFER, shadow_vbo); // bind the newly created buffer to the GL_ARRAY_BUFFER target
	glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW); // copies the previously defined vertex data into the buffer's memory
	// vertex position
	glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, vertex_stride, 0);
	glEnableVertexAttribArray(0);



	shadow_vertex_shader = glCreateShader(GL_VERTEX_SHADER);
	const char* shadow_vertex_shader_source = LoadShader("shadow_shader.vert");
	glShaderSource(shadow_vertex_shader, 1, &shadow_vertex_shader_source, nullptr);
	glCompileShader(shadow_vertex_shader);
	SAFE_DELETE_ARRAY(shadow_vertex_shader_source);
	CheckShader(shadow_vertex_shader);

	shadow_fragment_shader = glCreateShader(GL_FRAGMENT_SHADER);
	const char* shadow_fragment_shader_source = LoadShader("shadow_shader.frag");
	glShaderSource(shadow_fragment_shader, 1, &shadow_fragment_shader_source, nullptr);
	glCompileShader(shadow_fragment_shader);
	SAFE_DELETE_ARRAY(shadow_fragment_shader_source);
	CheckShader(shadow_fragment_shader);

	shadow_program = glCreateProgram();
	glAttachShader(shadow_program, shadow_vertex_shader);
	glAttachShader(shadow_program, shadow_fragment_shader);
	glLinkProgram(shadow_program);


	glPointSize(2.0f);
	glLineWidth(1.0f);
	glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

	glEnable(GL_DEPTH_TEST);
	glDepthFunc(GL_LESS);

	return 0;
}

void Rasterizer::InitFrameBuffers()
{
	glGenFramebuffers(1, &fbo);
	glBindFramebuffer(GL_FRAMEBUFFER, fbo);
	// Color renderbuffer.
	glGenRenderbuffers(1, &rbo_color);
	glBindRenderbuffer(GL_RENDERBUFFER, rbo_color);
	glRenderbufferStorage(GL_RENDERBUFFER, GL_RGBA32F, camera.width_, camera.height_);
	// Depth renderbuffer
	glGenRenderbuffers(1, &rbo_depth);
	glBindRenderbuffer(GL_RENDERBUFFER, rbo_depth);
	glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT24, camera.width_, camera.height_);
	glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_RENDERBUFFER, rbo_color);
	glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, rbo_depth);
	if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) return;
	glBindFramebuffer(GL_FRAMEBUFFER, fbo);
}

int Rasterizer::MainLoop(bool rotate = true)
{
	glUseProgram(shader_program);

	float a = deg2rad(45);
	while (!glfwWindowShouldClose(window))
	{

		glBindFramebuffer(GL_FRAMEBUFFER, fbo);
		glDrawBuffer(GL_COLOR_ATTACHMENT0);
		glClearColor(0.2f, 0.3f, 0.3f, 1.0f); // state setting function
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT); // state using function

		glBindVertexArray(vao);
		Matrix4x4 model;
		model.set(0, 0, cosf(a));
		model.set(0, 1, -sinf(a));
		model.set(1, 0, sinf(a));
		model.set(1, 1, cosf(a));
		if(rotate)
		 a += 1e-2f;
		Matrix4x4 mvp = camera.projection() * camera.view() * model;
		SetMatrix4x4(shader_program, mvp.data(), "MVP");
		Matrix4x4 mv = camera.view() * model;
		SetMatrix4x4(shader_program, mv.data(), "MV");


		glDrawArrays(GL_TRIANGLES, 0, no_triangles * 3);

		glBindFramebuffer(GL_READ_FRAMEBUFFER, fbo); // bind custom FBO for reading
		glReadBuffer(GL_COLOR_ATTACHMENT0); // select it‘s first color buffer for reading
		glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0); // bind default FBO (0) for writing
		glDrawBuffer(GL_BACK_LEFT); // select it‘s left back buffer for writing
		glBlitFramebuffer(0, 0, camera.width_, camera.height_, 0, 0, camera.width_, camera.height_, GL_COLOR_BUFFER_BIT, GL_NEAREST);// copy

		//new program
		//InitShaderProgram();

		glfwSwapBuffers(window);
		glfwSwapInterval(1);
		glfwPollEvents();
	}
	return 1;
}

int Rasterizer::ReleaseDeviceAndScene()
{
	glDeleteShader(vertex_shader);
	glDeleteShader(fragment_shader);
	glDeleteProgram(shader_program);

	glDeleteBuffers(1, &vbo);
	glDeleteVertexArrays(1, &vao);

	glfwTerminate();
	return 1;
}


