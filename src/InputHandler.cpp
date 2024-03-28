#include "GraphDebugger.h"

namespace OpenGL{
    const int GLFW_KEYS[] = {
        GLFW_KEY_SPACE,
        GLFW_KEY_APOSTROPHE,
        GLFW_KEY_COMMA,
        GLFW_KEY_MINUS,
        GLFW_KEY_PERIOD,
        GLFW_KEY_SLASH,
        GLFW_KEY_0,
        GLFW_KEY_1,
        GLFW_KEY_2,
        GLFW_KEY_3,
        GLFW_KEY_4,
        GLFW_KEY_5,
        GLFW_KEY_6,
        GLFW_KEY_7,
        GLFW_KEY_8,
        GLFW_KEY_9,
        GLFW_KEY_SEMICOLON,
        GLFW_KEY_EQUAL,
        GLFW_KEY_A,
        GLFW_KEY_B,
        GLFW_KEY_C,
        GLFW_KEY_D,
        GLFW_KEY_E,
        GLFW_KEY_F,
        GLFW_KEY_G,
        GLFW_KEY_H,
        GLFW_KEY_I,
        GLFW_KEY_J,
        GLFW_KEY_K,
        GLFW_KEY_L,
        GLFW_KEY_M,
        GLFW_KEY_N,
        GLFW_KEY_O,
        GLFW_KEY_P,
        GLFW_KEY_Q,
        GLFW_KEY_R,
        GLFW_KEY_S,
        GLFW_KEY_T,
        GLFW_KEY_U,
        GLFW_KEY_V,
        GLFW_KEY_W,
        GLFW_KEY_X,
        GLFW_KEY_Y,
        GLFW_KEY_Z,
        GLFW_KEY_LEFT_BRACKET,
        GLFW_KEY_BACKSLASH,
        GLFW_KEY_RIGHT_BRACKET,
        GLFW_KEY_GRAVE_ACCENT,
        GLFW_KEY_WORLD_1,
        GLFW_KEY_WORLD_2,
        GLFW_KEY_ESCAPE,
        GLFW_KEY_ENTER,
        GLFW_KEY_TAB,
        GLFW_KEY_BACKSPACE,
        GLFW_KEY_INSERT,
        GLFW_KEY_DELETE,
        GLFW_KEY_RIGHT,
        GLFW_KEY_LEFT,
        GLFW_KEY_DOWN,
        GLFW_KEY_UP,
        GLFW_KEY_PAGE_UP,
        GLFW_KEY_PAGE_DOWN,
        GLFW_KEY_HOME,
        GLFW_KEY_END,
        GLFW_KEY_CAPS_LOCK,
        GLFW_KEY_SCROLL_LOCK,
        GLFW_KEY_NUM_LOCK,
        GLFW_KEY_PRINT_SCREEN,
        GLFW_KEY_PAUSE,
        GLFW_KEY_F1,
        GLFW_KEY_F2,
        GLFW_KEY_F3,
        GLFW_KEY_F4,
        GLFW_KEY_F5,
        GLFW_KEY_F6,
        GLFW_KEY_F7,
        GLFW_KEY_F8,
        GLFW_KEY_F9,
        GLFW_KEY_F10,
        GLFW_KEY_F11,
        GLFW_KEY_F12,
        GLFW_KEY_F13,
        GLFW_KEY_F14,
        GLFW_KEY_F15,
        GLFW_KEY_F16,
        GLFW_KEY_F17,
        GLFW_KEY_F18,
        GLFW_KEY_F19,
        GLFW_KEY_F20,
        GLFW_KEY_F21,
        GLFW_KEY_F22,
        GLFW_KEY_F23,
        GLFW_KEY_F24,
        GLFW_KEY_F25,
        GLFW_KEY_KP_0,
        GLFW_KEY_KP_1,
        GLFW_KEY_KP_2,
        GLFW_KEY_KP_3,
        GLFW_KEY_KP_4,
        GLFW_KEY_KP_5,
        GLFW_KEY_KP_6,
        GLFW_KEY_KP_7,
        GLFW_KEY_KP_8,
        GLFW_KEY_KP_9,
        GLFW_KEY_KP_DECIMAL,
        GLFW_KEY_KP_DIVIDE,
        GLFW_KEY_KP_MULTIPLY,
        GLFW_KEY_KP_SUBTRACT,
        GLFW_KEY_KP_ADD,
        GLFW_KEY_KP_ENTER,
        GLFW_KEY_KP_EQUAL,
        GLFW_KEY_LEFT_SHIFT,
        GLFW_KEY_LEFT_CONTROL,
        GLFW_KEY_LEFT_ALT,
        GLFW_KEY_LEFT_SUPER,
        GLFW_KEY_RIGHT_SHIFT,
        GLFW_KEY_RIGHT_CONTROL,
        GLFW_KEY_RIGHT_ALT,
        GLFW_KEY_RIGHT_SUPER,
        GLFW_KEY_MENU
    };

    const int GLFW_MOUSE_BUTTONS[] = {
        GLFW_MOUSE_BUTTON_1,
        GLFW_MOUSE_BUTTON_2,
        GLFW_MOUSE_BUTTON_3,
        GLFW_MOUSE_BUTTON_4,
        GLFW_MOUSE_BUTTON_5,
        GLFW_MOUSE_BUTTON_6,
        GLFW_MOUSE_BUTTON_7,
        GLFW_MOUSE_BUTTON_8
    };

    std::pair<double, double> InputHandler::_two_states[InputHandler::TWO_STATE_SIZE] = {};
    InputHandler::InputHandler(){
        for(int key = 0; key < KEY_SIZE; key++){
            _key_handlers[key] = nullptr;
            _key_states[key] = 0;
        }

        for(int key = 0; key < MOUSE_SIZE; key++){
            _mouse_handlers[key] = nullptr;
            _mouse_states[key] = 0;
        }

        for(int key = 0; key < TWO_STATE_SIZE; key++){
            _two_state_handlers[key] = nullptr;
            _two_states[key] = {0, 0};
        }
    }

    InputHandler::InputHandler(GLFWwindow* handle){
        glfwSetCursorPosCallback(handle, mousePosOffsetCallback);
        glfwSetScrollCallback(handle, mouseScrollOffsetCallback);
        for(int key = 0; key < KEY_SIZE; key++){
            _key_handlers[key] = nullptr;
        }

        for(int key = 0; key < MOUSE_SIZE; key++){
            _mouse_handlers[key] = nullptr;
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
        for(auto key: GLFW_KEYS){
            _key_states[key] = glfwGetKey(handle, key);
        }

        for(auto key: GLFW_MOUSE_BUTTONS){
            _mouse_states[key] = glfwGetMouseButton(handle, key);
        }
        glfwGetCursorPos(handle, &_two_states[0].first, &_two_states[0].second);
    }
};
