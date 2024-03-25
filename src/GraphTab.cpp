#include "GraphDebugger.h"
#include <algorithm>
#include <cstdint>
#include <mutex>
#include <optional>
#include <random>
#include <sstream>
#include <utility>

void GraphTab::prettifyCoordinates(OpenGL::Window& window){
    auto& coords = _node_coords.mutateData();
    float width = window.getWidth();
    float height = window.getHeight();
    coords = forceDirected(coords, _edges.getData(), {0, width}, {0, height}, _graph_density);
    auto [min_x, min_y] = coords[0];
    auto [max_x, max_y] = coords[0];
    for(auto [x, y]: coords){
        min_x = std::min(min_x, x);
        max_x = std::max(max_x, x);
        min_y = std::min(min_y, y);
        max_y = std::max(max_y, y);
    }
    // i need some value z, such that mn + z == res - mx - z
    _movement.first = (width - max_x - min_x) / 2;
    _movement.second = (height - max_y - min_y) / 2;

    const float percent_x = width * 0.90; 
    const float percent_y = height * 0.90; 

    _zoom = std::max({(max_x - min_x) / percent_x, (max_y - min_y) / percent_y, _zoom});
}

void GraphTab::processInput(OpenGL::Window& window){
    auto handle = window.getHandle();
    int width = window.getWidth();
    int height = window.getHeight();


    int left = glfwGetMouseButton(handle, GLFW_MOUSE_BUTTON_LEFT);
    int right = glfwGetMouseButton(handle, GLFW_MOUSE_BUTTON_RIGHT);
    double cursor_x, cursor_y;

    glfwGetCursorPos(handle, &cursor_x, &cursor_y);
    static double previous_cursor_x, previous_cursor_y;
    double cursor_dx = (cursor_x - previous_cursor_x) * _zoom, cursor_dy = (cursor_y - previous_cursor_y) * _zoom;
    previous_cursor_x = cursor_x;
    previous_cursor_y = cursor_y;

    cursor_x -= width / 2.;
    cursor_y -= height / 2.;

    cursor_x *= _zoom;
    cursor_y *= _zoom;

    cursor_x -= _movement.first;
    cursor_y -= _movement.second;

    cursor_x += width / 2.;
    cursor_y += height / 2.;
    
    static bool left_mouse_button_pressed = false;
    static bool right_mouse_button_pressed = false;

    auto& coords = _node_coords.getData();
    
    auto getClickedNode = [rsqr = _default_node_radius * _default_node_radius, &coords, this](float cx, float cy){
        size_t ind = 0;
        for(; ind < coords.size(); ind++){
            auto &[x, y] = coords[ind];
            double dx = x - cx;
            double dy = y - cy;
            if(dx * dx + dy * dy <= rsqr / _zoom){
                break;
            }
        }
        return ind;
    };


    if(left == GLFW_PRESS){
        static size_t index = 0;
        if(!left_mouse_button_pressed){
            index = getClickedNode(cursor_x, cursor_y);
            left_mouse_button_pressed = true;
        }
        else {
            if(index < coords.size()){
                auto &mutable_coords = _node_coords.mutateData();
                mutable_coords[index].first += cursor_dx;
                mutable_coords[index].second += cursor_dy;
            }
            else{
                _movement.first += cursor_dx;
                _movement.second += cursor_dy;
            }
        }
    }
    else if(left == GLFW_RELEASE){
        left_mouse_button_pressed = false;
    }

    if(right == GLFW_PRESS){
        if(!right_mouse_button_pressed){
            static std::optional<size_t> first_highlighted = std::nullopt;
            size_t v = getClickedNode(cursor_x, cursor_y);
            if(v != coords.size()){
                if(first_highlighted.has_value()){
                    std::cerr << "adding edge\n";
                    addEdge({*first_highlighted, v}, {_default_edge_color, _default_edge_thickness});
                    _node_properties.mutateData()[*first_highlighted].color = _default_node_color;
                    first_highlighted = std::nullopt;
                }
                else{
                    std::cerr << "highlighted a node\n";
                    first_highlighted = v;
                    _node_properties.mutateData()[v].color = 0x00FF00;
                }
            }
            else{
                std::cerr << "adding node\n";
                addNode({cursor_x, cursor_y}, {_default_node_color, _default_node_radius});
                if(first_highlighted.has_value()) _node_properties.mutateData()[*first_highlighted].color = _default_node_color;
                first_highlighted = std::nullopt;
            }
            right_mouse_button_pressed = true;
        }
    }
    else if(right == GLFW_RELEASE){
        right_mouse_button_pressed = false;
    }

// F KEY HANDLING (apply force directed algorithm on the graph)
    static bool f_pressed = false;
    int f_key = glfwGetKey(handle, GLFW_KEY_F);
    if(f_key == GLFW_PRESS){
        if(!f_pressed && coords.size()){
            prettifyCoordinates(window);
        }
        f_pressed = true;
    }
    else if(f_key == GLFW_RELEASE){
        f_pressed = false;
    }

// C KEY HANDLING (ask to input the density value)
    int c_key = glfwGetKey(handle, GLFW_KEY_C);
    if(c_key == GLFW_PRESS){
        std::cout << "Please input the new density value: ";
        std::cin >> _graph_density;
    }


// + KEY HANDLING (zooming in)
    int equal_key = glfwGetKey(handle, GLFW_KEY_EQUAL);
    if(equal_key == GLFW_PRESS){
        _zoom *= 0.95f;
    }

// - KEY HANDLING (zooming out)
    int minus_key = glfwGetKey(handle, GLFW_KEY_MINUS);
    if(minus_key == GLFW_PRESS){
        _zoom /= 0.95f;
    }

}

void GraphTab::draw(OpenGL::Window& window){
    _node_coords.dump();
    _edges.dump();

    _edge_properties.dump();
    _edge_shader.use();
    _edge_shader.setUniform2fv("resolution", {1.0f * window.getWidth(), 1.0f *  window.getHeight()});
    _edge_shader.setUniform2fv("movement", {_movement.first, _movement.second});
    _edge_shader.setUniform1f("zoom", _zoom);
    glDrawArrays(GL_TRIANGLES, 0, 6 * _edges.getData().size());

    _node_properties.dump();
    _node_shader.use();
    _node_shader.setUniform2fv("resolution", {1.0f * window.getWidth(), 1.0f *  window.getHeight()});
    _node_shader.setUniform2fv("movement", {_movement.first, _movement.second});
    _node_shader.setUniform1f("bound", _default_node_thickness);
    _node_shader.setUniform1f("zoom", _zoom);
    glDrawArrays(GL_TRIANGLES, 0, 6 * _node_coords.getData().size());


    _string_properties.dump();
    _string_shader.use();
    _string_shader.setUniform2fv("resolution", {1.0f * window.getWidth(), 1.0f *  window.getHeight()});
    _string_shader.setUniform2fv("movement", {_movement.first, _movement.second});
    _string_shader.setUniform1f("zoom", _zoom);
    for(uint32_t string_index = 0; string_index < _strings.size(); string_index++){
        const auto& str = _strings[string_index];
        auto& buffer = _string_buffer.mutateData();
        buffer.resize(str.size());
        std::copy(str.begin(), str.end(), buffer.begin());
        _string_buffer.dump();
        _string_shader.setUniform1ui("string_index", string_index);
        glBindTexture(GL_TEXTURE_2D, _texture_atlas_id);
        glDrawArrays(GL_TRIANGLES, 0, 6 * str.size());
    }
}

void GraphTab::addNode(std::pair<int, int> coords, NodeParams properties){
    _node_coords.mutateData().push_back(coords);
    _node_properties.mutateData().push_back(properties);
}

void GraphTab::addEdge(std::pair<uint32_t, uint32_t> edge, EdgeParams properties){
    std::cerr << _node_coords.getData().size() << ' ' << "Edge: " << edge.first << ' ' << edge.second << '\n';
    _edges.mutateData().push_back(edge);
    _edge_properties.mutateData().push_back(properties);
}

void GraphTab::addString(std::string str, std::pair<float, float> coord){
    _strings.push_back(std::move(str));
    _string_properties.mutateData().emplace_back(_default_string_color, _default_string_scale, std::move(coord));
}

std::string& GraphTab::mutateString(size_t index) {
    return _strings[index];
}

GraphTab::StringParams& GraphTab::mutateStringProperty(size_t index) {
    return _string_properties.mutateData()[index];
}

GraphTab::GraphTab(size_t node_count, const std::vector<std::pair<uint32_t, uint32_t>>& edges, OpenGL::Window& window):
    _empty_vao(),
    _node_shader(),
    _node_coords(GL_SHADER_STORAGE_BUFFER),
    _node_properties(GL_SHADER_STORAGE_BUFFER),
    _default_node_color(0x0),
    _default_node_radius(25),
    _default_node_thickness(5),
    _edge_shader(),
    _line(),
    _edges(GL_SHADER_STORAGE_BUFFER),
    _edge_properties(GL_SHADER_STORAGE_BUFFER),
    _default_edge_color(0x0),
    _default_edge_thickness(5),
    _string_buffer(GL_SHADER_STORAGE_BUFFER),
    _strings(),
    _string_properties(GL_SHADER_STORAGE_BUFFER),
    _default_string_color(0x0),
    _default_string_scale(5.0),
    _zoom(1.0),
    _graph_density(30),
    _movement(0.0, 0.0)
{
    {
        std::stringstream vertexstream;
        vertexstream << R"(
        #version 460 core

        struct NodeParams{
            uint vertex_color;
            float circle_radius;
        };

        layout (std430, binding=0) buffer coordinates {
            vec2 vertex_coords[];
        };

        layout (std430, binding=2) buffer node_parameters {
            NodeParams parameters[];
        };

        const vec2 direction[6] = {
            {-1.0, -1.0},
            {-1.0, 1.0},
            {1.0, -1.0},
            {1.0, -1.0},
            {1.0, 1.0},
            {-1.0, 1.0}
        };

        uniform vec2 resolution;
        uniform vec2 movement;
        uniform float zoom;

        out vec2 center;
        out vec3 color;
        out float radius;

        void main(){
            int node_id = gl_VertexID / 6;
            int vertice_id = gl_VertexID % 6;

            center = (vertex_coords[node_id] - resolution / 2 + movement) / zoom + resolution / 2;

            color = vec3(parameters[node_id].vertex_color & 255, (parameters[node_id].vertex_color >> 8) & 255, (parameters[node_id].vertex_color >> 16) & 255) / 255;

            radius = parameters[node_id].circle_radius;

            vec2 r = 2 * radius / zoom / resolution; 

            gl_Position = vec4(2 * center.x / resolution.x - 1, 1 - 2 * center.y / resolution.y, 0.0, 1.0);
            gl_Position.xy += r * direction[vertice_id];
        }
        )";
        
        std::stringstream fragmentstream;
        fragmentstream << R"(
        #version 460 core
        layout(origin_upper_left, pixel_center_integer) in vec4 gl_FragCoord;

        in vec2 center;
        in vec3 color;
        in float radius;

        out vec4 FragColor;

        uniform float bound;
        uniform float zoom;

        vec3 inner = vec3(1.0, 1.0, 1.0);

        float hardstep(float a, float b, float x){
            if(x < a) return 0;
            return 1;
        }

        void main(){
            float bound_zoomed = bound / zoom;
            float rad_zoomed = radius / zoom;
            vec2 diff = gl_FragCoord.xy - center.xy;
            float dist = length(diff);
            float t = 1.0 - hardstep(rad_zoomed - 2 * bound_zoomed, rad_zoomed - bound_zoomed, dist);
            float t_border = 1.0 - hardstep(rad_zoomed - bound_zoomed, rad_zoomed, dist);
            FragColor = vec4(mix(color, inner, t), t_border);
        }
        )";

        _node_shader.addShader(vertexstream, GL_VERTEX_SHADER);
        _node_shader.addShader(fragmentstream, GL_FRAGMENT_SHADER);
    }

    {
        std::stringstream vertexstream;
        vertexstream << R"(
        #version 460 core
        struct EdgeParams{
            uint edge_color;
            float edge_width;
        };
        
        layout (std430, binding=0) buffer coordinates {
            vec2 vertex_coords[];
        };

        layout (std430, binding=1) buffer edge_buffer {
            uvec2 edges[];
        };

        layout (std430, binding=3) buffer edge_parameters {
            EdgeParams parameters[];
        };

        out vec3 color;

        float direction[6] = {
            -1.0,
            1.0,
            -1.0,
            1.0,
            1.0,
            -1.0
        };

        uniform vec2 resolution;
        uniform vec2 movement;
        uniform float zoom;

        vec2 apply(vec2 a){
            return vec2(2 * a.x / resolution.x - 1, 1 - 2 * a.y / resolution.y);
        }

        void main(){
            int edge_id = gl_VertexID / 6;
            int vertice_id = gl_VertexID % 6;

            color = vec3(parameters[edge_id].edge_color & 255, (parameters[edge_id].edge_color >> 8) & 255, (parameters[edge_id].edge_color >> 16) & 255) / 255;

            vec2 v = vertex_coords[edges[edge_id].x] + movement;
            vec2 u = vertex_coords[edges[edge_id].y] + movement;

            v = apply(v);
            u = apply(u);

            vec2 dir = normalize((v - u) * resolution);
            vec2 offset = vec2(-dir.y, dir.x) * parameters[edge_id].edge_width / resolution;


            gl_Position.zw = vec2(0.0, zoom);

            if(vertice_id == 0 || vertice_id == 1 || vertice_id == 3) {
                gl_Position.xy = v + offset * direction[vertice_id];
            }
            else{
                gl_Position.xy = u + offset * direction[vertice_id];
            }
        }
        )";

        std::stringstream fragmentstream;
        fragmentstream << R"(
        #version 460 core
        out vec4 FragColor;

        in vec3 color;

        void main(){
            FragColor.rgb = color;
            FragColor.a = 1.0;
        }
        )";
        std::cerr << "second one\n";
        _edge_shader.addShader(vertexstream, GL_VERTEX_SHADER);
        _edge_shader.addShader(fragmentstream, GL_FRAGMENT_SHADER);
    }    

    {
        std::stringstream vertexstream;
        vertexstream << R"(
        )";
        
        std::stringstream fragmentstream;
        fragmentstream << R"(
        )";

        _string_shader.addShader(vertexstream, GL_VERTEX_SHADER);
        _string_shader.addShader(fragmentstream, GL_FRAGMENT_SHADER);
    }


    {
        auto& vec = _edges.mutateData();
        vec = edges;
    }

    {
        std::mt19937 rng(0);
        auto& vec = _node_coords.mutateData();
        auto distr_x = std::uniform_real_distribution<float>(0, window.getWidth());
        auto distr_y = std::uniform_real_distribution<float>(0, window.getHeight());
        vec = std::vector<std::pair<float, float>>(node_count);
        std::generate(vec.begin(), vec.end(), [&](){ return std::make_pair(distr_x(rng), distr_y(rng)); });
        prettifyCoordinates(window);
    }

    {
        auto& vec = _node_properties.mutateData();
        vec = std::vector<NodeParams>(node_count, {_default_node_color, _default_node_radius});
    }

    {
        auto& vec = _edge_properties.mutateData();
        vec = std::vector<EdgeParams>(_edges.getData().size(), {_default_edge_color, _default_edge_thickness});
    }

    _node_coords.bind();
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, _node_coords.getID());

    _edges.bind();
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, _edges.getID());

    _node_properties.bind();
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 2, _node_properties.getID());

    _edge_properties.bind();
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 3, _edge_properties.getID());
    
    _string_buffer.bind();
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 4, _string_buffer.getID());

    _string_properties.bind();
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 5, _string_properties.getID());

    glGenTextures(1, &_texture_atlas_id);
    glBindTexture(GL_TEXTURE_2D, _texture_atlas_id);
    glTexImage2D(
            GL_TEXTURE_2D,
            0,
            GL_RED,
            /*placehorder*/, 
            /*placehorder*/, 
            0,
            GL_RED,
            GL_UNSIGNED_BYTE,
            OpenGL::texture_atlas
    );
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    _empty_vao.bind();
}

std::vector<std::pair<float, float>>& GraphTab::getCoordsVector(){
    return _node_coords.mutateData();
}

std::vector<std::pair<uint32_t, uint32_t>>& GraphTab::getEdgesVector(){
    return _edges.mutateData();
}

GraphTab::~GraphTab(){
    std::lock_guard lock(mutating_mutex);
    glDeleteTextures(1, &_texture_atlas_id);
}
