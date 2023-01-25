#include <fstream>
#include <thread>
#include <vector>
#include <string>

int main() {
	std::vector<std::thread> v{};
	for(int i = 0; i < 5; ++i) {
		v.emplace_back([i]() noexcept -> void {
			std::string name = "file_id=" + std::to_string(i) + ".txt";
			std::ofstream file{name};
			file << "ogo";
			file.flush();
			file.close();
		});
	}

	for(auto & th: v) {
		th.join();
	}

	return 0;
}
