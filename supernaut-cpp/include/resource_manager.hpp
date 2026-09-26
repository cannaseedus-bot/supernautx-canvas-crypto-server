// Phase 7.7: Resource Manager for Containerized S7-MINI
// Loads embedded resources from executable at runtime

#pragma once
#include <windows.h>
#include <string>
#include <vector>
#include <map>
#include <memory>

class ResourceManager {
public:
    static ResourceManager& Instance() {
        static ResourceManager instance;
        return instance;
    }

    // Load resource by ID and return as string
    std::string LoadResource(UINT resourceId, const std::string& resourceType = "RCDATA") {
        HMODULE hModule = GetModuleHandle(NULL);
        HRSRC hRes = FindResource(hModule, MAKEINTRESOURCE(resourceId), resourceType.c_str());
        
        if (!hRes) {
            return "";
        }

        HGLOBAL hGlobal = LoadResource(hModule, hRes);
        if (!hGlobal) {
            return "";
        }

        DWORD size = SizeofResource(hModule, hRes);
        const char* pData = static_cast<const char*>(LockResource(hGlobal));
        
        if (!pData) {
            return "";
        }

        std::string result(pData, size);
        UnlockResource(hGlobal);
        FreeResource(hGlobal);
        
        return result;
    }

    // Load resource as binary data
    std::vector<uint8_t> LoadResourceBinary(UINT resourceId, const std::string& resourceType = "RCDATA") {
        HMODULE hModule = GetModuleHandle(NULL);
        HRSRC hRes = FindResource(hModule, MAKEINTRESOURCE(resourceId), resourceType.c_str());
        
        if (!hRes) {
            return std::vector<uint8_t>();
        }

        HGLOBAL hGlobal = LoadResource(hModule, hRes);
        if (!hGlobal) {
            return std::vector<uint8_t>();
        }

        DWORD size = SizeofResource(hModule, hRes);
        const uint8_t* pData = static_cast<const uint8_t*>(LockResource(hGlobal));
        
        if (!pData) {
            return std::vector<uint8_t>();
        }

        std::vector<uint8_t> result(pData, pData + size);
        UnlockResource(hGlobal);
        FreeResource(hGlobal);
        
        return result;
    }

    // Cache loaded resources
    std::string GetCachedResource(UINT resourceId) {
        auto it = resourceCache_.find(resourceId);
        if (it != resourceCache_.end()) {
            return it->second;
        }
        
        std::string resource = LoadResource(resourceId);
        if (!resource.empty()) {
            resourceCache_[resourceId] = resource;
        }
        
        return resource;
    }

    // Get resource size
    DWORD GetResourceSize(UINT resourceId, const std::string& resourceType = "RCDATA") {
        HMODULE hModule = GetModuleHandle(NULL);
        HRSRC hRes = FindResource(hModule, MAKEINTRESOURCE(resourceId), resourceType.c_str());
        
        if (!hRes) {
            return 0;
        }
        
        return SizeofResource(hModule, hRes);
    }

    // Verify all critical resources are available
    bool VerifyAllResourcesPresent() {
        const UINT criticalResources[] = {
            IDR_MODEL_MANIFEST_WEIGHTS,
            IDR_MODEL_S7_BINARY,
            IDR_MM1_PERSONALITY,
            IDR_BRAIN_BIGRAMS,
            IDR_SEMANTICS_XJSON,
            IDR_FOLDS_TOML,
            IDR_CM1_FOLD_TENSOR
        };

        for (UINT resourceId : criticalResources) {
            if (GetResourceSize(resourceId) == 0) {
                return false;
            }
        }

        return true;
    }

    // Get resource statistics
    struct ResourceStats {
        DWORD totalSize;
        int totalCount;
        std::map<std::string, DWORD> byType;
    };

    ResourceStats GetResourceStatistics() {
        ResourceStats stats = {0, 0, {}};

        // Count critical resources
        const UINT criticalResources[] = {
            IDR_MODEL_MANIFEST_WEIGHTS,
            IDR_MODEL_S7_BINARY,
            IDR_MM1_PERSONALITY,
            IDR_BRAIN_BIGRAMS,
            IDR_BRAIN_TRIGRAMS,
            IDR_SEMANTICS_XJSON,
            IDR_OBJECT_TOML,
            IDR_FOLDS_TOML,
            IDR_CM1_FOLD_TENSOR,
            IDR_ATOMIC_BRAIN_FAST,
            IDR_MICRONAUT_REGISTRY_XJSON,
            IDR_MODEL_API_REGISTRY
        };

        for (UINT resourceId : criticalResources) {
            DWORD size = GetResourceSize(resourceId);
            stats.totalSize += size;
            stats.totalCount++;
        }

        return stats;
    }

    // Resource IDs (match resources.rc)
    static const UINT IDR_MODEL_MANIFEST_WEIGHTS = 101;
    static const UINT IDR_MODEL_MANIFEST = 102;
    static const UINT IDR_MODEL_MERKLE_PROOF = 103;
    static const UINT IDR_MODEL_S7_BINARY = 104;
    static const UINT IDR_MM1_PERSONALITY = 201;
    static const UINT IDR_BRAIN_BIGRAMS = 202;
    static const UINT IDR_BRAIN_TRIGRAMS = 203;
    static const UINT IDR_BRAIN_VOCAB = 204;
    static const UINT IDR_SEMANTICS_XJSON = 301;
    static const UINT IDR_OBJECT_TOML = 302;
    static const UINT IDR_FOLDS_TOML = 303;
    static const UINT IDR_MANIFEST_JSON = 304;
    static const UINT IDR_CM1_FOLD_TENSOR = 401;
    static const UINT IDR_ATOMIC_BRAIN_FAST = 501;
    static const UINT IDR_ATOMIC_BRAIN_DEEP = 502;
    static const UINT IDR_MICRONAUT_REGISTRY_XJSON = 601;
    static const UINT IDR_MODEL_API_REGISTRY = 602;
    static const UINT IDR_ICON_MAIN = 701;

private:
    ResourceManager() = default;
    ~ResourceManager() = default;
    
    ResourceManager(const ResourceManager&) = delete;
    ResourceManager& operator=(const ResourceManager&) = delete;

    std::map<UINT, std::string> resourceCache_;
};
