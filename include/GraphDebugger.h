#pragma once
#define NOMINMAX // i hate windows
#include "glad/glad.h"
#include "GLFW/glfw3.h"
#include <algorithm>
#include <unordered_map>
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

    extern const uint8_t texture_atlas[];

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
        std::unordered_map<std::string, int> _uniform_positions;
        bool _is_linked;
        std::vector<Shader> _shaders;
        uint32_t _shader_program_id;
        int getUniformLocation(const std::string& name);
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
        bool _probably_changed;
    public:
        Buffer(GLenum buffer_type, const std::vector<T>& data = {}):
        _buffer_type(buffer_type),
        _data(data.begin(), data.end()),
        _probably_changed(true),
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
        _probably_changed(false),
        _ID(0){
            *this = std::move(x);
        }

        Buffer& operator=(Buffer&& x){
            _buffer_type = x._buffer_type;
            _data(std::move(x._data));
            _capacity = x._capacity;
            _ID = x._ID;
            _probably_changed = true;
            x._ID = 0;
            x._capacity = 0;
            x._probably_changed = false;
            return *this = std::move(x);
        }

        ~Buffer(){
            glDeleteBuffers(1, &_ID);
        }

        void bind() {
            glBindBuffer(_buffer_type, _ID);
        }

        uint32_t getID() const {
            return _ID;
        }

        void unbind() {
            glBindBuffer(_buffer_type, 0);
        }

        const std::vector<T>& getData() const {
            return _data;
        }

        std::vector<T>& mutateData() {
            _probably_changed = true;
            return _data;
        }

        bool isProbablyChanged() const {
            return _probably_changed;
        }

        void dump(){
            if(_probably_changed == false) return;
            _probably_changed = false;
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

    class InputHandler{
    public:
        struct BaseKey{
        public:
            virtual void perform(int key_state) = 0;
            virtual ~BaseKey() = default;
        };
        struct BaseTwoState {
        public:
            virtual void perform(std::pair<double, double> two_state) = 0;
            virtual ~BaseTwoState() = default;
        };
        InputHandler(GLFWwindow* handle);
        InputHandler();
        void attachKey(int key, std::unique_ptr<BaseKey>&& invoker);
        void attachMouse(int key, std::unique_ptr<BaseKey>&& invoker);
        void attachMousePos(std::unique_ptr<BaseTwoState>&& invoker);
        void attachMousePosOffset(std::unique_ptr<BaseTwoState>&& invoker);
        void attachMouseScrollOffset(std::unique_ptr<BaseTwoState>&& invoker);
        void invoke();
        int getKeyState(int key);
        int getMouseKeyState(int key);
        std::pair<double, double> getMousePos();
        std::pair<double, double> getMouseOffset();
        std::pair<double, double> getMouseScroll();
        void poll(GLFWwindow* handle);
    private:
        static void mousePosOffsetCallback(GLFWwindow* handle, double xoffset, double yoffset);
        static void mouseScrollOffsetCallback(GLFWwindow* handle, double xoffset, double yoffset);

        static const size_t KEY_SIZE = GLFW_KEY_LAST + 1, MOUSE_SIZE = GLFW_MOUSE_BUTTON_LAST + 1, TWO_STATE_SIZE = 3;
        int _key_states[KEY_SIZE];
        int _mouse_states[MOUSE_SIZE];
        static std::pair<double, double> _two_states[TWO_STATE_SIZE];

        std::unique_ptr<BaseKey> _key_handlers[KEY_SIZE];
        std::unique_ptr<BaseKey> _mouse_handlers[MOUSE_SIZE];
        std::unique_ptr<BaseTwoState> _two_state_handlers[TWO_STATE_SIZE];
    };

    /*
    This class creates a OpenGL window that supports multiple tabs.
    Since OpenGL is very bad at multithreading, it is not recommended to create more than one window.
    */
    class Window{
    private:
        InputHandler _input_handler;
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
            std::thread async_thread([&]() {
                std::unique_lock lck(_tab_mutex);
                _addition_pending = true;
                func = [&]() -> std::shared_ptr<T> {
                    return std::make_shared<T>(args...);
                };
                _tab_cv.wait(lck, [&]() {return _addition_pending == false; });
            });
            async_thread.join();
            return std::dynamic_pointer_cast<T>(_tabs.back());
        }
        void deleteTab(size_t index);
        void run();
        int getWidth() const;
        int getHeight() const;
        GLFWwindow* getHandle() const;
    };
};

std::vector<std::pair<float, float>> forceDirected(std::vector<std::pair<float, float>> coords, const std::vector<std::pair<uint32_t, uint32_t>>& edges);

class GraphTab : public OpenGL::Tab {
    // the input handler
    OpenGL::InputHandler _input_handler;

    // empty vao because it doesn't work otherwise
    OpenGL::VertexArray _empty_vao;

    // node related
    OpenGL::ShaderProgram _node_shader; // shader program for drawing nodes
    OpenGL::Buffer<std::pair<float, float>> _node_coords;

    struct NodeParams{
        uint32_t color;
        float radius;
    };

    OpenGL::Buffer<NodeParams> _node_properties;
    OpenGL::Buffer<uint32_t> _available_node_indices;
    std::vector<uint32_t> _deleted_node_indices;

    // i don't know if i should add this one to the NodeParams, like, it just doesn't make sense to? Because I will send this to the GPU, mmmm
    std::vector<uint32_t> _node_labels; // labels are indexes to their respective strings in _strings array 

    uint32_t _default_node_color;
    float _default_node_radius;
    float _default_node_thickness;

    void addNode(std::pair<int, int> coords, NodeParams properties, std::string label = "");
    void deleteNode(uint32_t node_index);

    void prettifyCoordinates(OpenGL::Window& window);

    // edges related
    OpenGL::ShaderProgram _edge_shader;
    OpenGL::Buffer<std::pair<uint32_t, uint32_t>> _edges;

    OpenGL::Buffer<uint32_t> _available_edge_indices;
    std::vector<uint32_t> _deleted_edge_indices;
    OpenGL::Buffer<uint32_t> _mutli_edge_index;
    std::unordered_map<std::pair<uint32_t, uint32_t>, uint32_t> _count_edges;

    struct EdgeParams{
        uint32_t color;
        float thickness;
    };
    OpenGL::Buffer<EdgeParams> _edge_properties;
    std::vector<uint32_t> _edge_labels;
    uint32_t _default_edge_color;
    float _default_edge_thickness;

    void addEdge(std::pair<uint32_t, uint32_t> edge, EdgeParams properties, std::string str = "");
    void updateEdgeLabelPos(uint32_t edge_index);
    void deleteEdge(uint32_t edge_index);

    // text related
    const int letters_in_row = 8, letters_in_column = 12;
    const int sdf_glyph_width = 32, sdf_glyph_height = 56, glyph_advance = 19;
    const int actual_glyph_width = 19, actual_glyph_height = 32;
    const int texture_atlas_width = letters_in_column * sdf_glyph_width, texture_atlas_height = letters_in_row * sdf_glyph_height;

    uint32_t _texture_atlas_id; // the only thing that is not RAII here
    OpenGL::ShaderProgram _string_shader;
    OpenGL::Buffer<uint32_t> _string_buffer;

    struct StringParams {
        uint32_t color;
        float scale;
    };

    struct StringCoord {
        int alignment;
        int is_affected_by_movement;
        std::pair<float, float> coord;
    };
    
    enum class StringAlignment {
        top_left = 0,
        top_center = 1,
        top_right = 2,
        middle_left = 3,
        middle_center = 4,
        middle_right = 5,
        bottom_left = 6,
        bottom_center = 7,
        bottom_right = 8,
    };

    bool _was_mutated;
    
    std::vector<std::string> _strings;
    OpenGL::Buffer<StringCoord> _string_coords;
    OpenGL::Buffer<StringParams> _string_properties;
    OpenGL::Buffer<uint32_t> _string_prefix_sum;
    OpenGL::Buffer<uint32_t> _available_string_indices;
    std::vector<uint32_t> _deleted_string_indices;

    uint32_t _default_string_color;
    float _default_string_scale;

    uint32_t addString(std::string str, StringCoord coordinates, StringParams parameters);
    std::string& mutateString(size_t string_index);
    StringCoord& mutateStringCoord(size_t string_index);
    StringParams& mutateStringProperty(size_t string_index);
    void deleteString(size_t string_index);

    // graph view related
    float _zoom;
    float _graph_density;
    std::pair<float, float> _movement;

    void processInput(OpenGL::Window& win);
    void draw(OpenGL::Window& win);

    std::mutex _mutating_mutex;
public:

    GraphTab(size_t node_count, const std::vector<std::pair<uint32_t, uint32_t>>& edges, OpenGL::Window& window);
    ~GraphTab();

    void setNodeCoords(const std::vector<std::pair<float, float>>& coords);
    void setNodeColors(const std::vector<uint32_t>& colors);
    void setNodeLabels(const std::vector<std::string>& labels);

    std::vector<std::pair<uint32_t, uint32_t>> getEdges() const;
    void setEdges(const std::vector<std::pair<uint32_t, uint32_t>>& edges);
    void setEdgeColors(const std::vector<uint32_t>& colors);
    void setEdgeLabels(const std::vector<std::string>& labels);
    void setEdgesThickness(const std::vector<float>& thicknesses);
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
    /**
     * @brief Construct an unweighted graph
     * 
     * @param vertice_count number of nodes in a graph
     * @param edges list of edges (0 indexed)
     * @param is_directed true if graph is directed
     */
    Graph(uint32_t vertice_count, const std::vector<std::pair<uint32_t, uint32_t>>& edges, bool is_directed = false);
    
    /**
     * @brief Constructs a weighted graph
     * 
     * @param vertice_count number of nodes in a graph
     * @param edges list of edges (0 indexed)
     * @param is_directed true if graph is directed
     */
    Graph(uint32_t vertice_count, std::vector<std::tuple<uint32_t, uint32_t, int>> edges, bool is_directed = false);
    ~Graph();
    
    /**
     * @brief Creates a window with the visualization of the graph. All parameters are optional.
     * 
     * @param node_colors coloring of the nodes, colors are encoded in uint32_t like that: 0xRRGGBB 
     * @param edge_colors coloring of the edges, colors are encoded in uint32_t like that: 0xRRGGBB 
     * @param coords coordinates of the nodes, coordinates must be pairs {x, y} in screen space (pixels)  
     * @param line_widths thicknesses of the edges in pixels 
     * @param node_labels names for the nodes
     */
    void visualize(const std::vector<uint32_t>& node_colors = {}, const std::vector<uint32_t>& edge_colors = {}, const std::vector<std::pair<float, float>>& coords = {}, const std::vector<float>& line_widths = {}, const std::vector<std::string>& node_labels = {});

    /**
     * @brief Draws a graph with some edges highlighted
     * 
     * @param edges edges to be highlighted
     */
    void visualizeWithHighlightedEdges(const std::vector<std::pair<uint32_t, uint32_t>>& edges);

    /*
    */

    /**
     * @brief Generic BFS
     * 
     * @param starting_node node to start bfs from
     * @param f  f must be an object (or a lambda) with an overloaded operator() that takes such arguments:
                (current node, next node, index of an edge, graph)
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

    /**
     * @brief Generic DFS
     * 
     * @param starting_node node to start dfs from
     * @param f  f must be an object (or a lambda) with an overloaded operator() that takes such arguments:
                (current node, next node, previous node, index of an edge, graph)
                if you want to change the graph, pass just by ref
     */
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

    /**
     * @brief Get the number of nodes in the graph
     * 
     * @return uint32_t number of nodes
     */
    uint32_t getNodes(); 
    
    /**
     * @brief Get the list of edges
     * 
     * @return std::vector<std::pair<uint32_t, uint32_t>> edges
     */
    std::vector<std::pair<uint32_t, uint32_t>> getEdges();
    
    /**
     * @brief Get the adjacency matrix of the graph. std::optional doesn't have a value if there's no path
     * 
     * @return std::vector<std::vector<std::optional<int32_t>>> 
     */
    std::vector<std::vector<std::optional<int32_t>>> getAdjacencyMatrix(); 
    
    /**
     * @brief Finds distances from the first node using Dijkstra's algorithm. std::optional doesn't have a value if there's no path
     * 
     * @param starting_node source node
     * @return std::vector<std::optional<int32_t>> vector of distances
     */
    std::vector<std::optional<int32_t>> findDistancesFromNode(uint32_t starting_node);

    /**
     * @brief Find the shortest path between start and finish. std::optional doesn't have a value if there's no path
     * 
     * @param start 
     * @param finish 
     * @return std::optional<std::vector<uint32_t>> 
     */
    std::optional<std::vector<uint32_t>> findShortestPathBetweenNodes(uint32_t start, uint32_t finish);

    /**
     * @brief Finds pairwise distances using Floyd-Warshall algorithm
     * 
     * @return std::vector<std::vector<std::optional<int32_t>>> vector of distances
     */
    std::vector<std::vector<std::optional<int32_t>>> findPairwiseDistances();
    
    /**
     * @brief Finds Hamiltonian cycle. std::optional doesn't have a value if the Graph is not hamiltonian
     * 
     * @return std::optional<std::vector<uint32_t>> 
     */
    std::optional<std::vector<uint32_t>> findHamiltonianCycle(); 

    /**
     * @brief Finds Eulerian circuit. std::optional doesn't have a value if the Graph is not eulerian
     * 
     * @return std::optional<std::vector<uint32_t>> 
     */
    std::optional<std::vector<uint32_t>> findEulerianCircuit();

    /**
     * @brief Returns the edges of minimum spanning tree in a graph
     * 
     * @return std::vector<uint32_t> 
     */
    std::vector<uint32_t> findMinimumSpanningTree();
};
