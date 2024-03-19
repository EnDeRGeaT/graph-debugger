#include "GraphDebugger.h"
#include <algorithm>
#include <mutex>
#include <optional>
#include <random>
#include <sstream>
#include <utility>

void GraphTab::prettifyCoordinates(OpenGL::Window& window){
    auto& coords = _node_coords.getData();
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
    
    auto getClickedNode = [rsqr = _node_radius * _node_radius, &coords, this](float cx, float cy){
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
                coords[index].first += cursor_dx;
                coords[index].second += cursor_dy;
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
                    _edges.getData().emplace_back(*first_highlighted, v);
                    first_highlighted = std::nullopt;
                }
                else{
                    first_highlighted = v;
                }
            }
            else{
                coords.emplace_back(cursor_x, cursor_y);
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


// t KEY HANDLING (testing things out)
    int t_key = glfwGetKey(handle, GLFW_KEY_T);
    if(t_key == GLFW_PRESS){
        _zoom /= 0.95f;
    }

}

void GraphTab::draw(OpenGL::Window& window){
    _line.bind();
    _line_shader.use();
    _edges.dump();
    _node_coords.dump();
    _line_shader.setUniform2fv("resolution", {1.0f * window.getWidth(), 1.0f *  window.getHeight()});
    _line_shader.setUniform2fv("movement", {_movement.first, _movement.second});
    _line_shader.setUniform1f("width", _edge_thickness);
    _line_shader.setUniform1f("zoom", _zoom);
    glDrawElements(GL_LINES, _edges.getData().size() * 2, GL_UNSIGNED_INT, 0);

    _circle.bind();
    _circle_shader.use();
    _circle_shader.setUniform2fv("resolution", {1.0f * window.getWidth(), 1.0f *  window.getHeight()});
    _circle_shader.setUniform2fv("movement", {_movement.first, _movement.second});
    _circle_shader.setUniform1f("radius", _node_radius);
    _circle_shader.setUniform1f("bound", _node_thickness);
    _circle_shader.setUniform1f("zoom", _zoom);
    glDrawArrays(GL_POINTS, 0, _node_coords.getData().size());
}

GraphTab::GraphTab(size_t node_count, const std::vector<std::pair<uint32_t, uint32_t>>& edges, OpenGL::Window& window):
    _circle_shader(),
    _line_shader(),
    _circle(),
    _line(),
    _node_coords(GL_ARRAY_BUFFER),
    _edges(GL_ELEMENT_ARRAY_BUFFER),
    _node_radius(25),
    _node_thickness(5),
    _edge_thickness(5),
    _zoom(1.0),
    _graph_density(30),
    _movement(0.0, 0.0)
{
    {
        std::stringstream vertexstream;
        vertexstream << R"(
        #version 460 core
        layout (location = 0) in vec2 vertex_position;

        uniform vec2 resolution;
        uniform vec2 movement;
        uniform float zoom;
        out vec2 center;

        void main(){
            center = (vertex_position - resolution / 2 + movement) / zoom + resolution / 2;
            gl_Position = vec4(2 * center.x / resolution.x - 1, 1 - 2 * center.y / resolution.y, 0.0, 1.0);
        }
        )";
        
        std::stringstream geometrystream;
        geometrystream << R"(
        #version 460 core
        layout (points) in;
        layout (triangle_strip, max_vertices = 4) out;

        uniform vec2 resolution;
        uniform float radius;
        uniform float zoom;
        in vec2 center[];
        out vec2 cen;

        void main(){
            float rx = 2 * radius / zoom / resolution.x; 
            float ry = 2 * radius / zoom / resolution.y; 

            cen = center[0];
            gl_Position = gl_in[0].gl_Position + vec4(-rx, ry, 0.0, 0.0);
            EmitVertex();

            gl_Position = gl_in[0].gl_Position + vec4(-rx, -ry, 0.0, 0.0);
            EmitVertex();

            gl_Position = gl_in[0].gl_Position + vec4(rx, ry, 0.0, 0.0);
            EmitVertex();

            gl_Position = gl_in[0].gl_Position + vec4(rx, -ry, 0.0, 0.0);
            EmitVertex();

            EndPrimitive();
        }
        )";
        
        std::stringstream fragmentstream;
        fragmentstream << R"(
        #version 460 core
        layout(origin_upper_left, pixel_center_integer) in vec4 gl_FragCoord;
        in vec2 cen;
        out vec4 FragColor;

        uniform float radius; 
        uniform float bound;
        uniform float zoom;

        vec3 color = vec3(1.0, 1.0, 1.0);
        vec3 border = vec3(0.0, 0.0, 0.0);

        float hardstep(float a, float b, float x){
            if(x < a) return 0;
            return 1;
        }

        void main(){
            float bound_zoomed = bound / zoom;
            float radius_zoomed = radius / zoom;
            vec2 diff = gl_FragCoord.xy - cen.xy;
            float dist = length(diff);
            float t = 1.0 - hardstep(radius_zoomed - 2 * bound_zoomed, radius_zoomed - bound_zoomed, dist);
            float t_border = 1.0 - hardstep(radius_zoomed - bound_zoomed, radius_zoomed, dist);
            FragColor = vec4(mix(border, color, t), t_border);
        }
        )";

        _circle_shader.addShader(vertexstream, GL_VERTEX_SHADER);
        _circle_shader.addShader(geometrystream, GL_GEOMETRY_SHADER);
        _circle_shader.addShader(fragmentstream, GL_FRAGMENT_SHADER);
    }

    {
        std::stringstream vertexstream;
        vertexstream << R"(
        #version 460 core
        layout (location = 0) in vec2 vertex_position;

        uniform vec2 resolution;
        uniform vec2 movement;
        uniform float zoom;

        void main(){
            vec2 v = vertex_position + movement;
            gl_Position = vec4(2 * v.x / resolution.x - 1, 1 - 2 * v.y / resolution.y, 0.0, zoom);
        }
        )";

        std::stringstream geometrystream;
        geometrystream << R"(
        #version 460 core
        layout (lines) in;
        layout (triangle_strip, max_vertices = 4) out;

        uniform float width;
        uniform vec2 resolution;
        uniform float zoom;


        void main(){
            float width_zoomed = width;
            vec2 dir = normalize((gl_in[0].gl_Position.xy - gl_in[1].gl_Position.xy) * resolution);
            vec4 offset = vec4(vec2(-dir.y, dir.x) * width_zoomed / resolution, 0.0, 0.0);

            gl_Position = gl_in[1].gl_Position + offset;
            EmitVertex();

            gl_Position = gl_in[1].gl_Position - offset;
            EmitVertex();

            gl_Position = gl_in[0].gl_Position + offset;
            EmitVertex();

            gl_Position = gl_in[0].gl_Position - offset;
            EmitVertex();

            EndPrimitive();
        }
        )";
        std::stringstream fragmentstream;
        fragmentstream << R"(
        #version 460 core
        out vec4 FragColor;

        void main(){
            FragColor = vec4(0.0, 0.0, 0.0, 1.0);
        }
        )";
        _line_shader.addShader(vertexstream, GL_VERTEX_SHADER);
        _line_shader.addShader(geometrystream, GL_GEOMETRY_SHADER);
        _line_shader.addShader(fragmentstream, GL_FRAGMENT_SHADER);
    }    




    {
        auto& vec = _edges.getData();
        vec = edges;
    }

    {
        std::mt19937 rng(0);
        auto& vec = _node_coords.getData();
        auto distr_x = std::uniform_real_distribution<float>(0, window.getWidth());
        auto distr_y = std::uniform_real_distribution<float>(0, window.getHeight());
        vec = std::vector<std::pair<float, float>>(node_count);
        std::generate(vec.begin(), vec.end(), [&](){ return std::make_pair(distr_x(rng), distr_y(rng)); });
        prettifyCoordinates(window);
    }

    _line.bind();
    _edges.bind();
    _node_coords.bind();
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, sizeof(_node_coords.getData()[0]), (GLvoid*)0);  
    glEnableVertexAttribArray(0);

    _circle.bind();
    _node_coords.bind();
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, sizeof(_node_coords.getData()[0]), (GLvoid*)0);  
    glEnableVertexAttribArray(0);
}

std::vector<std::pair<float, float>>& GraphTab::getCoordsVector(){
    return _node_coords.getData();
}

std::vector<std::pair<uint32_t, uint32_t>>& GraphTab::getEdgesVector(){
    return _edges.getData();
}

GraphTab::~GraphTab(){
    std::lock_guard lock(mutating_mutex);
}
