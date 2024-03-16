#include "GraphDebugger.h"

int main(){
    std::vector<std::pair<uint32_t, uint32_t>> edges1 = {
        {0, 1},
        {1, 2},
        {2, 4},
        {3, 1}
    };

    std::vector<std::pair<uint32_t, uint32_t>> edges2 = {
        {0, 1},
        {1, 2},
        {2, 4},
        {3, 1},
        {2, 5},
        {1, 5},
        {0, 6}
    };
    // std::shared_ptr<OpenGL::Window> window;
    // std::mutex init_mutex;
    // std::condition_variable cv;
    // std::thread win_thread([&](){
    //     {
    //         std::lock_guard lock(init_mutex);
    //         window = std::make_shared<OpenGL::Window>(800, 600);
    //         cv.notify_all();
    //     }
    //     window->run();
    // });
    // std::unique_lock lock(init_mutex);
    // cv.wait(lock, [&](){return window != nullptr;});
    // window->addTab<GraphTab>(5, edges1, *window);
    // win_thread.join();
    {
        auto G1 = Graph(5, edges1);
        G1.visualize();
        auto G2 = Graph(7, edges2);
        G2.visualize();
    }
}
