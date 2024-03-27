#include "GraphDebugger.h"

namespace OpenGL{
    InputHandler::InputHandler(GLFWwindow* handle){
        glfwSetCursorPosCallback(handle, mousePosOffsetCallback);
        glfwSetScrollCallback(handle, mouseScrollOffsetCallback);
        for(int key = 0; key < KEY_SIZE; key++){
            if(_key_handlers[key] == nullptr) continue;
            _key_handlers[key]->perform(_key_states[key]);
        }

        for(int key = 0; key < MOUSE_SIZE; key++){
            if(_mouse_handlers[key] == nullptr) continue;
            _mouse_handlers[key]->perform(_mouse_states[key]);
        }

        for(int key = 0; key < TWO_STATE_SIZE; key++){
            _two_state_handlers[key] = nullptr;
            _two_states[key] = {0, 0};
        }
    }

    void InputHandler::attachKey(int key, std::unique_ptr<BaseKey>&& invoker){
        _key_handlers[key] = std::move(invoker);
    }

    void InputHandler::attachMouse(int key, std::unique_ptr<BaseKey>&& invoker){
        _mouse_handlers[key] = std::move(invoker);
    }

    void InputHandler::attachMousePos(std::unique_ptr<BaseTwoState>&& invoker){
        _two_state_handlers[0] = std::move(invoker);
    }

    void InputHandler::attachMousePosOffset(std::unique_ptr<BaseTwoState>&& invoker){
        _two_state_handlers[1] = std::move(invoker);
    }

    void InputHandler::attachMouseScrollOffset(std::unique_ptr<BaseTwoState>&& invoker){
       _two_state_handlers[2] = std::move(invoker); 
    }

    int InputHandler::getKeyState(int key){
        return _key_states[key];
    }

    int InputHandler::getMouseKeyState(int key){
        return _mouse_states[key];
    }

    std::pair<double, double> InputHandler::getMousePos(){
        return _two_states[0];
    }

    std::pair<double, double> InputHandler::getMouseOffset(){
        return _two_states[1];
    }

    std::pair<double, double> InputHandler::getMouseScroll(){
        return _two_states[2];
    }

    void InputHandler::mousePosOffsetCallback(GLFWwindow* handle, double xoffset, double yoffset){
        _two_states[0].first += xoffset;
        _two_states[0].second += yoffset;
        _two_states[1].first += xoffset;
        _two_states[1].second += yoffset;
    }

    void InputHandler::mouseScrollOffsetCallback(GLFWwindow* handle, double xoffset, double yoffset){
        _two_states[2].first += xoffset;
        _two_states[2].second += yoffset;
    }

    void InputHandler::invoke(){
        for(int key = 0; key < KEY_SIZE; key++){
            if(_key_handlers[key] == nullptr) continue;
            _key_handlers[key]->perform(_key_states[key]);
        }

        for(int key = 0; key < MOUSE_SIZE; key++){
            if(_mouse_handlers[key] == nullptr) continue;
            _mouse_handlers[key]->perform(_mouse_states[key]);
        }

        for(int key = 0; key < TWO_STATE_SIZE; key++){
            if(_two_state_handlers[key] != nullptr) {
                _two_state_handlers[key]->perform(_two_states[key]);
            }
            if(key) _two_states[key] = {0, 0};
        }
    }

    void InputHandler::poll(GLFWwindow* handle){
        for(int key = 0; key < KEY_SIZE; key++){
            _key_states[key] = glfwGetKey(handle, key);
        }

        for(int key = 0; key < MOUSE_SIZE; key++){
            if(_mouse_handlers[key] == nullptr) continue;
            _mouse_handlers[key]->perform(_mouse_states[key]);
        }
    }
};
