#include <cstdint>
#include <cstring>
#include <vector>
#include <string>
#include <fstream>
#include <iostream>
#include <iomanip>
#include <sstream>
#include <algorithm>
#include <openssl/sha.h>

class SCXQ2CanonicalPacker {
public:
    static constexpr uint32_t MAGIC = 0x51515153;  // "SCQQ"
    static constexpr uint8_t VERSION = 0x02;

    // Adler-32 checksum (same as Python implementation)
    static uint32_t adler32(const uint8_t* data, size_t len) {
        uint32_t a = 1, b = 0;
        for (size_t i = 0; i < len; ++i) {
            a = (a + data[i]) % 65521;
            b = (b + a) % 65521;
        }
        return (b << 16) | a;
    }

    // SHA-256 hash (for manifest determinism field)
    static std::string sha256(const uint8_t* data, size_t len) {
        unsigned char hash[SHA256_DIGEST_LENGTH];
        SHA256_CTX sha256;
        SHA256_Init(&sha256);
        SHA256_Update(&sha256, data, len);
        SHA256_Final(hash, &sha256);

        std::stringstream ss;
        for (int i = 0; i < SHA256_DIGEST_LENGTH; i++) {
            ss << std::hex << std::setw(2) << std::setfill('0') << (int)hash[i];
        }
        return ss.str();
    }

    // Pack a single lane (ID + length + count + data + CRC32)
    static std::vector<uint8_t> pack_lane(uint16_t lane_id, const uint8_t* data, size_t data_len, uint32_t record_count) {
        std::vector<uint8_t> lane;
        
        // Lane ID (2 bytes, little-endian)
        lane.push_back(lane_id & 0xFF);
        lane.push_back((lane_id >> 8) & 0xFF);
        
        // Lane length = 4 (CRC32) + data_len (little-endian)
        uint32_t lane_len = 4 + data_len;
        lane.push_back(lane_len & 0xFF);
        lane.push_back((lane_len >> 8) & 0xFF);
        lane.push_back((lane_len >> 16) & 0xFF);
        lane.push_back((lane_len >> 24) & 0xFF);
        
        // Record count (4 bytes, little-endian)
        lane.push_back(record_count & 0xFF);
        lane.push_back((record_count >> 8) & 0xFF);
        lane.push_back((record_count >> 16) & 0xFF);
        lane.push_back((record_count >> 24) & 0xFF);
        
        // Data
        if (data_len > 0) {
            lane.insert(lane.end(), data, data + data_len);
        }
        
        // CRC32 (Adler-32)
        uint32_t crc = adler32(data, data_len);
        lane.push_back(crc & 0xFF);
        lane.push_back((crc >> 8) & 0xFF);
        lane.push_back((crc >> 16) & 0xFF);
        lane.push_back((crc >> 24) & 0xFF);
        
        return lane;
    }

    // Generate empty vector
    static std::vector<uint8_t> generate_vector_empty() {
        std::vector<uint8_t> binary;
        
        // Header (12 bytes)
        binary.push_back(MAGIC & 0xFF);
        binary.push_back((MAGIC >> 8) & 0xFF);
        binary.push_back((MAGIC >> 16) & 0xFF);
        binary.push_back((MAGIC >> 24) & 0xFF);
        binary.push_back(VERSION);
        binary.push_back(0x00);  // reserved
        binary.push_back(0x00);  // flags (low byte)
        binary.push_back(0x00);  // flags (high byte)
        binary.push_back(0x00);  // total_len placeholder
        binary.push_back(0x00);
        binary.push_back(0x00);
        binary.push_back(0x00);
        
        // 5 empty lanes
        for (uint16_t lane_id = 0x0001; lane_id <= 0x0005; ++lane_id) {
            auto lane = pack_lane(lane_id, nullptr, 0, 0);
            binary.insert(binary.end(), lane.begin(), lane.end());
        }
        
        // Manifest JSON
        std::string manifest = R"({"@kind":"scxq2-canonical-v1.2","@version":"1.0","total_records":0,"created_tick":0,"creator":"SCXQ2CanonicalPacker.cpp","determinism":{"canonical_sorted":true,"hash_value":")" 
                             + sha256(binary.data(), binary.size()) + R"("}})";
        
        std::vector<uint8_t> manifest_bytes(manifest.begin(), manifest.end());
        binary.insert(binary.end(), manifest_bytes.begin(), manifest_bytes.end());
        
        // Footer (8 bytes)
        uint32_t manifest_len = manifest_bytes.size();
        binary.push_back(manifest_len & 0xFF);
        binary.push_back((manifest_len >> 8) & 0xFF);
        binary.push_back((manifest_len >> 16) & 0xFF);
        binary.push_back((manifest_len >> 24) & 0xFF);
        
        uint32_t manifest_crc = adler32(manifest_bytes.data(), manifest_bytes.size());
        binary.push_back(manifest_crc & 0xFF);
        binary.push_back((manifest_crc >> 8) & 0xFF);
        binary.push_back((manifest_crc >> 16) & 0xFF);
        binary.push_back((manifest_crc >> 24) & 0xFF);
        
        // Fix total_len in header
        uint32_t total_len = binary.size();
        binary[8] = total_len & 0xFF;
        binary[9] = (total_len >> 8) & 0xFF;
        binary[10] = (total_len >> 16) & 0xFF;
        binary[11] = (total_len >> 24) & 0xFF;
        
        return binary;
    }

    // Generate simple vector
    static std::vector<uint8_t> generate_vector_simple() {
        std::vector<uint8_t> binary;
        
        // Header
        binary.push_back(MAGIC & 0xFF);
        binary.push_back((MAGIC >> 8) & 0xFF);
        binary.push_back((MAGIC >> 16) & 0xFF);
        binary.push_back((MAGIC >> 24) & 0xFF);
        binary.push_back(VERSION);
        binary.push_back(0x00);
        binary.push_back(0x00);
        binary.push_back(0x00);
        binary.push_back(0x00);
        binary.push_back(0x00);
        binary.push_back(0x00);
        binary.push_back(0x00);
        
        // DICT lane: two symbols
        const uint8_t dict_data[] = "symbol_a\0symbol_b\0";
        auto dict_lane = pack_lane(0x0001, dict_data, sizeof(dict_data) - 1, 2);
        binary.insert(binary.end(), dict_lane.begin(), dict_lane.end());
        
        // FIELD lane: two fields
        uint8_t field_data[16];
        std::memcpy(field_data, "\x01\x00\x00\x00\x02\x00\x00\x00timestamp", 16);
        auto field_lane = pack_lane(0x0002, field_data, 16, 1);
        binary.insert(binary.end(), field_lane.begin(), field_lane.end());
        
        // LANE lane: one event
        uint8_t lane_data[14];
        std::memcpy(lane_data, "\x00\x00\x00\x00\x01\x00event_data", 14);
        auto lane_obj = pack_lane(0x0003, lane_data, 14, 1);
        binary.insert(binary.end(), lane_obj.begin(), lane_obj.end());
        
        // EDGE lane: one edge
        uint8_t edge_data[12];
        std::memcpy(edge_data, "\x00\x00\x00\x00\x01\x00\x00\x00causality", 12);
        auto edge_lane = pack_lane(0x0004, edge_data, 12, 1);
        binary.insert(binary.end(), edge_lane.begin(), edge_lane.end());
        
        // BATCH lane: empty
        auto batch_lane = pack_lane(0x0005, nullptr, 0, 0);
        binary.insert(binary.end(), batch_lane.begin(), batch_lane.end());
        
        // Manifest
        std::string manifest = R"({"@kind":"scxq2-canonical-v1.2","@version":"1.0","total_records":3,"created_tick":0,"creator":"SCXQ2CanonicalPacker.cpp","determinism":{"canonical_sorted":true,"hash_value":")" 
                             + sha256(binary.data(), binary.size()) + R"("}})";
        
        std::vector<uint8_t> manifest_bytes(manifest.begin(), manifest.end());
        binary.insert(binary.end(), manifest_bytes.begin(), manifest_bytes.end());
        
        // Footer
        uint32_t manifest_len = manifest_bytes.size();
        binary.push_back(manifest_len & 0xFF);
        binary.push_back((manifest_len >> 8) & 0xFF);
        binary.push_back((manifest_len >> 16) & 0xFF);
        binary.push_back((manifest_len >> 24) & 0xFF);
        
        uint32_t manifest_crc = adler32(manifest_bytes.data(), manifest_bytes.size());
        binary.push_back(manifest_crc & 0xFF);
        binary.push_back((manifest_crc >> 8) & 0xFF);
        binary.push_back((manifest_crc >> 16) & 0xFF);
        binary.push_back((manifest_crc >> 24) & 0xFF);
        
        // Fix total_len
        uint32_t total_len = binary.size();
        binary[8] = total_len & 0xFF;
        binary[9] = (total_len >> 8) & 0xFF;
        binary[10] = (total_len >> 16) & 0xFF;
        binary[11] = (total_len >> 24) & 0xFF;
        
        return binary;
    }

    // Generate events vector
    static std::vector<uint8_t> generate_vector_events() {
        std::vector<uint8_t> binary;
        
        // Header
        binary.push_back(MAGIC & 0xFF);
        binary.push_back((MAGIC >> 8) & 0xFF);
        binary.push_back((MAGIC >> 16) & 0xFF);
        binary.push_back((MAGIC >> 24) & 0xFF);
        binary.push_back(VERSION);
        binary.push_back(0x00);
        binary.push_back(0x00);
        binary.push_back(0x00);
        binary.push_back(0x00);
        binary.push_back(0x00);
        binary.push_back(0x00);
        binary.push_back(0x00);
        
        // Empty DICT
        auto dict_lane = pack_lane(0x0001, nullptr, 0, 0);
        binary.insert(binary.end(), dict_lane.begin(), dict_lane.end());
        
        // Empty FIELD
        auto field_lane = pack_lane(0x0002, nullptr, 0, 0);
        binary.insert(binary.end(), field_lane.begin(), field_lane.end());
        
        // LANE: 3 events
        uint8_t lane_data[12];
        for (int i = 0; i < 3; ++i) {
            std::memcpy(lane_data + i * 4, &i, 4);
        }
        auto lane_obj = pack_lane(0x0003, lane_data, 12, 3);
        binary.insert(binary.end(), lane_obj.begin(), lane_obj.end());
        
        // Empty EDGE
        auto edge_lane = pack_lane(0x0004, nullptr, 0, 0);
        binary.insert(binary.end(), edge_lane.begin(), edge_lane.end());
        
        // Empty BATCH
        auto batch_lane = pack_lane(0x0005, nullptr, 0, 0);
        binary.insert(binary.end(), batch_lane.begin(), batch_lane.end());
        
        // Manifest
        std::string manifest = R"({"@kind":"scxq2-canonical-v1.2","@version":"1.0","total_records":3,"created_tick":0,"creator":"SCXQ2CanonicalPacker.cpp","determinism":{"canonical_sorted":true,"hash_value":")" 
                             + sha256(binary.data(), binary.size()) + R"("}})";
        
        std::vector<uint8_t> manifest_bytes(manifest.begin(), manifest.end());
        binary.insert(binary.end(), manifest_bytes.begin(), manifest_bytes.end());
        
        // Footer
        uint32_t manifest_len = manifest_bytes.size();
        binary.push_back(manifest_len & 0xFF);
        binary.push_back((manifest_len >> 8) & 0xFF);
        binary.push_back((manifest_len >> 16) & 0xFF);
        binary.push_back((manifest_len >> 24) & 0xFF);
        
        uint32_t manifest_crc = adler32(manifest_bytes.data(), manifest_bytes.size());
        binary.push_back(manifest_crc & 0xFF);
        binary.push_back((manifest_crc >> 8) & 0xFF);
        binary.push_back((manifest_crc >> 16) & 0xFF);
        binary.push_back((manifest_crc >> 24) & 0xFF);
        
        // Fix total_len
        uint32_t total_len = binary.size();
        binary[8] = total_len & 0xFF;
        binary[9] = (total_len >> 8) & 0xFF;
        binary[10] = (total_len >> 16) & 0xFF;
        binary[11] = (total_len >> 24) & 0xFF;
        
        return binary;
    }

    // Generate edges vector
    static std::vector<uint8_t> generate_vector_edges() {
        std::vector<uint8_t> binary;
        
        // Header
        binary.push_back(MAGIC & 0xFF);
        binary.push_back((MAGIC >> 8) & 0xFF);
        binary.push_back((MAGIC >> 16) & 0xFF);
        binary.push_back((MAGIC >> 24) & 0xFF);
        binary.push_back(VERSION);
        binary.push_back(0x00);
        binary.push_back(0x00);
        binary.push_back(0x00);
        binary.push_back(0x00);
        binary.push_back(0x00);
        binary.push_back(0x00);
        binary.push_back(0x00);
        
        // Empty DICT, FIELD, LANE
        auto dict_lane = pack_lane(0x0001, nullptr, 0, 0);
        binary.insert(binary.end(), dict_lane.begin(), dict_lane.end());
        
        auto field_lane = pack_lane(0x0002, nullptr, 0, 0);
        binary.insert(binary.end(), field_lane.begin(), field_lane.end());
        
        auto lane_obj = pack_lane(0x0003, nullptr, 0, 0);
        binary.insert(binary.end(), lane_obj.begin(), lane_obj.end());
        
        // EDGE: 3 edges
        uint8_t edge_data[24];
        for (int i = 0; i < 3; ++i) {
            uint32_t val1 = i;
            uint32_t val2 = i + 1;
            std::memcpy(edge_data + i * 8, &val1, 4);
            std::memcpy(edge_data + i * 8 + 4, &val2, 4);
        }
        auto edge_lane = pack_lane(0x0004, edge_data, 24, 3);
        binary.insert(binary.end(), edge_lane.begin(), edge_lane.end());
        
        // Empty BATCH
        auto batch_lane = pack_lane(0x0005, nullptr, 0, 0);
        binary.insert(binary.end(), batch_lane.begin(), batch_lane.end());
        
        // Manifest
        std::string manifest = R"({"@kind":"scxq2-canonical-v1.2","@version":"1.0","total_records":3,"created_tick":0,"creator":"SCXQ2CanonicalPacker.cpp","determinism":{"canonical_sorted":true,"hash_value":")" 
                             + sha256(binary.data(), binary.size()) + R"("}})";
        
        std::vector<uint8_t> manifest_bytes(manifest.begin(), manifest.end());
        binary.insert(binary.end(), manifest_bytes.begin(), manifest_bytes.end());
        
        // Footer
        uint32_t manifest_len = manifest_bytes.size();
        binary.push_back(manifest_len & 0xFF);
        binary.push_back((manifest_len >> 8) & 0xFF);
        binary.push_back((manifest_len >> 16) & 0xFF);
        binary.push_back((manifest_len >> 24) & 0xFF);
        
        uint32_t manifest_crc = adler32(manifest_bytes.data(), manifest_bytes.size());
        binary.push_back(manifest_crc & 0xFF);
        binary.push_back((manifest_crc >> 8) & 0xFF);
        binary.push_back((manifest_crc >> 16) & 0xFF);
        binary.push_back((manifest_crc >> 24) & 0xFF);
        
        // Fix total_len
        uint32_t total_len = binary.size();
        binary[8] = total_len & 0xFF;
        binary[9] = (total_len >> 8) & 0xFF;
        binary[10] = (total_len >> 16) & 0xFF;
        binary[11] = (total_len >> 24) & 0xFF;
        
        return binary;
    }

    // Generate full vector
    static std::vector<uint8_t> generate_vector_full() {
        std::vector<uint8_t> binary;
        
        // Header
        binary.push_back(MAGIC & 0xFF);
        binary.push_back((MAGIC >> 8) & 0xFF);
        binary.push_back((MAGIC >> 16) & 0xFF);
        binary.push_back((MAGIC >> 24) & 0xFF);
        binary.push_back(VERSION);
        binary.push_back(0x00);
        binary.push_back(0x00);
        binary.push_back(0x00);
        binary.push_back(0x00);
        binary.push_back(0x00);
        binary.push_back(0x00);
        binary.push_back(0x00);
        
        // DICT: symbols
        std::string dict_str;
        for (int i = 0; i < 10; ++i) {
            dict_str += "sym_" + std::to_string(i) + "\0";
        }
        std::vector<uint8_t> dict_data(dict_str.begin(), dict_str.end());
        auto dict_lane = pack_lane(0x0001, dict_data.data(), dict_data.size(), 10);
        binary.insert(binary.end(), dict_lane.begin(), dict_lane.end());
        
        // FIELD: field definitions
        std::vector<uint8_t> field_data;
        for (int i = 0; i < 10; ++i) {
            field_data.push_back(i & 0xFF);
            field_data.push_back((i >> 8) & 0xFF);
            field_data.push_back((i >> 16) & 0xFF);
            field_data.push_back((i >> 24) & 0xFF);
        }
        auto field_lane = pack_lane(0x0002, field_data.data(), field_data.size(), 10);
        binary.insert(binary.end(), field_lane.begin(), field_lane.end());
        
        // LANE: events
        std::vector<uint8_t> lane_data;
        for (int i = 0; i < 20; ++i) {
            lane_data.push_back(i & 0xFF);
            lane_data.push_back((i >> 8) & 0xFF);
            lane_data.push_back((i >> 16) & 0xFF);
            lane_data.push_back((i >> 24) & 0xFF);
        }
        auto lane_obj = pack_lane(0x0003, lane_data.data(), lane_data.size(), 20);
        binary.insert(binary.end(), lane_obj.begin(), lane_obj.end());
        
        // EDGE: edges
        std::vector<uint8_t> edge_data;
        for (int i = 0; i < 15; ++i) {
            uint32_t val1 = i;
            uint32_t val2 = i + 1;
            edge_data.push_back(val1 & 0xFF);
            edge_data.push_back((val1 >> 8) & 0xFF);
            edge_data.push_back((val1 >> 16) & 0xFF);
            edge_data.push_back((val1 >> 24) & 0xFF);
            edge_data.push_back(val2 & 0xFF);
            edge_data.push_back((val2 >> 8) & 0xFF);
            edge_data.push_back((val2 >> 16) & 0xFF);
            edge_data.push_back((val2 >> 24) & 0xFF);
        }
        auto edge_lane = pack_lane(0x0004, edge_data.data(), edge_data.size(), 15);
        binary.insert(binary.end(), edge_lane.begin(), edge_lane.end());
        
        // BATCH: batch items
        std::string batch_str;
        for (int i = 0; i < 5; ++i) {
            batch_str += "batch_" + std::to_string(i);
        }
        std::vector<uint8_t> batch_data(batch_str.begin(), batch_str.end());
        auto batch_lane = pack_lane(0x0005, batch_data.data(), batch_data.size(), 5);
        binary.insert(binary.end(), batch_lane.begin(), batch_lane.end());
        
        // Manifest
        std::string manifest = R"({"@kind":"scxq2-canonical-v1.2","@version":"1.0","total_records":60,"created_tick":0,"creator":"SCXQ2CanonicalPacker.cpp","determinism":{"canonical_sorted":true,"hash_value":")" 
                             + sha256(binary.data(), binary.size()) + R"("}})";
        
        std::vector<uint8_t> manifest_bytes(manifest.begin(), manifest.end());
        binary.insert(binary.end(), manifest_bytes.begin(), manifest_bytes.end());
        
        // Footer
        uint32_t manifest_len = manifest_bytes.size();
        binary.push_back(manifest_len & 0xFF);
        binary.push_back((manifest_len >> 8) & 0xFF);
        binary.push_back((manifest_len >> 16) & 0xFF);
        binary.push_back((manifest_len >> 24) & 0xFF);
        
        uint32_t manifest_crc = adler32(manifest_bytes.data(), manifest_bytes.size());
        binary.push_back(manifest_crc & 0xFF);
        binary.push_back((manifest_crc >> 8) & 0xFF);
        binary.push_back((manifest_crc >> 16) & 0xFF);
        binary.push_back((manifest_crc >> 24) & 0xFF);
        
        // Fix total_len
        uint32_t total_len = binary.size();
        binary[8] = total_len & 0xFF;
        binary[9] = (total_len >> 8) & 0xFF;
        binary[10] = (total_len >> 16) & 0xFF;
        binary[11] = (total_len >> 24) & 0xFF;
        
        return binary;
    }
};

int main() {
    std::cout << "Generating SCXQ2 v1.2 canonical test vectors (C++)...\n\n";
    
    struct Vector {
        const char* name;
        std::vector<uint8_t> (*generator)();
    };
    
    Vector vectors[] = {
        {"scxq2_vector_empty.bin", SCXQ2CanonicalPacker::generate_vector_empty},
        {"scxq2_vector_simple.bin", SCXQ2CanonicalPacker::generate_vector_simple},
        {"scxq2_vector_events.bin", SCXQ2CanonicalPacker::generate_vector_events},
        {"scxq2_vector_edges.bin", SCXQ2CanonicalPacker::generate_vector_edges},
        {"scxq2_vector_full.bin", SCXQ2CanonicalPacker::generate_vector_full},
    };
    
    const char* output_dir = "C:\\public_html\\MX2LM\\codex\\AS-XCFE\\micronaut\\s7-llm-mini\\vectors\\";
    
    for (const auto& vec : vectors) {
        auto binary = vec.generator();
        
        std::string filepath = std::string(output_dir) + vec.name;
        std::ofstream file(filepath, std::ios::binary);
        if (!file) {
            std::cerr << "ERROR: Could not open " << filepath << std::endl;
            return 1;
        }
        
        file.write(reinterpret_cast<const char*>(binary.data()), binary.size());
        file.close();
        
        std::string hash = SCXQ2CanonicalPacker::sha256(binary.data(), binary.size());
        
        std::cout << "✓ " << std::setw(30) << std::left << vec.name 
                  << " (" << std::setw(6) << std::right << binary.size() << " bytes) "
                  << "hash=" << hash.substr(0, 16) << "...\n";
    }
    
    std::cout << "\n✓ All vectors generated in: " << output_dir << std::endl;
    return 0;
}
