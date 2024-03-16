#include "GraphDebugger.h"
#include <mutex>
namespace OpenGL{

    void Window::resize(){
        glfwGetFramebufferSize(_handle, &_width, &_height);
        glViewport(0, 0, _width, _height);
    }

    void Window::processInput(){
        int escape_key = glfwGetKey(_handle, GLFW_KEY_ESCAPE);
        if(escape_key == GLFW_PRESS){
            glfwSetWindowShouldClose(_handle, true);
        }
        
        static bool left_pressed = false;
        int left_key = glfwGetKey(_handle, GLFW_KEY_LEFT);
        if(left_key == GLFW_PRESS){
            if(_current_tab != 0) _current_tab--;
        }
        left_pressed = left_key == GLFW_PRESS;

        static bool right_pressed = false;
        int right_key = glfwGetKey(_handle, GLFW_KEY_RIGHT);
        if(right_key == GLFW_PRESS){
            if(_current_tab + 1 != _tabs.size()) _current_tab++;
        }
        right_pressed = right_key == GLFW_PRESS;

        static bool q_pressed = false;
        int q_key = glfwGetKey(_handle, GLFW_KEY_Q);
        if(q_key == GLFW_PRESS && !q_pressed){
            if(!_tabs.empty()){
                deleteTab(_current_tab);
                if(!_tabs.empty() && _current_tab == _tabs.size()){
                    _current_tab--;
                }
            }
        }
        q_pressed = q_key == GLFW_PRESS;

        if(!_tabs.empty()) _tabs[_current_tab]->processInput(*this);
    }

    Window::Window(int width, int height):
    _tabs(),
    _addition_pending(false),
    _current_tab(0),
    _width(width),
    _height(height)
    {
        glfwInit();
        glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
        glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 6);
        glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
        glfwWindowHint(GLFW_OPENGL_DEBUG_CONTEXT, true);
    #ifdef __APPLE__
        glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
    #endif
        // glfw window creation
        // --------------------
        _handle = glfwCreateWindow(_width, _height, "Graph Debugger", NULL, NULL);
        if (_handle == NULL)
        {
            std::cerr << "Failed to create GLFW window" << std::endl;
            return;
        }
        glfwMakeContextCurrent(_handle);
        glfwSetInputMode(_handle, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
        glfwSwapInterval(1);

        // glad: load all OpenGL function pointers
        // ---------------------------------------
        if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
        {
            std::cerr << "Failed to initialize GLAD" << std::endl;
            return;
        }    

        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA); 
        int flags; glGetIntegerv(GL_CONTEXT_FLAGS, &flags);        
        if (flags & GL_CONTEXT_FLAG_DEBUG_BIT)
        {
            glEnable(GL_DEBUG_OUTPUT);
            glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS); 
            glDebugMessageCallback(glDebugOutput, nullptr);
            glDebugMessageControl(GL_DONT_CARE, GL_DONT_CARE, GL_DONT_CARE, 0, nullptr, GL_TRUE);
        } 

    }

    void Window::run(){
        glfwMakeContextCurrent(_handle);
        while (!glfwWindowShouldClose(_handle)) {
            // render
            // ------
            glfwMakeContextCurrent(_handle);
            glClearColor(1.0f, 1.0f, 1.0f, 1.0f);
            glClear(GL_COLOR_BUFFER_BIT);
        
            resize();
            {
            std::lock_guard lock(_tab_mutex);
            if(_addition_pending){
                _tabs.push_back(func());
                _addition_pending = false;
                _tab_cv.notify_one();
            }
            processInput();

            if(!_tabs.empty()){
                _tabs[_current_tab]->draw(*this);
               
            }
            }
            // glfw: swap buffers and poll IO events (keys pressed/released, mouse moved etc.)
            // -------------------------------------------------------------------------------
            glfwSwapBuffers(_handle);
            glfwPollEvents();
        }
    }

    Window::~Window(){
        {
            std::lock_guard lock(_tab_mutex);
            _tabs.clear();
            glfwTerminate();
        } 
    }

    void Window::changeTab(size_t index){
        _current_tab = index;
    }

    void Window::deleteTab(size_t index){
        _tabs.erase(_tabs.begin() + index);
    }

    int Window::getWidth() const {
        return _width;
    }

    int Window::getHeight() const {
        return _height;
    }

    GLFWwindow* Window::getHandle() const{
        return _handle;
    }
};
