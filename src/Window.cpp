#include "GraphDebugger.h"
namespace OpenGL{

    void Window::resize(){
        glfwGetFramebufferSize(_handle, &_width, &_height);
        glViewport(0, 0, _width, _height);
    }

    void Window::processInput(){
        _input_handler.invoke();
        _input_handler.poll(_handle);
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
        glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
        glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
        // glfwWindowHint(GLFW_OPENGL_DEBUG_CONTEXT, true);
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
        // int flags; glGetIntegerv(GL_CONTEXT_FLAGS, &flags);        
        // if (flags & GL_CONTEXT_FLAG_DEBUG_BIT)
        // {
        //     glEnable(GL_DEBUG_OUTPUT);
        //     glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS); 
        //     glDebugMessageCallback(glDebugOutput, nullptr);
        //     glDebugMessageControl(GL_DONT_CARE, GL_DONT_CARE, GL_DONT_CARE, 0, nullptr, GL_TRUE);
        // } 
        _input_handler = InputHandler(_handle);

        struct ESCAPEKEY : public OpenGL::InputHandler::BaseKey {
            OpenGL::InputHandler& input;
            OpenGL::Window& window;
            ESCAPEKEY(OpenGL::InputHandler& input_handler, OpenGL::Window& win) :
                input(input_handler),
                window(win)
            {}

            virtual void perform(int key){
                if(key == GLFW_PRESS){
                    glfwSetWindowShouldClose(window._handle, true);
                }
            }
        };

        struct QKEY : public OpenGL::InputHandler::BaseKey {
            OpenGL::InputHandler& input;
            OpenGL::Window& window;
            bool pressed = false;
            QKEY(OpenGL::InputHandler& input_handler, OpenGL::Window& win) :
                input(input_handler),
                window(win)
            {}

            virtual void perform(int key){
                if(key == GLFW_PRESS && !pressed){
                    if(!window._tabs.empty()){
                        window.deleteTab(window._current_tab);
                        if(!window._tabs.empty() && window._current_tab == window._tabs.size()){
                            window._current_tab--;
                        }
                    }
                }
                pressed = key == GLFW_PRESS;
            }
        };

        struct LEFTARROWKEY : public OpenGL::InputHandler::BaseKey {
            OpenGL::InputHandler& input;
            OpenGL::Window& window;
            bool pressed = false;
            LEFTARROWKEY(OpenGL::InputHandler& input_handler, OpenGL::Window& win) :
                input(input_handler),
                window(win)
            {}

            virtual void perform(int key){
                if(key == GLFW_PRESS && !pressed){
                    if(window._current_tab != 0) window._current_tab--;
                }
                pressed = key == GLFW_PRESS;
            }
        };
        struct RIGHTARROWKEY : public OpenGL::InputHandler::BaseKey {
            OpenGL::InputHandler& input;
            OpenGL::Window& window;
            bool pressed = false;
            RIGHTARROWKEY(OpenGL::InputHandler& input_handler, OpenGL::Window& win) :
                input(input_handler),
                window(win)
            {}

            virtual void perform(int key){
                if(key == GLFW_PRESS && !pressed){
                    if(window._current_tab + 1 != window._tabs.size()) window._current_tab++;
                }
                pressed = key == GLFW_PRESS;
            }
        };

        _input_handler.attachKey(GLFW_KEY_ESCAPE, std::make_unique<ESCAPEKEY>(_input_handler, *this));
        _input_handler.attachKey(GLFW_KEY_Q, std::make_unique<QKEY>(_input_handler, *this));
        _input_handler.attachKey(GLFW_KEY_LEFT, std::make_unique<LEFTARROWKEY>(_input_handler, *this));
        _input_handler.attachKey(GLFW_KEY_RIGHT, std::make_unique<RIGHTARROWKEY>(_input_handler, *this));
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
        _tabs.erase(_tabs.begin() + static_cast<uint32_t>(index));
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
