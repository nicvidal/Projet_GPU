
#include "atom.h"
#include "shaders.h"
#include "sotl.h"
#include "util.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>

GLuint v_shader, f_shader;
GLuint shader_program;
GLuint psize_location,
  minext_location,
  maxext_location;

#ifndef SHADERS_FILES_DIR
#error "You should define SHADERS_FILES_DIR to shaders files directory"
#endif

static GLuint shaderCompile(GLenum type, const char *filePath)
{
  GLuint shader;
  char *source;
  GLint length, result;

  source = file_get_contents(filePath);
  if (!source) {
    sotl_log(ERROR, "Failed to read contents of the shader '%s'.\n", filePath);
    return 0;
  }

  shader = glCreateShader(type);
  length = strlen(source);
  glShaderSource(shader, 1, (const char **)&source, &length);
  glCompileShader(shader);
  free(source);

  glGetShaderiv(shader, GL_COMPILE_STATUS, &result);
  if(result == GL_FALSE) {
    char *log;

    glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &length);
    log = xmalloc(length);
    glGetShaderInfoLog(shader, length, &result, log);
    sotl_log(ERROR, "shaderCompileFromFile(): Unable to compile %s: %s\n", filePath, log);
    free(log);
    glDeleteShader(shader);
    return 0;
  }

  return shader;
}

void shadersInit(void)
{
  GLint err;

  shader_program = glCreateProgram();

  v_shader = shaderCompile(GL_VERTEX_SHADER, SHADERS_FILES_DIR"/shader.vertex");
  glAttachShader(shader_program, v_shader);

  f_shader = shaderCompile(GL_FRAGMENT_SHADER, SHADERS_FILES_DIR"/shader.fragment");
  glAttachShader(shader_program, f_shader);

  glLinkProgram(shader_program);

  glGetProgramiv(shader_program, GL_LINK_STATUS, &err);
  if(err == GL_FALSE) {
    GLint length;
    char *log;

    glGetProgramiv(shader_program, GL_INFO_LOG_LENGTH, &length);
    log = xmalloc(length);
    glGetProgramInfoLog(shader_program, length, &err, log);
    sotl_log(CRITICAL, "Program linking failed: %s\n", log);
  }

  glUseProgram(shader_program);
  psize_location = glGetUniformLocation(shader_program, "PointSize");
  minext_location = glGetUniformLocation(shader_program, "MinExt");
  maxext_location = glGetUniformLocation(shader_program, "MaxExt");
  glUseProgram(0);

}
