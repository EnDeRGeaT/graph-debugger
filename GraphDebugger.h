#pragma once
#include "include/glad/glad.h"
#include "glfw/include/GLFW/glfw3.h"
#include <algorithm>
#include <condition_variable>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <cwchar>
#include <functional>
#include <iostream>
#include <istream>
#include <memory>
#include <mutex>
#include <vector>
#include <thread>
#include <optional>
#include <queue>


namespace OpenGL{
    void APIENTRY glDebugOutput(GLenum source, 
                            GLenum type, 
                            unsigned int id, 
                            GLenum severity, 
                            GLsizei length, 
                            const char *message, 
                            const void *userParam);

    /*
    RAII encapsulation of a shader.
    Compiles shader on construction and prints logs in case of a compile error.
    Non-copyable.
    */
    class Shader{
        uint32_t _id;
        void compile(GLenum shader_type);
    public:
        Shader(std::istream& is, GLenum shader_type);
        Shader(const std::string& code, GLenum shader_type);
        Shader(const char* code, GLenum shader_type);
        Shader(const Shader& x) = delete;
        Shader& operator=(const Shader& x) = delete;
        Shader(Shader&& x);
        Shader& operator=(Shader&& x);
        ~Shader();
        uint32_t getID() const;
    };
    /*
    RAII encapsulation of a shader program.
    Links after use.
    You can't add shaders after the program's been linked
    */
    class ShaderProgram{
        bool _is_linked;
        std::vector<Shader> _shaders;
        uint32_t _shader_program_id;
    public:
        ShaderProgram();
        ~ShaderProgram();
        void addShader(std::istream& is, GLenum shader_type);
        void addShader(const Shader& shader);
        void addShader(Shader&& sh);
        void use();
        
        void setUniform1f(const std::string& name, float v0);
        void setUniform1i(const std::string& name, int v0);
        void setUniform1ui(const std::string& name, uint32_t v0);
        void setUniform1fv(const std::string& name, const std::vector<float>& value);
        void setUniform2fv(const std::string& name, const std::vector<float>& value);
        void setUniform3fv(const std::string& name, const std::vector<float>& value);
        void setUniform4fv(const std::string& name, const std::vector<float>& value);
        void setUniform1iv(const std::string& name, const std::vector<int>& value);
        void setUniform2iv(const std::string& name, const std::vector<int>& value);
        void setUniform3iv(const std::string& name, const std::vector<int>& value);
        void setUniform4iv(const std::string& name, const std::vector<int>& value);
        void setUniform1uiv(const std::string& name, const std::vector<uint32_t>& value);
        void setUniform2uiv(const std::string& name, const std::vector<uint32_t>& value);
        void setUniform3uiv(const std::string& name, const std::vector<uint32_t>& value);
        void setUniform4uiv(const std::string& name, const std::vector<uint32_t>& value);
        void setUniformMatrix2fv(const std::string& name, bool transpose, const std::vector<float>& value);
        void setUniformMatrix3fv(const std::string& name, bool transpose, const std::vector<float>& value);
        void setUniformMatrix4fv(const std::string& name, bool transpose, const std::vector<float>& value);
    };

    template<class T>
    class Buffer{
        GLenum _buffer_type;
        std::vector<T> _data;
        size_t _capacity;
        uint32_t _ID;
    public:
        Buffer(GLenum buffer_type, const std::vector<T>& data = {}):
        _buffer_type(buffer_type),
        _data(data.begin(), data.end()),
        _capacity(_data.capacity()) {
            glGenBuffers(1, &_ID);
            dump();
        } 
        Buffer(const Buffer& x):
        Buffer(Buffer(x._buffer_type, x._data)) {}
        Buffer& operator=(const Buffer& x) {
            return *this = Buffer(x);
        }
        Buffer(Buffer&& x):
        _buffer_type(0),
        _data({}),
        _capacity(0),
        _ID(0){
            *this = std::move(x);
        }

        Buffer& operator=(Buffer&& x){
            _buffer_type = x._buffer_type;
            _data(std::move(x._data));
            _capacity = x._capacity;
            _ID = x._ID;
            x._ID = 0;
            x._capacity = 0;
            return *this = std::move(x);
        }

        ~Buffer(){
            glDeleteBuffers(1, &_ID);
        }

        void bind() {
            glBindBuffer(_buffer_type, _ID);
        }

        void unbind() {
            glBindBuffer(_buffer_type, 0);
        }

        std::vector<T>& getData(){
            return _data;
        }

        void dump(){
            bind();
            if(_capacity != _data.capacity()){
                _capacity = _data.capacity();
                glBufferData(_buffer_type, _capacity * sizeof(T), nullptr, GL_DYNAMIC_DRAW);
            }
            if(_data.size()){
                auto ptr = glMapBufferRange(_buffer_type, 0, _data.size(), GL_MAP_WRITE_BIT);
                memcpy(ptr, &_data[0], sizeof(T) * _data.size());
                glUnmapBuffer(_buffer_type);
            }
        }

    };

    class VertexArray{
        uint32_t _ID;
    public:
        VertexArray();
        VertexArray(const VertexArray& x) = delete;
        VertexArray& operator=(const VertexArray& x) = delete;
        VertexArray(VertexArray&& x);
        VertexArray& operator=(VertexArray&& x);
        ~VertexArray();
        
        void bind();
        void unbind();
    };

    class Window;

    class Tab{
    protected:
        friend class Window;
        virtual void draw(Window& window) = 0;
        virtual void processInput(Window& window) = 0;    
    };

    /*
    This class creates a OpenGL window that supports multiple tabs.
    Since OpenGL is very bad at multithreading, it is not recommended to create more than one window.
    */

    class Window{
    private:
        std::mutex _tab_mutex;
        std::condition_variable _tab_cv;
        GLFWwindow* _handle;
        std::vector<std::shared_ptr<Tab>> _tabs;
        std::function<std::shared_ptr<Tab>()> func;
        bool _addition_pending;
        size_t _current_tab;
        int _width, _height;
        void resize();
        void processInput();
        void changeTab(size_t index);
    public:
        Window(int width, int height);
        ~Window();
        template<typename T, typename ...Args>
        std::weak_ptr<T> addTab(Args&&... args){
            std::unique_lock lck(_tab_mutex);
            _addition_pending = true;
            func = [&]() -> std::shared_ptr<T> {
                return std::make_shared<T>(args...);
            };
            _tab_cv.wait(lck, [&]() {return _addition_pending == false; });
            return std::dynamic_pointer_cast<T>(_tabs.back());
        }
        void deleteTab(size_t index);
        void run();
        int getWidth() const;
        int getHeight() const;
        GLFWwindow* getHandle() const;
    };

};

std::vector<std::pair<float, float>> forceDirected(std::vector<std::pair<float, float>> coords, const std::vector<std::pair<uint32_t, uint32_t>>& edges, std::pair<float, float> x_range, std::pair<float, float> y_range, float density = 30);

class GraphTab : public OpenGL::Tab {
    OpenGL::ShaderProgram _circle_shader, _line_shader;
    OpenGL::VertexArray _circle, _line;
    OpenGL::Buffer<std::pair<float, float>> _node_coords;
    OpenGL::Buffer<std::pair<uint32_t, uint32_t>> _edges;
    // OpenGL::Buffer<std::tuple<float, float, float>> _node_colors;
    // OpenGL::Buffer<float> _node_radiuses;

    float _node_radius;
    float _node_thickness;
    float _edge_thickness;
    float _zoom;
    float _graph_density;
    std::pair<float, float> _movement;

    void prettifyCoordinates(OpenGL::Window& window);
    void processInput(OpenGL::Window& win);
    void draw(OpenGL::Window& win);
public:
    std::mutex mutating_mutex;

    std::vector<std::pair<float, float>>& getCoordsVector();
    std::vector<std::pair<uint32_t, uint32_t>>& getEdgesVector();

    GraphTab(size_t node_count, const std::vector<std::pair<uint32_t, uint32_t>>& edges, OpenGL::Window& window);
    ~GraphTab();
};

class Graph{
    uint32_t _sz;
    uint32_t _is_directed;

    std::vector<std::pair<uint32_t, uint32_t>> _edges;
    std::vector<int32_t> _weights;
    std::vector<std::vector<std::pair<uint32_t, uint32_t>>> _adj_list;
    
    static std::shared_ptr<OpenGL::Window> _window;
    std::mutex _window_init_mutex;
    std::thread _running_thread;
    std::weak_ptr<GraphTab> _associated_tab;
    
public:

    Graph(uint32_t vertice_count, const std::vector<std::pair<uint32_t, uint32_t>>& edges, bool is_directed = false);
    Graph(uint32_t vertice_count, std::vector<std::tuple<uint32_t, uint32_t, int>> edges, bool is_directed = false);
    ~Graph();
    
    
    void visualize(const std::vector<uint32_t>& colors = {}, const std::vector<std::pair<float, float>>& coords = {}, const std::vector<float>& line_widths = {}, const std::vector<std::string>& node_labels = {});
    void visualizeBipartite();
    void visualizeWithHighlightedEdges(const std::vector<std::pair<uint32_t, uint32_t>>& edges);
    /*
    f must be an object (or a lambda) with an overloaded operator() that takes such arguments:
    (current_node, next_node, index_of_an_edge, const Graph& our_graph)
    if you want to change the graph, pass just by ref
    */
    template<typename T>
    void bfs(uint32_t starting_node, T& f){
        std::queue<uint32_t> q;
        std::vector<char> used(_sz);
        q.push(starting_node);
        used[starting_node] = 1;
        while(!q.empty()){
            auto v = q.front();
            q.pop();
            for(const auto& [u, index]: _adj_list[v]){
                if(used[u]) continue;
                f(v, u, index, *this);
                used[u] = 1;
                q.push(u);
            }
        }
    }

    
    template<typename T>
    void dfs(uint32_t node, T& f){
        std::vector<char> used(_sz);
        auto self = [&used, &f, this](auto& self, uint32_t v, uint32_t p){
            used[v] = 1;
            for(auto [u, ind]: _adj_list[v]){
                if(u == p) continue;
                self(self, u, v);
            }
            for(auto [u, ind]: _adj_list[v]){
                if(u == p) continue;
                f(v, u, p, ind, this);
            }
        };
        self(self, node, node);
    }

    uint32_t getNodes(); 
    std::vector<std::pair<uint32_t, uint32_t>> getEdges();
    std::vector<std::vector<std::optional<int32_t>>> getAdjacencyMatrix();
    
    std::vector<std::optional<int32_t>> findDistancesFromNode(uint32_t starting_node);
    std::optional<std::vector<uint32_t>> findShortestPathBetweenNodes(uint32_t start, uint32_t finish);
    std::vector<std::vector<std::optional<int32_t>>> findPairwiseDistances();
    
    std::optional<std::vector<uint32_t>> findHamiltonianCycle();
    std::optional<std::vector<uint32_t>> findEulerianCircuit();
    std::vector<uint32_t> findMinimumSpanningTree();
};