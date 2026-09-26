#include <cstdint>
#include <cstring>
#include <vector>
#include <string>
#include <fstream>
#include <iostream>
#include <iomanip>
#include <sstream>

// Inline SHA256 implementation (no external dependencies)
class SHA256 {
private:
    uint32_t m_state[8];
    uint64_t m_count;
    uint8_t m_buffer[64];
    uint8_t m_input_buffer[64];

    static const uint32_t K[64];
    
    static uint32_t rotr(uint32_t x, uint32_t n) {
        return (x >> n) | (x << (32 - n));
    }
    
    static uint32_t ch(uint32_t x, uint32_t y, uint32_t z) {
        return (x & y) ^ (~x & z);
    }
    
    static uint32_t maj(uint32_t x, uint32_t y, uint32_t z) {
        return (x & y) ^ (x & z) ^ (y & z);
    }
    
    static uint32_t sigma0(uint32_t x) {
        return rotr(x, 2) ^ rotr(x, 13) ^ rotr(x, 22);
    }
    
    static uint32_t sigma1(uint32_t x) {
        return rotr(x, 6) ^ rotr(x, 11) ^ rotr(x, 25);
    }
    
    static uint32_t gamma0(uint32_t x) {
        return rotr(x, 7) ^ rotr(x, 18) ^ (x >> 3);
    }
    
    static uint32_t gamma1(uint32_t x) {
        return rotr(x, 17) ^ rotr(x, 19) ^ (x >> 10);
    }
    
    void transform(const uint8_t* buffer) {
        uint32_t W[64];
        for (int i = 0; i < 16; i++) {
            W[i] = (buffer[i*4] << 24) | (buffer[i*4+1] << 16) | (buffer[i*4+2] << 8) | buffer[i*4+3];
        }
        
        for (int i = 16; i < 64; i++) {
            W[i] = gamma1(W[i-2]) + W[i-7] + gamma0(W[i-15]) + W[i-16];
        }
        
        uint32_t a = m_state[0], b = m_state[1], c = m_state[2], d = m_state[3];
        uint32_t e = m_state[4], f = m_state[5], g = m_state[6], h = m_state[7];
        
        for (int i = 0; i < 64; i++) {
            uint32_t t1 = h + sigma1(e) + ch(e, f, g) + K[i] + W[i];
            uint32_t t2 = sigma0(a) + maj(a, b, c);
            h = g;
            g = f;
            f = e;
            e = d + t1;
            d = c;
            c = b;
            b = a;
            a = t1 + t2;
        }
        
        m_state[0] += a;
        m_state[1] += b;
        m_state[2] += c;
        m_state[3] += d;
        m_state[4] += e;
        m_state[5] += f;
        m_state[6] += g;
        m_state[7] += h;
    }

public:
    SHA256() : m_count(0) {
        m_state[0] = 0x6a09e667;
        m_state[1] = 0xbb67ae85;
        m_state[2] = 0x3c6ef372;
        m_state[3] = 0xa54ff53a;
        m_state[4] = 0x510e527f;
        m_state[5] = 0x9b05688c;
        m_state[6] = 0x1f83d9ab;
        m_state[7] = 0x5be0cd19;
    }
    
    void update(const uint8_t* data, size_t len) {
        size_t idx = (m_count / 8) % 64;
        m_count += len * 8;
        size_t partLen = 64 - idx;
        
        size_t i = 0;
        if (len >= partLen) {
            std::memcpy(&m_buffer[idx], data, partLen);
            transform(m_buffer);
            
            for (i = partLen; i + 63 < len; i += 64) {
                transform(&data[i]);
            }
            idx = 0;
        }
        
        std::memcpy(&m_buffer[idx], &data[i], len - i);
    }
    
    std::string finalize() {
        uint8_t bits[8];
        for (int i = 7; i >= 0; i--) {
            bits[i] = m_count & 0xFF;
            m_count >>= 8;
        }
        
        size_t idx = (m_count / 8) % 64;
        size_t padLen = (idx < 56) ? (56 - idx) : (120 - idx);
        
        uint8_t padding[64];
        padding[0] = 0x80;
        std::memset(&padding[1], 0, sizeof(padding) - 1);
        
        update(padding, padLen);
        update(bits, 8);
        
        std::string result;
        for (int i = 0; i < 8; i++) {
            uint32_t val = m_state[i];
            result += "0123456789abcdef"[(val >> 28) & 0xF];
            result += "0123456789abcdef"[(val >> 24) & 0xF];
            result += "0123456789abcdef"[(val >> 20) & 0xF];
            result += "0123456789abcdef"[(val >> 16) & 0xF];
            result += "0123456789abcdef"[(val >> 12) & 0xF];
            result += "0123456789abcdef"[(val >> 8) & 0xF];
            result += "0123456789abcdef"[(val >> 4) & 0xF];
            result += "0123456789abcdef"[(val >> 0) & 0xF];
        }
        return result;
    }
};

const uint32_t SHA256::K[64] = {
    0x428a2f98, 0x71374491, 0xb5c0fbcf, 0xe9b5dba5, 0x3956c25b, 0x59f111f1, 0x923f82a4, 0xab1c5ed5,
    0xd807aa98, 0x12835b01, 0x243185be, 0x550c7dc3, 0x72be5d74, 0x80deb1fe, 0x9bdc06a7, 0xc19bf174,
    0xe49b69c1, 0xefbe4786, 0x0fc19dc6, 0x240ca1cc, 0x2de92c6f, 0x4a7484aa, 0x5cb0a9dc, 0x76f988da,
    0x983e5152, 0xa831c66d, 0xb00327c8, 0xbf597fc7, 0xc6e00bf3, 0xd5a79147, 0x06ca6351, 0x14292967,
    0x27b70a85, 0x2e1b2138, 0x4d2c6dfc, 0x53380d13, 0x650a7354, 0x766a0abb, 0x81c2c92e, 0x92722c85,
    0xa2bfe8a1, 0xa81a664b, 0xc24b8b70, 0xc76c51a3, 0xd192e819, 0xd6990624, 0xf40e3585, 0x106aa070,
    0x19a4c116, 0x1e376c08, 0x2748774c, 0x34b0bcb5, 0x391c0cb3, 0x4ed8aa4a, 0x5b9cca4f, 0x682e6ff3,
    0x748f82ee, 0x78a5636f, 0x84c87814, 0x8cc70208, 0x90befffa, 0xa4506ceb, 0xbef9a3f7, 0xc67178f2
};

class SCXQ2CanonicalPacker {
public:
    static constexpr uint32_t MAGIC = 0x51515153;  // "SCQQ"
    static constexpr uint8_t VERSION = 0x02;

    // Adler-32 checksum
    static uint32_t adler32(const uint8_t* data, size_t len) {
        uint32_t a = 1, b = 0;
        for (size_t i = 0; i < len; ++i) {
            a = (a + data[i]) % 65521;
            b = (b + a) % 65521;
        }
        return (b << 16) | a;
    }

    // SHA-256 hash
    static std::string sha256(const uint8_t* data, size_t len) {
        SHA256 sha;
        sha.update(data, len);
        return sha.finalize();
    }

    // Pack a single lane
    static std::vector<uint8_t> pack_lane(uint16_t lane_id, const uint8_t* data, size_t data_len, uint32_t record_count) {
        std::vector<uint8_t> lane;
        
        lane.push_back(lane_id & 0xFF);
        lane.push_back((lane_id >> 8) & 0xFF);
        
        uint32_t lane_len = 4 + data_len;
        lane.push_back(lane_len & 0xFF);
        lane.push_back((lane_len >> 8) & 0xFF);
        lane.push_back((lane_len >> 16) & 0xFF);
        lane.push_back((lane_len >> 24) & 0xFF);
        
        lane.push_back(record_count & 0xFF);
        lane.push_back((record_count >> 8) & 0xFF);
        lane.push_back((record_count >> 16) & 0xFF);
        lane.push_back((record_count >> 24) & 0xFF);
        
        if (data_len > 0) {
            lane.insert(lane.end(), data, data + data_len);
        }
        
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
        
        for (uint16_t lane_id = 0x0001; lane_id <= 0x0005; ++lane_id) {
            auto lane = pack_lane(lane_id, nullptr, 0, 0);
            binary.insert(binary.end(), lane.begin(), lane.end());
        }
        
        std::string manifest = R"({"@kind":"scxq2-canonical-v1.2","@version":"1.0","total_records":0,"created_tick":0,"creator":"SCXQ2CanonicalPacker.cpp","determinism":{"canonical_sorted":true,"hash_value":")" 
                             + sha256(binary.data(), binary.size()) + R"("}})";
        
        std::vector<uint8_t> manifest_bytes(manifest.begin(), manifest.end());
        binary.insert(binary.end(), manifest_bytes.begin(), manifest_bytes.end());
        
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
        
        uint32_t total_len = binary.size();
        binary[8] = total_len & 0xFF;
        binary[9] = (total_len >> 8) & 0xFF;
        binary[10] = (total_len >> 16) & 0xFF;
        binary[11] = (total_len >> 24) & 0xFF;
        
        return binary;
    }

    static std::vector<uint8_t> generate_vector_simple() {
        std::vector<uint8_t> binary;
        
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
        
        const uint8_t dict_data[] = "symbol_a\0symbol_b\0";
        auto dict_lane = pack_lane(0x0001, dict_data, sizeof(dict_data) - 1, 2);
        binary.insert(binary.end(), dict_lane.begin(), dict_lane.end());
        
        uint8_t field_data[16];
        std::memcpy(field_data, "\x01\x00\x00\x00\x02\x00\x00\x00timestamp", 16);
        auto field_lane = pack_lane(0x0002, field_data, 16, 1);
        binary.insert(binary.end(), field_lane.begin(), field_lane.end());
        
        uint8_t lane_data[14];
        std::memcpy(lane_data, "\x00\x00\x00\x00\x01\x00event_data", 14);
        auto lane_obj = pack_lane(0x0003, lane_data, 14, 1);
        binary.insert(binary.end(), lane_obj.begin(), lane_obj.end());
        
        uint8_t edge_data[12];
        std::memcpy(edge_data, "\x00\x00\x00\x00\x01\x00\x00\x00causality", 12);
        auto edge_lane = pack_lane(0x0004, edge_data, 12, 1);
        binary.insert(binary.end(), edge_lane.begin(), edge_lane.end());
        
        auto batch_lane = pack_lane(0x0005, nullptr, 0, 0);
        binary.insert(binary.end(), batch_lane.begin(), batch_lane.end());
        
        std::string manifest = R"({"@kind":"scxq2-canonical-v1.2","@version":"1.0","total_records":3,"created_tick":0,"creator":"SCXQ2CanonicalPacker.cpp","determinism":{"canonical_sorted":true,"hash_value":")" 
                             + sha256(binary.data(), binary.size()) + R"("}})";
        
        std::vector<uint8_t> manifest_bytes(manifest.begin(), manifest.end());
        binary.insert(binary.end(), manifest_bytes.begin(), manifest_bytes.end());
        
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
        
        uint32_t total_len = binary.size();
        binary[8] = total_len & 0xFF;
        binary[9] = (total_len >> 8) & 0xFF;
        binary[10] = (total_len >> 16) & 0xFF;
        binary[11] = (total_len >> 24) & 0xFF;
        
        return binary;
    }

    static std::vector<uint8_t> generate_vector_events() {
        std::vector<uint8_t> binary;
        
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
        
        auto dict_lane = pack_lane(0x0001, nullptr, 0, 0);
        binary.insert(binary.end(), dict_lane.begin(), dict_lane.end());
        
        auto field_lane = pack_lane(0x0002, nullptr, 0, 0);
        binary.insert(binary.end(), field_lane.begin(), field_lane.end());
        
        uint8_t lane_data[12];
        for (int i = 0; i < 3; ++i) {
            std::memcpy(lane_data + i * 4, &i, 4);
        }
        auto lane_obj = pack_lane(0x0003, lane_data, 12, 3);
        binary.insert(binary.end(), lane_obj.begin(), lane_obj.end());
        
        auto edge_lane = pack_lane(0x0004, nullptr, 0, 0);
        binary.insert(binary.end(), edge_lane.begin(), edge_lane.end());
        
        auto batch_lane = pack_lane(0x0005, nullptr, 0, 0);
        binary.insert(binary.end(), batch_lane.begin(), batch_lane.end());
        
        std::string manifest = R"({"@kind":"scxq2-canonical-v1.2","@version":"1.0","total_records":3,"created_tick":0,"creator":"SCXQ2CanonicalPacker.cpp","determinism":{"canonical_sorted":true,"hash_value":")" 
                             + sha256(binary.data(), binary.size()) + R"("}})";
        
        std::vector<uint8_t> manifest_bytes(manifest.begin(), manifest.end());
        binary.insert(binary.end(), manifest_bytes.begin(), manifest_bytes.end());
        
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
        
        uint32_t total_len = binary.size();
        binary[8] = total_len & 0xFF;
        binary[9] = (total_len >> 8) & 0xFF;
        binary[10] = (total_len >> 16) & 0xFF;
        binary[11] = (total_len >> 24) & 0xFF;
        
        return binary;
    }

    static std::vector<uint8_t> generate_vector_edges() {
        std::vector<uint8_t> binary;
        
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
        
        auto dict_lane = pack_lane(0x0001, nullptr, 0, 0);
        binary.insert(binary.end(), dict_lane.begin(), dict_lane.end());
        
        auto field_lane = pack_lane(0x0002, nullptr, 0, 0);
        binary.insert(binary.end(), field_lane.begin(), field_lane.end());
        
        auto lane_obj = pack_lane(0x0003, nullptr, 0, 0);
        binary.insert(binary.end(), lane_obj.begin(), lane_obj.end());
        
        uint8_t edge_data[24];
        for (int i = 0; i < 3; ++i) {
            uint32_t val1 = i;
            uint32_t val2 = i + 1;
            std::memcpy(edge_data + i * 8, &val1, 4);
            std::memcpy(edge_data + i * 8 + 4, &val2, 4);
        }
        auto edge_lane = pack_lane(0x0004, edge_data, 24, 3);
        binary.insert(binary.end(), edge_lane.begin(), edge_lane.end());
        
        auto batch_lane = pack_lane(0x0005, nullptr, 0, 0);
        binary.insert(binary.end(), batch_lane.begin(), batch_lane.end());
        
        std::string manifest = R"({"@kind":"scxq2-canonical-v1.2","@version":"1.0","total_records":3,"created_tick":0,"creator":"SCXQ2CanonicalPacker.cpp","determinism":{"canonical_sorted":true,"hash_value":")" 
                             + sha256(binary.data(), binary.size()) + R"("}})";
        
        std::vector<uint8_t> manifest_bytes(manifest.begin(), manifest.end());
        binary.insert(binary.end(), manifest_bytes.begin(), manifest_bytes.end());
        
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
        
        uint32_t total_len = binary.size();
        binary[8] = total_len & 0xFF;
        binary[9] = (total_len >> 8) & 0xFF;
        binary[10] = (total_len >> 16) & 0xFF;
        binary[11] = (total_len >> 24) & 0xFF;
        
        return binary;
    }

    static std::vector<uint8_t> generate_vector_full() {
        std::vector<uint8_t> binary;
        
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
        
        std::string dict_str;
        for (int i = 0; i < 10; ++i) {
            dict_str += "sym_" + std::to_string(i) + "\0";
        }
        std::vector<uint8_t> dict_data(dict_str.begin(), dict_str.end());
        auto dict_lane = pack_lane(0x0001, dict_data.data(), dict_data.size(), 10);
        binary.insert(binary.end(), dict_lane.begin(), dict_lane.end());
        
        std::vector<uint8_t> field_data;
        for (int i = 0; i < 10; ++i) {
            field_data.push_back(i & 0xFF);
            field_data.push_back((i >> 8) & 0xFF);
            field_data.push_back((i >> 16) & 0xFF);
            field_data.push_back((i >> 24) & 0xFF);
        }
        auto field_lane = pack_lane(0x0002, field_data.data(), field_data.size(), 10);
        binary.insert(binary.end(), field_lane.begin(), field_lane.end());
        
        std::vector<uint8_t> lane_data;
        for (int i = 0; i < 20; ++i) {
            lane_data.push_back(i & 0xFF);
            lane_data.push_back((i >> 8) & 0xFF);
            lane_data.push_back((i >> 16) & 0xFF);
            lane_data.push_back((i >> 24) & 0xFF);
        }
        auto lane_obj = pack_lane(0x0003, lane_data.data(), lane_data.size(), 20);
        binary.insert(binary.end(), lane_obj.begin(), lane_obj.end());
        
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
        
        std::string batch_str;
        for (int i = 0; i < 5; ++i) {
            batch_str += "batch_" + std::to_string(i);
        }
        std::vector<uint8_t> batch_data(batch_str.begin(), batch_str.end());
        auto batch_lane = pack_lane(0x0005, batch_data.data(), batch_data.size(), 5);
        binary.insert(binary.end(), batch_lane.begin(), batch_lane.end());
        
        std::string manifest = R"({"@kind":"scxq2-canonical-v1.2","@version":"1.0","total_records":60,"created_tick":0,"creator":"SCXQ2CanonicalPacker.cpp","determinism":{"canonical_sorted":true,"hash_value":")" 
                             + sha256(binary.data(), binary.size()) + R"("}})";
        
        std::vector<uint8_t> manifest_bytes(manifest.begin(), manifest.end());
        binary.insert(binary.end(), manifest_bytes.begin(), manifest_bytes.end());
        
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
