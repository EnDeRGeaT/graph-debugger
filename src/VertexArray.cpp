#include "GraphDebugger.h"

namespace OpenGL{

    void VertexArray::unbind(){
        glBindVertexArray(0);
    }

    void VertexArray::bind(){
        glBindVertexArray(_ID);
    }

    VertexArray::~VertexArray(){
        glDeleteBuffers(1, &_ID);
    }
    
    VertexArray& VertexArray::operator=(VertexArray&& x){
        _ID = x._ID;
        x._ID = 0;
        return *this;
    }

    VertexArray::VertexArray(VertexArray&& x):
    _ID(x._ID){
        *this = std::move(x);
    }

    VertexArray::VertexArray(){
        glGenVertexArrays(1, &_ID);
    }

};
