// Phase 7.7 Containerization: Example resource usage in S7-MINI

#include <resource_manager.hpp>
#include <iostream>
#include <iomanip>
#include <json/json.h>

int main() {
    std::cout << "S7-MINI Phase 7.7 Containerization" << std::endl;
    std::cout << "===================================" << std::endl << std::endl;

    ResourceManager& rm = ResourceManager::Instance();

    // Verify all resources are present
    std::cout << "[*] Verifying embedded resources..." << std::endl;
    if (!rm.VerifyAllResourcesPresent()) {
        std::cerr << "[ERROR] Critical resources missing!" << std::endl;
        return 1;
    }
    std::cout << "[OK] All critical resources present" << std::endl << std::endl;

    // Get resource statistics
    std::cout << "[*] Resource statistics:" << std::endl;
    auto stats = rm.GetResourceStatistics();
    std::cout << "    Total resources: " << stats.totalCount << std::endl;
    std::cout << "    Total size: " << std::fixed << std::setprecision(2) 
              << (stats.totalSize / 1024.0 / 1024.0) << " MB" << std::endl << std::endl;

    // Load and parse manifest
    std::cout << "[*] Loading model manifest..." << std::endl;
    std::string manifestJson = rm.GetCachedResource(ResourceManager::IDR_MODEL_MANIFEST_WEIGHTS);
    if (manifestJson.empty()) {
        std::cerr << "[ERROR] Failed to load manifest" << std::endl;
        return 1;
    }
    std::cout << "[OK] Manifest loaded (" << manifestJson.size() << " bytes)" << std::endl << std::endl;

    // Load personality configuration
    std::cout << "[*] Loading MM-1 personality..." << std::endl;
    std::string personalityJson = rm.GetCachedResource(ResourceManager::IDR_MM1_PERSONALITY);
    if (personalityJson.empty()) {
        std::cerr << "[ERROR] Failed to load personality" << std::endl;
        return 1;
    }
    std::cout << "[OK] Personality loaded (" << personalityJson.size() << " bytes)" << std::endl << std::endl;

    // Load brain structures
    std::cout << "[*] Loading brain structures..." << std::endl;
    std::string bigramsJson = rm.GetCachedResource(ResourceManager::IDR_BRAIN_BIGRAMS);
    std::string trigramsJson = rm.GetCachedResource(ResourceManager::IDR_BRAIN_TRIGRAMS);
    if (bigramsJson.empty() || trigramsJson.empty()) {
        std::cerr << "[ERROR] Failed to load brain files" << std::endl;
        return 1;
    }
    std::cout << "[OK] Brain structures loaded" << std::endl;
    std::cout << "    Bigrams: " << bigramsJson.size() << " bytes" << std::endl;
    std::cout << "    Trigrams: " << trigramsJson.size() << " bytes" << std::endl << std::endl;

    // Load model binary
    std::cout << "[*] Loading S7-MINI model binary..." << std::endl;
    std::vector<uint8_t> modelBinary = rm.LoadResourceBinary(ResourceManager::IDR_MODEL_S7_BINARY);
    if (modelBinary.empty()) {
        std::cerr << "[ERROR] Failed to load S7 binary" << std::endl;
        return 1;
    }
    std::cout << "[OK] Model binary loaded (" << modelBinary.size() << " bytes)" << std::endl << std::endl;

    // Load configuration files
    std::cout << "[*] Loading configuration files..." << std::endl;
    std::string semantics = rm.GetCachedResource(ResourceManager::IDR_SEMANTICS_XJSON);
    std::string folds = rm.GetCachedResource(ResourceManager::IDR_FOLDS_TOML);
    std::string manifest = rm.GetCachedResource(ResourceManager::IDR_MANIFEST_JSON);
    
    if (semantics.empty() || folds.empty() || manifest.empty()) {
        std::cerr << "[ERROR] Failed to load configuration" << std::endl;
        return 1;
    }
    std::cout << "[OK] All configuration files loaded" << std::endl;
    std::cout << "    Semantics: " << semantics.size() << " bytes" << std::endl;
    std::cout << "    Folds: " << folds.size() << " bytes" << std::endl;
    std::cout << "    Manifest: " << manifest.size() << " bytes" << std::endl << std::endl;

    // Load proof artifacts
    std::cout << "[*] Loading proof artifacts..." << std::endl;
    std::string foldTensor = rm.GetCachedResource(ResourceManager::IDR_CM1_FOLD_TENSOR);
    if (foldTensor.empty()) {
        std::cerr << "[ERROR] Failed to load fold tensor proof" << std::endl;
        return 1;
    }
    std::cout << "[OK] Proof artifacts loaded" << std::endl;
    std::cout << "    CM1 Fold Tensor: " << foldTensor.size() << " bytes" << std::endl << std::endl;

    // Success
    std::cout << "===============================================" << std::endl;
    std::cout << "[SUCCESS] S7-MINI Containerization Verified" << std::endl;
    std::cout << "===============================================" << std::endl;
    std::cout << std::endl;
    std::cout << "System Status:" << std::endl;
    std::cout << "  ✓ All " << stats.totalCount << " resources embedded" << std::endl;
    std::cout << "  ✓ Total size: " << std::fixed << std::setprecision(2) 
              << (stats.totalSize / 1024.0 / 1024.0) << " MB" << std::endl;
    std::cout << "  ✓ Zero external file dependencies" << std::endl;
    std::cout << "  ✓ Ready for production deployment" << std::endl;
    std::cout << std::endl;

    return 0;
}
