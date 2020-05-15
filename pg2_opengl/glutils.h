#ifndef GL_UTILS_H_
#define GL_UTILS_H_

void SetMatrix4x4( const GLuint program, const GLfloat * data, const char * matrix_name );
void SetVector3(const GLuint program, const GLfloat* data, const char* vector_name);
void SetInt(const GLuint program, const GLint data, const char* name);
void SetSampler(const GLuint program, GLenum texture_unit, const char* sampler_name);
void SetHandle(const GLuint program, GLuint64 texture_handle, const char* sampler_name);
GLuint64 GetLSB(GLuint64 intValue);
GLuint64 GetMSB(GLuint64 intValue);

#endif
