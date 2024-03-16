#include "GraphDebugger.h"
#include <sstream>

namespace OpenGL{

    Shader::Shader(const std::string &code, GLenum shader_type) {
        const char* c = code.c_str();
        _id = glCreateShader(shader_type);
        glShaderSource(_id, 1, &c, NULL); compile(shader_type);
    }

    Shader::Shader(std::istream &is, GLenum shader_type) {
        std::stringstream ss;
        ss << is.rdbuf();
        std::string s = ss.str();
        const char* code = s.c_str();
        _id = glCreateShader(shader_type);
        glShaderSource(_id, 1, &code, NULL);
        compile(shader_type);
    }
    
    Shader::Shader(const char *code, GLenum shader_type) {
        _id = glCreateShader(shader_type);
        glShaderSource(_id, 1, &code, NULL);
        compile(shader_type);
    }

    void Shader::compile(GLenum shader_type) {
        glCompileShader(_id);
        int status;
        glGetShaderiv(_id, GL_COMPILE_STATUS, &status);
        if(!status){
            std::cerr << "Error compiling shader of " << shader_type << " type. Logs:\n";
            char logs[1024];
            glGetShaderInfoLog(_id, 1024, NULL, logs);
            std::cerr << logs << std::endl;
            glDeleteShader(_id);
            _id = 0;
        }
    }
    

    Shader::Shader(Shader &&x) {
        this->_id = x._id;
        x._id = 0;
    }

    Shader &Shader::operator=(Shader &&x) {
        _id = x._id;
        x._id = 0;
        return *this;
    }

    Shader::~Shader() {
        glDeleteShader(_id);
    }

    uint32_t Shader::getID() const {
        return _id;
    }

};
