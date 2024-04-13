#include "GraphDebugger.h"
#include <algorithm>
#include <memory>
#include <mutex>
#include <optional>
#include <string>
#include <utility>
#include <vector>
#include <queue>

std::shared_ptr<OpenGL::Window> Graph::_window = nullptr;

Graph::Graph(uint32_t vertice_count, const std::vector<std::pair<uint32_t, uint32_t>>& edges, bool is_directed):
    _sz(vertice_count),
    _is_directed(is_directed), 
    _edges({}),
    _weights({}),
    _adj_list({})
{
    _edges = edges;
    if(_is_directed == false) {
        for (auto &[x, y] : _edges) {
            if(x > y) std::swap(x, y);
        }
    }
    _adj_list.assign(_sz, {});
    std::sort(_edges.begin(), _edges.end());
    int i = 0;
    for(auto &[x, y]: _edges){
        _adj_list[x].emplace_back(y, i);
        if(_is_directed == false) _adj_list[y].emplace_back(x, i);
        i++;
    }
}

Graph::Graph(uint32_t vertice_count, std::vector<std::tuple<uint32_t, uint32_t, int32_t>> edges, bool is_directed):
    _sz(vertice_count),
    _is_directed(is_directed), 
    _edges({}),
    _weights({}),
    _adj_list({})    
{
    for(auto [x, y, w]: edges){
        if(x > y && is_directed == false) std::swap(x, y);
    }
    std::sort(edges.begin(), edges.end());
    _edges.reserve(_sz);
    _weights.reserve(_sz);
    _adj_list.assign(_sz, {});
    int i = 0;
    for(auto &[x, y, w]: edges){
        _adj_list[x].emplace_back(y, i);
        if(_is_directed == false) _adj_list[y].emplace_back(x, i);
        _edges.emplace_back(x, y);
        _weights.push_back(w);
        i++;
    }
}

Graph::~Graph(){
    if(_running_thread.joinable()){
        std::cerr << "Waiting on window to close...\n";
        std::cerr.flush();
        _running_thread.join();
    } 
}

void Graph::visualize(const std::vector<uint32_t>& node_colors, const std::vector<uint32_t>& edge_colors, const std::vector<std::pair<float, float>>& coords, const std::vector<float>& line_widths, const std::vector<std::string>& node_labels) {
    std::unique_lock lck(_window_init_mutex);
    if(_window == nullptr){
        std::condition_variable cv;
        _running_thread = std::thread([&](){
            {
                std::lock_guard lock(_window_init_mutex);
                _window = std::make_shared<OpenGL::Window>(800, 600);
                cv.notify_all();
            }
            _window->run();
            _window.reset();
        });
        cv.wait(lck, [&](){return _window != nullptr;});
    }
    if(_associated_tab.expired()){
        _associated_tab = _window->addTab<GraphTab>(_sz, _edges, *_window);
    }

    auto tab = _associated_tab.lock();

    if(coords.size() == _sz){
        tab->setNodeCoords(coords);
    }
    if(_weights.size()){
        std::vector<std::string> str_weights;
        std::transform(_weights.begin(), _weights.end(), std::back_inserter(str_weights), [](int32_t x) { return std::to_string(x); });
        tab->setEdgeLabels(str_weights);
    }
    if(node_colors.size() == _sz){
        tab->setNodeColors(node_colors);
    }
    if(edge_colors.size() == _sz){
        tab->setEdgeColors(edge_colors);
    }
    if(line_widths.size() == _edges.size()){
        tab->setEdgesThickness(line_widths);
    }
    if(node_labels.size() == _sz){
        tab->setEdgeLabels(node_labels);
    }
}

void Graph::visualizeWithHighlightedEdges(const std::vector<std::pair<uint32_t, uint32_t>>& edges){
    std::vector<uint32_t> colors(_edges.size(), 0xEAD8C0);
    for(auto [x, y]: edges){
        if(_is_directed == false && x > y) std::swap(x, y);
        auto xy = std::make_pair(x, y);
        uint32_t ind = std::lower_bound(_edges.begin(), _edges.end(), xy) - _edges.begin();
        if(ind != _edges.size() && _edges[ind] == xy){
            colors[ind] = 0x0; 
        }
    }
    visualize({}, colors);
}


uint32_t Graph::getNodes(){
    return _sz;
} 

std::vector<std::pair<uint32_t, uint32_t>> Graph::getEdges(){
    return _edges;
}

std::vector<std::vector<std::optional<int32_t>>> Graph::getAdjacencyMatrix(){
    std::vector<std::vector<std::optional<int32_t>>> res(_sz, std::vector<std::optional<int32_t>>(_sz, std::nullopt));
    if(_weights.empty()){
        for(uint32_t v = 0; v < _sz; v++){
            for(const auto& [u, i]: _adj_list[v]) res[v][u] = 1;
        }
    }
    else{
        for(uint32_t v = 0; v < _sz; v++){
            for(const auto& [u, i]: _adj_list[v]) res[v][u] = _weights[i];
        }
    }
    return res;
}

std::vector<std::optional<int32_t>> Graph::findDistancesFromNode(uint32_t starting_node){
    std::vector<std::optional<int32_t>> dist(_sz, std::nullopt);
    if(_weights.empty()){
        dist[starting_node] = 0;
        auto F = [&dist](uint32_t v, uint32_t u, uint32_t index, const Graph& g){
            dist[u] = *dist[v] + 1;
        };
        bfs(starting_node, F);
    }
    else{
        std::vector<char> used(_sz);
        std::priority_queue<std::pair<int32_t, uint32_t>, std::vector<std::pair<int32_t, uint32_t>>, std::greater<std::pair<int32_t, uint32_t>>> pq;
        dist[starting_node] = 0;
        pq.push({0, starting_node});
        while(!pq.empty()){
            auto [c, v] = pq.top();
            pq.pop();
            if(used[v]) continue;
            used[v] = 1;
            for(auto [u, i]: _adj_list[v]){
                if(used[u]) continue;
                if(dist[u].has_value() == false || c + _weights[i] < *dist[u]){
                    dist[u] = c + _weights[i];
                    pq.push({*dist[u], u});
                }
            }
        }
    }
    return dist;
}

std::optional<std::vector<uint32_t>> Graph::findShortestPathBetweenNodes(uint32_t start, uint32_t finish){
    std::vector<std::optional<int32_t>> dist(_sz, std::nullopt);
    std::vector<std::optional<uint32_t>> parent(_sz, std::nullopt);
    if(_weights.empty()){
        dist[start] = 0;
        auto F = [&dist, &parent](uint32_t v, uint32_t u, uint32_t index, const Graph& g){
            dist[u] = *dist[v] + 1;
            parent[u] = v;
        };
        bfs(start, F);
    }
    else{
        std::vector<char> used(_sz);
        std::priority_queue<std::pair<int32_t, uint32_t>, std::vector<std::pair<int32_t, uint32_t>>, std::greater<std::pair<int32_t, uint32_t>>> pq;
        dist[start] = 0;
        pq.push({0, start});
        while(!pq.empty()){
            auto [c, v] = pq.top();
            pq.pop();
            if(used[v]) continue;
            used[v] = 1;
            for(auto [u, i]: _adj_list[v]){
                if(used[u]) continue;
                if(dist[u].has_value() == false || c + _weights[i] < *dist[u]){
                    dist[u] = c + _weights[i];
                    parent[u] = v;
                    pq.push({*dist[u], u});
                }
            }
        }
    }
    std::optional<std::vector<uint32_t>> path;
    if(!parent[finish].has_value()) return path;
    path = std::vector<uint32_t>();
    path->push_back(finish);
    while(parent[path->back()].has_value()){
        path->push_back(*parent[path->back()]);
    }
    std::reverse(path->begin(), path->end());
    return path;
}
 
std::vector<std::vector<std::optional<int32_t>>> Graph::findPairwiseDistances(){
    if(_weights.empty()){
        std::vector<std::vector<std::optional<int32_t>>> dist(_sz);
        for(uint32_t start = 0; start < _sz; start++){
            dist[start] = findDistancesFromNode(start);
        }
        return dist;
    }
    auto res = getAdjacencyMatrix();
    for(uint32_t i = 0; i < _sz; i++){
        res[i][i] = 0;
    }
    for(uint32_t mid = 0; mid < _sz; mid++){
        for(uint32_t v = 0; v < _sz; v++){
            if(!res[v][mid].has_value()) continue;
            for(uint32_t u = 0; u < _sz; u++){
                if(u == v || !res[mid][u].has_value()) continue;
                if(res[v][u].has_value()) res[v][u] = std::min(*res[v][u], *res[v][mid] + *res[mid][u]);
                else res[v][u] = *res[v][mid] + *res[mid][u];
            }
        }
    }
    return res;
}


std::optional<std::vector<uint32_t>> Graph::findHamiltonianCycle(){
    auto masks = std::vector(1 << (_sz - 1), std::vector(_sz - 1, std::optional<std::pair<uint32_t, uint32_t>>(std::nullopt)));
    auto adj_mat = getAdjacencyMatrix();
    for(uint32_t v = 0; v + 1 < _sz; v++){
        if(adj_mat[_sz - 1][v].has_value()) masks[0][v] = {0, _sz - 1};
    }
    for(uint32_t mask = 0; mask < masks.size(); mask++){
        for(uint32_t u = 0; u + 1 < _sz; u++){
            if(masks[mask][u].has_value() == false || (mask >> u & 1)) continue;
            for(uint32_t v = 0; v + 1 < _sz; v++){
                if((mask >> v & 1) || u == v) continue;
                if(adj_mat[u][v].has_value()){
                    masks[mask | (1 << u)][v] = {mask, u};
                }
            }
        }
    }
    uint32_t big = static_cast<uint32_t>(masks.size()) - 1;
    for(uint32_t v = 0; v + 1 < _sz; v++){
        uint32_t mask = big ^ (1 << v);
        if(masks[mask][v].has_value() && adj_mat[v][_sz - 1].has_value()){
            std::vector<uint32_t> path;
            path.push_back(_sz - 1);
            while(v != _sz - 1){
                path.push_back(v);
                auto [nmask, nv] = *masks[mask][v];
                mask = nmask;
                v = nv;
            }
            return path;
        }
    }
    return std::nullopt;
}


std::optional<std::vector<uint32_t>> Graph::findEulerianCircuit(){
    std::vector<uint32_t> deg(_sz);
    for(const auto& [x, y]: _edges){
        deg[x] ^= 1;
        deg[y] ^= 1;
    }
    std::optional<std::vector<uint32_t>> path;
    if(std::count(deg.begin(), deg.end(), 1)){
        return std::nullopt;
    }
    path = std::vector<uint32_t>();
    std::vector<char> used_edges(_edges.size());
    std::vector<uint32_t> adj_indexes(_sz);

    auto dfs = [this, &path, &used_edges, &adj_indexes](auto& self, uint32_t v) -> void {
        for(; adj_indexes[v] < _adj_list[v].size(); adj_indexes[v]++){
            auto [u, ind] = _adj_list[v][adj_indexes[v]];
            if(used_edges[ind]) continue;
            used_edges[ind] = 1;
            self(self, u);
        }
        (*path).push_back(v);
    };

    dfs(dfs, 0);
    return path;
}


std::vector<uint32_t> Graph::findMinimumSpanningTree(){
    std::priority_queue<std::pair<int32_t, uint32_t>, std::vector<std::pair<int32_t, uint32_t>>, std::greater<>> pq;
    std::vector<char> in_tree(_sz);
    std::vector<uint32_t> MST;
    in_tree[0] = 1;
    for(auto [s, j]: _adj_list[0]){
        pq.push({_weights.empty() ? 1 : _weights[j], j});
    }
    while(!pq.empty()){
        auto [w, ind] = pq.top();
        pq.pop();
        auto [u, v] = _edges[ind];
        if(in_tree[u] > in_tree[v]) std::swap(u, v);
        if(in_tree[u]) continue;
        in_tree[u] = 1;
        MST.push_back(ind);
        for(auto [s, j]: _adj_list[u]){
            if(in_tree[s]) continue;
            pq.push({_weights.empty() ? 1 : _weights[j], j});
        }
    }
    return MST;
}


