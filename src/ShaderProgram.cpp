#include "GraphDebugger.h"

namespace OpenGL{
    int ShaderProgram::getUniformLocation(const std::string& name){
        auto it = _uniform_positions.find(name);
        if(it == _uniform_positions.end()) return _uniform_positions[name] = glGetUniformLocation(_shader_program_id, name.c_str());
        else return it->second;
    }

    ShaderProgram::ShaderProgram() : _is_linked(false), _shader_program_id(glCreateProgram())
    {}

    ShaderProgram::~ShaderProgram(){
        glDeleteProgram(_shader_program_id);    
    }

    void ShaderProgram::addShader(std::istream& is, GLenum shader_type){
        if(_is_linked) {
            std::cerr << "This shader program was already linked\n";
            std::cerr.flush();
            return;
        }
        _shaders.push_back(Shader(is, shader_type));
        glAttachShader(_shader_program_id, _shaders.back().getID());
    }

    void ShaderProgram::addShader(const Shader &shader) {
        if(_is_linked) {
            std::cerr << "This shader program was already linked\n";
            std::cerr.flush();
            return;
        }
        glAttachShader(_shader_program_id, shader.getID());
    }

    void ShaderProgram::addShader(Shader &&sh) {
        if(_is_linked) {
            std::cerr << "This shader program was already linked\n";
            std::cerr.flush();
            return;
        }
        _shaders.push_back(std::move(sh));
        glAttachShader(_shader_program_id, _shaders.back().getID());
    }

    void ShaderProgram::use(){
        if(_is_linked == false){
            glLinkProgram(_shader_program_id);
            int status;
            glGetProgramiv(_shader_program_id, GL_LINK_STATUS, &status);
            if(!status){
                std::cerr << "Error linking shader program. Logs:\n";
                char logs[1024];
                glGetProgramInfoLog(_shader_program_id, 1024, NULL, logs);
                std::cerr << logs << std::endl;
            }
            _shaders.clear();
            _is_linked = true;
        }
        glUseProgram(_shader_program_id);
    }
    
    void ShaderProgram::setUniform1f(const std::string& name, float value){
        glUniform1f(getUniformLocation(name), value);
    }

    void ShaderProgram::setUniform1i(const std::string& name, int value){
        glUniform1i(getUniformLocation(name), value);
    }

    void ShaderProgram::setUniform1ui(const std::string& name, uint32_t value){
        glUniform1ui(getUniformLocation(name), value);
    }

    void ShaderProgram::setUniform1fv(const std::string& name, const std::vector<float>& value){
        glUniform1fv(getUniformLocation(name), 1, value.data());
    }

    void ShaderProgram::setUniform2fv(const std::string& name, const std::vector<float>& value){
        glUniform2fv(getUniformLocation(name), 1, value.data());
    }

    void ShaderProgram::setUniform3fv(const std::string& name, const std::vector<float>& value){
        glUniform3fv(getUniformLocation(name), 1, value.data());
    }

    void ShaderProgram::setUniform4fv(const std::string& name, const std::vector<float>& value){
        glUniform4fv(getUniformLocation(name), 1, value.data());
    }

    void ShaderProgram::setUniform1iv(const std::string& name, const std::vector<int>& value){
        glUniform1iv(getUniformLocation(name), 1, value.data());
    }

    void ShaderProgram::setUniform2iv(const std::string& name, const std::vector<int>& value){
        glUniform2iv(getUniformLocation(name), 1, value.data());
    }

    void ShaderProgram::setUniform3iv(const std::string& name, const std::vector<int>& value){
        glUniform3iv(getUniformLocation(name), 1, value.data());
    }

    void ShaderProgram::setUniform4iv(const std::string& name, const std::vector<int>& value){
        glUniform4iv(getUniformLocation(name), 1, value.data());
    }

    void ShaderProgram::setUniform1uiv(const std::string& name, const std::vector<uint32_t>& value){
        glUniform1uiv(getUniformLocation(name), 1, value.data());
    }

    void ShaderProgram::setUniform2uiv(const std::string& name, const std::vector<uint32_t>& value){
        glUniform2uiv(getUniformLocation(name), 1, value.data());
    }

    void ShaderProgram::setUniform3uiv(const std::string& name, const std::vector<uint32_t>& value){
        glUniform3uiv(getUniformLocation(name), 1, value.data());
    }

    void ShaderProgram::setUniform4uiv(const std::string& name, const std::vector<uint32_t>& value){
        glUniform4uiv(getUniformLocation(name), 1, value.data());
    }

    void ShaderProgram::setUniformMatrix2fv(const std::string& name, bool transpose, const std::vector<float>& value){
        glUniformMatrix2fv(getUniformLocation(name), 1, transpose, value.data());
    }

    void ShaderProgram::setUniformMatrix3fv(const std::string& name, bool transpose, const std::vector<float>& value){
        glUniformMatrix3fv(getUniformLocation(name), 1, transpose, value.data());
    }

    void ShaderProgram::setUniformMatrix4fv(const std::string& name, bool transpose, const std::vector<float>& value){
        glUniformMatrix4fv(getUniformLocation(name), 1, transpose, value.data());
    }
};
