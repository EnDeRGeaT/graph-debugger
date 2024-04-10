#include "GraphDebugger.h"
#include <algorithm>
#include <cmath>
#include <cstdint>
#include <fcntl.h>
#include <mutex>
#include <numbers>
#include <numeric>
#include <optional>
#include <random>
#include <sstream>
#include <string>
#include <utility>


void GraphTab::prettifyCoordinates(OpenGL::Window& window){
    auto& coords = _node_coords.mutateData();
    float width = static_cast<float>(window.getWidth());
    float height = static_cast<float>(window.getHeight());
    coords = forceDirected(coords, _edges.getData(), {0, width}, {0, height}, _graph_density); // deleted edges are still accounted

    auto& scoords = _string_coords.mutateData();
    for(const auto &index: _available_node_indices.getData()) {
        scoords[_node_labels[index]].coord = coords[index];
    }

    for(const auto& index: _available_edge_indices.getData()){
        updateEdgeLabelPos(index);
    }

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

    const float percent_x = width * 0.90F; 
    const float percent_y = height * 0.90F; 

    _zoom = std::max({(max_x - min_x) / percent_x, (max_y - min_y) / percent_y, _zoom});
}

void GraphTab::processInput(OpenGL::Window& window){
    _input_handler.invoke();
    _input_handler.poll(window.getHandle());
}

void GraphTab::draw(OpenGL::Window& window){

    // EDGE DRAWING
    _node_coords.dump();
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, _node_coords.getID());

    _edges.dump();
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, _edges.getID());

    _edge_properties.dump();
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 2, _edge_properties.getID());

    _available_edge_indices.dump();
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 3, _available_edge_indices.getID());

    _edge_shader.use();
    _edge_shader.setUniform2fv("resolution", {1.0F * static_cast<float>(window.getWidth()), 1.0F * static_cast<float>(window.getHeight())});
    _edge_shader.setUniform2fv("movement", {_movement.first, _movement.second});
    _edge_shader.setUniform1f("zoom", _zoom);
    glDrawArrays(GL_TRIANGLES, 0, 6 * static_cast<int>(_available_edge_indices.getData().size()));
    // glDrawArrays(GL_TRIANGLES, 0, 6 * static_cast<int>(_edges.getData().size()));


    // NODE DRAWING
    _node_properties.dump();
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, _node_properties.getID());
    _available_node_indices.dump();
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 2, _available_node_indices.getID());

    _node_shader.use();
    _node_shader.setUniform2fv("resolution", {1.0F * static_cast<float>(window.getWidth()), 1.0F * static_cast<float>(window.getHeight())});
    _node_shader.setUniform2fv("movement", {_movement.first, _movement.second});
    _node_shader.setUniform1f("bound", _default_node_thickness);
    _node_shader.setUniform1f("zoom", _zoom);
    glDrawArrays(GL_TRIANGLES, 0, 6 * static_cast<int>(_available_node_indices.getData().size()));

    // TEXT DRAWING
    if(_was_mutated){
        _was_mutated = false;
        auto& psum = _string_prefix_sum.mutateData();
        auto& buffer = _string_buffer.mutateData();
        const auto& indices = _available_string_indices.getData();
        psum.resize(indices.size());
        for(size_t i = 0; i < indices.size(); i++){
            psum[i] = static_cast<uint32_t>(_strings[indices[i]].size());
        }
        std::partial_sum(psum.begin(), psum.end(), psum.begin());
        buffer.resize(psum.back());
        auto buffer_it = buffer.begin();
        for(const auto& index: indices){
            const auto& str = _strings[index];
            std::copy(str.begin(), str.end(), buffer_it);
            buffer_it += str.end() - str.begin();
        }
    }


    _string_buffer.dump();
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, _string_buffer.getID());

    _string_properties.dump();
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, _string_properties.getID());

    _string_coords.dump();
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 2, _string_coords.getID());

    _string_prefix_sum.dump();
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 3, _string_prefix_sum.getID());

    _available_string_indices.dump();
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 4, _available_string_indices.getID());

    _string_shader.use();
    _string_shader.setUniform2fv("resolution", {1.0F * static_cast<float>(window.getWidth()), 1.0F * static_cast<float>(window.getHeight())});
    _string_shader.setUniform1f("zoom", _zoom);
    _string_shader.setUniform2fv("sdf_glyph_size", {1.0F * static_cast<float>(sdf_glyph_width), 1.0F * static_cast<float>(sdf_glyph_height)});
    _string_shader.setUniform1ui("letters_in_column", static_cast<uint32_t>(letters_in_column));
    _string_shader.setUniform1f("bearing", static_cast<float>(glyph_advance));
    _string_shader.setUniform2fv("movement", {_movement.first, _movement.second});
    _string_shader.setUniform1ui("strings_size", static_cast<uint32_t>(_available_string_indices.getData().size()));
    
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, _texture_atlas_id);
    glDrawArrays(GL_TRIANGLES, 0, 6 * static_cast<int>(_string_buffer.getData().size()));
};

void GraphTab::addNode(std::pair<int, int> coords, NodeParams properties){
    auto str_coord = StringCoord{ static_cast<int>(StringAlignment::middle_center), 1, coords };
    auto str_property = StringParams{_default_string_color, _default_string_scale};

    if(_deleted_node_indices.empty()) {
        _available_node_indices.mutateData().push_back(static_cast<uint32_t>(_available_node_indices.getData().size()));
        _node_coords.mutateData().emplace_back(coords);
        _node_properties.mutateData().push_back(properties);
        auto str = std::to_string(_node_labels.size());
        _node_labels.push_back(addString(str, str_coord, str_property));
    }
    else {
        uint32_t ind = _deleted_node_indices.back();
        _deleted_node_indices.pop_back();
        _available_node_indices.mutateData().push_back(ind);
        _node_coords.mutateData()[ind] = coords;
        _node_properties.mutateData()[ind] = properties;
        auto str = std::to_string(ind);
        _node_labels[ind] = addString(str, str_coord, str_property);
    }
}

void GraphTab::deleteNode(uint32_t node_index) {
    auto& indices = _available_node_indices.mutateData();
    auto to_del = std::find(indices.begin(), indices.end(), node_index);
    if(to_del == indices.end()) return;
    deleteString(_node_labels[*to_del]);

    std::vector<uint32_t> edges_to_delete;
    for(const auto& edge_index: _available_edge_indices.getData()) {
        const auto& [u, v] = _edges.getData()[edge_index];
        if(u == *to_del || v == *to_del) edges_to_delete.push_back(edge_index);
    }
    for(const auto& x: edges_to_delete) deleteEdge(x);
    _deleted_node_indices.push_back(*to_del);
    indices.erase(to_del);
}

void GraphTab::addEdge(std::pair<uint32_t, uint32_t> edge, EdgeParams properties){
    if(_deleted_edge_indices.empty()) {
        _available_edge_indices.mutateData().push_back(static_cast<uint32_t>(_available_edge_indices.getData().size()));
        _edges.mutateData().push_back(edge);
        _edge_properties.mutateData().push_back(properties);
    }
    else {
        uint32_t ind = _deleted_edge_indices.back();
        _deleted_edge_indices.pop_back();
        _available_edge_indices.mutateData().push_back(ind);
        _edges.mutateData()[ind] = edge;
        _edge_properties.mutateData()[ind] = properties;
    }

    // auto str = std::to_string(_edge_labels.size());
    // auto str_coord = StringCoord{ static_cast<int>(StringAlignment::middle_center), 1, {0, 0} };
    // auto str_property = StringParams{_default_string_color, 0.6f * _default_string_scale};
    // _edge_labels.push_back(addString(str, str_coord, str_property));
    // updateEdgeLabelPos(static_cast<uint32_t>(_edges.getData().size()) - 1);
}

void GraphTab::updateEdgeLabelPos(uint32_t edge_index){ 
    if(_edge_labels.empty()) return;
    auto edge = _edges.getData()[edge_index];
    auto u = _node_coords.getData()[edge.first];
    auto v = _node_coords.getData()[edge.second];

    float angle = std::atan2(u.second - v.second, u.first - v.first);
    if(angle < 0) angle += static_cast<float>(std::numbers::pi);

    if(angle < std::numbers::pi / 2){
        _string_coords.mutateData()[_edge_labels[edge_index]] = {static_cast<int>(StringAlignment::bottom_left), true, {(u.first + v.first) / 2 + 5, (u.second + v.second) / 2}};
    }
    else{
        _string_coords.mutateData()[_edge_labels[edge_index]] = {static_cast<int>(StringAlignment::top_left), true, {(u.first + v.first) / 2 + 5, (u.second + v.second) / 2}};
    }
}

void GraphTab::deleteEdge(uint32_t edge_index) {
    auto& indices = _available_edge_indices.mutateData();
    auto to_del = std::find(indices.begin(), indices.end(), edge_index);
    if(to_del == indices.end()) return;
    _deleted_edge_indices.push_back(*to_del);
    indices.erase(to_del);
}

uint32_t GraphTab::addString(std::string str, StringCoord coordinates, StringParams parameters){
    _was_mutated = true;
    if(_deleted_string_indices.empty()) {
        _available_string_indices.mutateData().push_back(static_cast<uint32_t>(_available_string_indices.getData().size()));
        _strings.push_back(std::move(str));
        _string_properties.mutateData().push_back(parameters);
        _string_coords.mutateData().push_back(coordinates);
        return static_cast<uint32_t>(_strings.size()) - 1;
    }
    else {
        uint32_t ind = _deleted_string_indices.back();
        _deleted_string_indices.pop_back();
        _available_string_indices.mutateData().push_back(ind);
        _strings[ind] = std::move(str);
        _string_properties.mutateData()[ind] = parameters;
        _string_coords.mutateData()[ind] = coordinates;
        return ind;
    }
}

std::string& GraphTab::mutateString(size_t string_index) {
    _was_mutated = true;
    return _strings[string_index];
}

GraphTab::StringCoord& GraphTab::mutateStringCoord(size_t string_index){
    return _string_coords.mutateData()[string_index];
}

GraphTab::StringParams& GraphTab::mutateStringProperty(size_t string_index) {
    return _string_properties.mutateData()[string_index];
}

void GraphTab::deleteString(size_t string_index){
    _was_mutated = true;
    auto& indices = _available_string_indices.mutateData();
    auto to_del = std::find(indices.begin(), indices.end(), string_index);
    if(to_del == indices.end()) return;
    _deleted_string_indices.push_back(*to_del);
    indices.erase(to_del);
}

GraphTab::GraphTab(size_t node_count, const std::vector<std::pair<uint32_t, uint32_t>>& edges, OpenGL::Window& window):
    _empty_vao(),
    _node_shader(),
    _node_coords(GL_SHADER_STORAGE_BUFFER),
    _node_properties(GL_SHADER_STORAGE_BUFFER),
    _available_node_indices(GL_SHADER_STORAGE_BUFFER),
    _default_node_color(0x0),
    _default_node_radius(30),
    _default_node_thickness(5),
    _edge_shader(),
    _edges(GL_SHADER_STORAGE_BUFFER),
    _available_edge_indices(GL_SHADER_STORAGE_BUFFER),
    _edge_properties(GL_SHADER_STORAGE_BUFFER),
    _default_edge_color(0x0),
    _default_edge_thickness(5),
    _string_buffer(GL_SHADER_STORAGE_BUFFER),
    _was_mutated(true),
    _strings(),
    _string_coords(GL_SHADER_STORAGE_BUFFER),
    _string_properties(GL_SHADER_STORAGE_BUFFER),
    _string_prefix_sum(GL_SHADER_STORAGE_BUFFER),
    _available_string_indices(GL_SHADER_STORAGE_BUFFER),
    _default_string_color(0x0),
    _default_string_scale(1.0),
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

        layout (std430, binding=1) buffer node_parameters {
            NodeParams parameters[];
        };

        layout (std430, binding=2) buffer node_available {
            uint node_indices[];
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
            uint node_id = node_indices[gl_VertexID / 6];
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

        layout (std430, binding=2) buffer edge_parameters {
            EdgeParams parameters[];
        };

        layout (std430, binding=3) buffer available_edges {
            uint edge_indices[];
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

        void main() {
            uint edge_id = edge_indices[gl_VertexID / 6];
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

        void main() {
            FragColor.rgb = color;
            FragColor.a = 1.0;
        }
        )";
        _edge_shader.addShader(vertexstream, GL_VERTEX_SHADER);
        _edge_shader.addShader(fragmentstream, GL_FRAGMENT_SHADER);
    }    

    {
        std::stringstream vertexstream;
        vertexstream << R"(
        #version 460 core
        struct StringParams{
            uint color;
            float scale;
        };

        struct StringCoord{
            int alignment;
            int is_affected_by_movement;
            vec2 coord;
        };
        
        layout (std430, binding=0) buffer strings {
            uint chars[];
        };

        layout (std430, binding=1) buffer string_properties {
            StringParams parameters[];
        };

        layout (std430, binding=2) buffer string_coords {
            StringCoord coordinates[];
        };
        
        layout (std430, binding=3) buffer string_psum {
            uint prefix_sum[];
        };

        layout (std430, binding=4) buffer string_available {
            uint string_indices[];
        };

        uniform vec2 resolution;
        uniform float zoom;

        uniform vec2 sdf_glyph_size;
        uniform uint letters_in_column;
        uniform float bearing;

        uniform vec2 movement;
        uniform uint strings_size;

        vec2 alignCoords(vec2 size, uint string_index){
            return coordinates[string_index].coord + 
                    movement * coordinates[string_index].is_affected_by_movement -
                    vec2(coordinates[string_index].alignment % 3, coordinates[string_index].alignment / 3) * size / 2;
        }

        uint getStringIndex(uint char_id){
            uint lo = 0, hi = strings_size;
            while(lo + 1 < hi){
                uint mid = (lo + hi) / 2;
                if(prefix_sum[mid - 1] <= char_id){
                    lo = mid;
                }
                else{
                    hi = mid;
                }
            }
            return lo;
        }

        out vec2 texCoords;
        out vec3 color;

        const vec2 direction[6] = {
            {0.0, 0.0},
            {0.0, 1.0},
            {1.0, 0.0},
            {0.0, 1.0},
            {1.0, 1.0},
            {1.0, 0.0}
        };

        void main() {
            uint char_id = gl_VertexID / 6;
            uint vertice_id = gl_VertexID % 6;

            uint string_index = string_indices[getStringIndex(char_id)];

            vec2 leftTex = vec2((chars[char_id] - 32) % letters_in_column, (chars[char_id] - 32) / letters_in_column) * sdf_glyph_size;
            uint prev = (string_index > 0 ? prefix_sum[string_index - 1] : 0);
            uint sz = prefix_sum[string_index] - prev; 
            vec2 left = alignCoords(vec2(bearing * sz, sdf_glyph_size.y - 14) * parameters[string_index].scale, string_index) + vec2(bearing * parameters[string_index].scale * (char_id - prev), 0.0);

            color = vec3(parameters[string_index].color & 255, (parameters[string_index].color >> 8) & 255, (parameters[string_index].color >> 16) & 255) / 255;

            vec2 sq = sdf_glyph_size * parameters[string_index].scale;
            
            gl_Position.xy = left + sq * direction[vertice_id];
            texCoords = leftTex + sdf_glyph_size * direction[vertice_id];

            gl_Position = vec4(2 * gl_Position.x / resolution.x - 1, 1 - 2 * gl_Position.y / resolution.y, 0.0, zoom);
        }
        )";
        
        std::stringstream fragmentstream;
        fragmentstream << R"(
        #version 460 core
        in vec2 texCoords;
        out vec4 FragColor;

        uniform sampler2D text;
        in vec3 color;

        const float SDFsize = 40;

        void main() {    
            vec2 coords = texCoords / textureSize(text, 0);
            float dist = (0.5 - texture(text, coords).r) * SDFsize;

            vec2 duv = fwidth(coords);

            float dtex = length(duv * textureSize(text, 0));

            float pixelDist = dist * 2 / dtex;

            FragColor = vec4(color, clamp(0.5 - pixelDist, 0.0, 1.0));
        }
        )";

        _string_shader.addShader(vertexstream, GL_VERTEX_SHADER);
        _string_shader.addShader(fragmentstream, GL_FRAGMENT_SHADER);
    }

    std::mt19937 rng(0);
    auto distr_x = std::uniform_real_distribution<float>(0, static_cast<float>(window.getWidth()));
    auto distr_y = std::uniform_real_distribution<float>(0, static_cast<float>(window.getHeight()));
    for(size_t i = 0; i < node_count; i++){
        addNode(std::make_pair(distr_x(rng), distr_y(rng)), {_default_node_color, _default_node_radius});
    }

    for(const auto& edge: edges) {
        addEdge(edge, {_default_edge_color, _default_edge_thickness});
    }

    prettifyCoordinates(window);

    glGenTextures(1, &_texture_atlas_id);
    glBindTexture(GL_TEXTURE_2D, _texture_atlas_id);
    glTexImage2D(
            GL_TEXTURE_2D,
            0,
            GL_RED,
            texture_atlas_width,
            texture_atlas_height,
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

    struct MouseInput : public OpenGL::InputHandler::BaseTwoState {
        OpenGL::InputHandler& input;
        GraphTab& tab;
        OpenGL::Window& window;
        bool left_mouse_button_pressed = false;
        bool right_mouse_button_pressed = false;
        double previous_cursor_x = 0, previous_cursor_y = 0;
        uint32_t index = 0;
        std::optional<size_t> first_highlighted = std::nullopt;
        MouseInput(OpenGL::InputHandler& input_handler, GraphTab& assoc_tab, OpenGL::Window& win) :
            input(input_handler),
            tab(assoc_tab),
            window(win)
        {}

        virtual void perform(std::pair<double, double> cursor){
            int left = input.getMouseKeyState(GLFW_MOUSE_BUTTON_LEFT);
            int right = input.getMouseKeyState(GLFW_MOUSE_BUTTON_RIGHT);

            auto [cursor_x, cursor_y] = cursor;
            float cursor_dx = static_cast<float>(cursor_x - previous_cursor_x), cursor_dy = static_cast<float>(cursor_y - previous_cursor_y);
            previous_cursor_x = cursor_x;
            previous_cursor_y = cursor_y;

            cursor_dx *= tab._zoom;
            cursor_dy *= tab._zoom;

            int width = window.getWidth();
            int height = window.getHeight();

            cursor_x -= width / 2.;
            cursor_y -= height / 2.;

            cursor_x *= tab._zoom;
            cursor_y *= tab._zoom;

            cursor_x -= tab._movement.first;
            cursor_y -= tab._movement.second;

            cursor_x += width / 2.;
            cursor_y += height / 2.;

            const auto& coords = tab._node_coords.getData();
            const auto& available_nodes = tab._available_node_indices.getData();

            auto getClickedNode = [rsqr = tab._default_node_radius * tab._default_node_radius, &coords, &available_nodes, this](double cx, double cy) -> uint32_t {
                for(const auto& ind: available_nodes){
                    const auto &[x, y] = coords[ind];
                    double dx = x - cx;
                    double dy = y - cy;
                    if(dx * dx + dy * dy <= rsqr / tab._zoom){
                        return ind;
                    }
                }
                return static_cast<uint32_t>(coords.size());
            };

            if(left == GLFW_PRESS){
                if(!left_mouse_button_pressed){
                    index = getClickedNode(cursor_x, cursor_y);
                    left_mouse_button_pressed = true;
                }
                else {
                    if(index < coords.size()){
                        auto &mutable_coords = tab._node_coords.mutateData();
                        mutable_coords[index].first += cursor_dx;
                        mutable_coords[index].second += cursor_dy;
                        tab._string_coords.mutateData()[tab._node_labels[index]].coord = mutable_coords[index];
                        for(const auto& edge_index: tab._available_edge_indices.getData()){
                            tab.updateEdgeLabelPos(edge_index);
                        }
                    }
                    else{
                        tab._movement.first += cursor_dx;
                        tab._movement.second += cursor_dy;
                    }
                }
            }
            else if(left == GLFW_RELEASE){
                left_mouse_button_pressed = false;
            }

            if(right == GLFW_PRESS){
                if(!right_mouse_button_pressed){
                    size_t v = getClickedNode(cursor_x, cursor_y);
                    if(v != coords.size()){
                        if(first_highlighted.has_value()){
                            tab.addEdge({*first_highlighted, v}, {tab._default_edge_color, tab._default_edge_thickness});
                            tab._node_properties.mutateData()[*first_highlighted].color = tab._default_node_color;
                            first_highlighted = std::nullopt;
                        }
                        else{
                            first_highlighted = v;
                            tab._node_properties.mutateData()[v].color = 0x00FF00;
                        }
                    }
                    else{
                        tab.addNode({cursor_x, cursor_y}, {tab._default_node_color, tab._default_node_radius});
                        if(first_highlighted.has_value()) tab._node_properties.mutateData()[*first_highlighted].color = tab._default_node_color;
                        first_highlighted = std::nullopt;
                    }
                    right_mouse_button_pressed = true;
                }
            }
            else if(right == GLFW_RELEASE){
                right_mouse_button_pressed = false;
            }
        }
    };


    struct FKEY : public OpenGL::InputHandler::BaseKey {
        OpenGL::InputHandler& input;
        GraphTab& tab;
        OpenGL::Window& window;
        bool f_pressed = false;
        FKEY(OpenGL::InputHandler& input_handler, GraphTab& assoc_tab, OpenGL::Window& win) :
            input(input_handler),
            tab(assoc_tab),
            window(win)
        {}

        virtual void perform(int key){
            if(key == GLFW_PRESS){
                if(!f_pressed && tab._node_coords.getData().size()){
                    tab.prettifyCoordinates(window);
                }
                f_pressed = true;
            }
            else if(key == GLFW_RELEASE){
                f_pressed = false;
            }
        }
    };
    

    struct CKEY : public OpenGL::InputHandler::BaseKey {
        OpenGL::InputHandler& input;
        GraphTab& tab;
        OpenGL::Window& window;
        CKEY(OpenGL::InputHandler& input_handler, GraphTab& assoc_tab, OpenGL::Window& win) :
            input(input_handler),
            tab(assoc_tab),
            window(win)
        {}

        virtual void perform(int key){
            if(key == GLFW_PRESS){
                std::cout << "Please input the new density value: ";
                std::cin >> tab._graph_density;
            }
        }
    };


    struct PLUSKEY : public OpenGL::InputHandler::BaseKey {
        OpenGL::InputHandler& input;
        GraphTab& tab;
        OpenGL::Window& window;
        PLUSKEY(OpenGL::InputHandler& input_handler, GraphTab& assoc_tab, OpenGL::Window& win) :
            input(input_handler),
            tab(assoc_tab),
            window(win)
        {}

        virtual void perform(int key){
            if(key == GLFW_PRESS){
                tab._zoom *= 0.95f;
            }
        }
    };

    struct MINUSKEY : public OpenGL::InputHandler::BaseKey {
        OpenGL::InputHandler& input;
        GraphTab& tab;
        OpenGL::Window& window;
        MINUSKEY(OpenGL::InputHandler& input_handler, GraphTab& assoc_tab, OpenGL::Window& win) :
            input(input_handler),
            tab(assoc_tab),
            window(win)
        {}

        virtual void perform(int key){
            if(key == GLFW_PRESS){
                tab._zoom /= 0.95f;
            }
        }
    };

    struct DKEY : public OpenGL::InputHandler::BaseKey {
        OpenGL::InputHandler& input;
        GraphTab& tab;
        OpenGL::Window& window;
        bool d_pressed = false;
        DKEY(OpenGL::InputHandler& input_handler, GraphTab& assoc_tab, OpenGL::Window& win) :
            input(input_handler),
            tab(assoc_tab),
            window(win)
        {}

        virtual void perform(int key){
            if(key && d_pressed == false) {
                auto last = tab._available_node_indices.getData().back();
                tab.deleteNode(last);
            }
            d_pressed = key;
        }
    };

    _input_handler.attachMousePos(std::make_unique<MouseInput>(_input_handler, *this, window));
    _input_handler.attachKey(GLFW_KEY_F, std::make_unique<FKEY>(_input_handler, *this, window));
    _input_handler.attachKey(GLFW_KEY_C, std::make_unique<CKEY>(_input_handler, *this, window));
    _input_handler.attachKey(GLFW_KEY_EQUAL, std::make_unique<PLUSKEY>(_input_handler, *this, window));
    _input_handler.attachKey(GLFW_KEY_MINUS, std::make_unique<MINUSKEY>(_input_handler, *this, window));
    _input_handler.attachKey(GLFW_KEY_D, std::make_unique<DKEY>(_input_handler, *this, window));
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
