#include <cmath>
#include <vector>
#include <cstdint>

namespace debug {
    
std::vector<std::pair<float, float>> forceDirected(std::vector<std::pair<float, float>> coords, const std::vector<std::pair<uint32_t, uint32_t>> &edges){
    const float tolerance = 0.5;
	const float eps = 1e-9f;
    const float K = 300;

	auto dist = [&](const std::pair<float, float> &a) {
		return std::sqrt(a.first * a.first + a.second * a.second) + eps;
	};
	auto attractive = [&](const std::pair<float, float> &a, float d) {
		return d * d / K;
	};
	auto repulsive = [&](const std::pair<float, float> &a, float d) {
		return K * K / d;
	};
	auto updateStep = [progress = 0](float step, float current_energy, float previous_energy) mutable {
		const float t = 0.9f;
		if (current_energy < previous_energy) {
			progress++;
			if (progress >= 5) {
				progress = 0;
				step /= t;
			}
		} else {
			step *= t;
		}
		return step;
	};
	float energy = 1e12f;
    float step = K;
    for(auto [x0, y0]: coords){
        for(auto [x, y]: coords){
            step = std::max(step, dist({x - x0, y - y0}) * 0.25f);
        }
    }
	while (true) {
		float previous_energy = energy;
		std::vector<std::pair<float, float>> forces(coords.size());
		for (auto [u, v] : edges) {
			if (u == v)
				continue;
			auto dir = std::make_pair(coords[u].first - coords[v].first,
					coords[u].second - coords[v].second);
			float d = dist(dir);
			float a = attractive(dir, d);
			dir.first /= d;
			dir.second /= d;
			forces[u].first -= dir.first * a;
			forces[u].second -= dir.second * a;

			forces[v].first += dir.first * a;
			forces[v].second += dir.second * a;
		}

		for (size_t i = 0; i < coords.size(); i++) {
			auto [x0, y0] = coords[i];
            float origin_d = dist(coords[i]);
            float origin_a = attractive(coords[i], origin_d);
            float origin_r = repulsive(coords[i], origin_d);
            forces[i].first -= (origin_a - origin_r) * x0 / origin_d;
            forces[i].second -= (origin_a - origin_r) * y0 / origin_d;
			for (size_t j = i + 1; j < coords.size(); j++) {
				auto [x1, y1] = coords[j];
				auto dir = std::make_pair(x1 - x0, y1 - y0);

                float d = dist(dir);
				float r = repulsive(dir, d);
                dir.first /= d;
                dir.second /= d;

				forces[j].first += dir.first * r;
				forces[j].second += dir.second * r;

				forces[i].first -= dir.first * r;
				forces[i].second -= dir.second * r;
			}
		}
		energy = 0;
		float mx = 0;
		for (size_t i = 0; i < coords.size(); i++) {
			float d = dist(forces[i]);
			float dx = step * forces[i].first / d;
			float dy = step * forces[i].second / d;
			if (i == 0) {
				mx = dist({dx, dy});
			} else {
				mx = std::max(mx, dist({dx, dy}));
			}
			coords[i].first += dx;
			coords[i].second += dy;
			energy += d;
		}
		if(mx < tolerance){
		    break;
		}
		step = updateStep(step, energy, previous_energy);
	}

	return coords;
}
}
