#include "GraphDebugger.h"

namespace OpenGL{
    InputHandler::InputHandler(){
        for(int key = 0; key < 128; key++){
            _key_handlers[key] = nullptr;
        }
    }
    
    void InputHandler::attach(int key, std::unique_ptr<Base>&& invoker){
        _key_handlers[key] = std::move(invoker);
    }

    void InputHandler::invoke(){
        for(int key = 0; key < 128; key++){
            if(_key_handlers[key] == nullptr) continue;
            _key_handlers[key]->perform(key);
        }
    }

    void InputHandler::poll(GLFWwindow* handle){
        for(int key = 0; key < 128; key++){
            _key_states[key] = glfwGetKey(handle, key);
        }
    }

};
