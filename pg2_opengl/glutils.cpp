#include "pch.h"
#include "glutils.h"


void SetMatrix4x4( const GLuint program, const GLfloat * data, const char * matrix_name )
{	
	const GLint location = glGetUniformLocation( program, matrix_name );

	if ( location == -1 )
	{
		printf( "Matrix '%s' not found in active shader.\n", matrix_name );
	}
	else
	{
		glUniformMatrix4fv( location, 1, GL_TRUE, data );
	}
}
void SetVector3(const GLuint program, const GLfloat* data, const char* vector_name)
{
	const GLint location = glGetUniformLocation(program, vector_name);

	if (location == -1)
	{
			printf("Vector '%s' not found in active shader.\n", vector_name);
	}
	else
	{
		glUniform3fv(location, 1, data);
	}
}
void SetInt(const GLuint program, const GLint data, const char* name)
{
	const GLint location = glGetUniformLocation(program, name);

	if (location == -1)
	{
		printf("Int '%s' not found in active shader.\n", name);
	}
	else
	{
		glUniform1i(location, data);
	}
}
void SetSampler(const GLuint program, GLenum texture_unit, const char* sampler_name)
{
	const GLint location = glGetUniformLocation(program, sampler_name);
	if (location == -1)
	{
		printf("Texture sampler '%s' not found in active shader.\n", sampler_name);
	}
	else
	{
		glUniform1i(location, texture_unit);
	}
}
void SetHandle(const GLuint program,GLuint64 texture_handle, const char* sampler_name)
{
	const GLint location = glGetUniformLocation(program, sampler_name);
	if (location == -1)
	{
			printf("Texture handle '%s' not found in active shader.\n", sampler_name);
	}
	else
	{
		GLuint msb = (GLuint)(texture_handle >> 32);
		GLuint lsb = (GLuint)texture_handle;
		glUniform2ui(location, lsb, msb);		
	}
}
