#include <iostream>
#include <cstdlib>
#include "github_client.h"

int main() {
    const char* token = std::getenv("GITHUB_TOKEN");
    
    if (!token || std::string(token).empty()) {
        std::cout << "❌ No GITHUB_TOKEN found!\n";
        std::cout << "Set it with: export GITHUB_TOKEN=\"ghp_...\"\n";
        return 1;
    }
    
    std::cout << "✅ Token found: " << std::string(token).substr(0, 10) << "...\n";
    
    overwatch::GitHubClient client(token);
    auto rate = client.getRateLimit();
    
    std::cout << "Rate limit: " << rate["rate"]["remaining"] << "/" 
              << rate["rate"]["limit"] << " requests remaining\n";
    
    return 0;
}
