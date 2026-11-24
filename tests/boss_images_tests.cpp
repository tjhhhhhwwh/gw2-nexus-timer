#include "schedule.h"
#include "boss_images.generated.h"
#include <algorithm>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <iterator>
#include <vector>

int main(int argc, char** argv) {
    if (argc != 2) return 1;
    for (const auto& boss : world_timer::Bosses()) {
        const auto image = std::find_if(world_timer::kBossImages.begin(), world_timer::kBossImages.end(),
            [&](const auto& picture) { return boss.id == picture.id; });
        const auto path = std::filesystem::path(argv[1]) / "assets/bosses" / (boss.id + ".jpg");
        std::ifstream file(path, std::ios::binary);
        const std::vector<unsigned char> original{std::istreambuf_iterator<char>(file), {}};
        if (!file || original.empty() || image == world_timer::kBossImages.end() || image->size != original.size()
            || !std::equal(original.begin(), original.end(), image->data)) {
            std::cerr << "FAIL: embedded picture differs from local JPEG: " << boss.id << '\n';
            return 1;
        }
    }
    std::cout << "PASS: every embedded boss picture matches its copied JPEG byte for byte\n";
}
