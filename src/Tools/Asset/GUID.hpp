#pragma once

#include <array>
#include <cstdint>
#include <string>
#include <random>
#include <sstream>
#include <iomanip>
#include <functional>
#include <algorithm>
#include <cstring>
#include <cctype>

namespace gbe {

    class GUID {
    public:
        // 128-bit storage (16 bytes)
        using ValueType = std::array<uint8_t, 16>;

        // Default constructor creates an Empty GUID (all zeros)
        GUID() {
            m_bytes.fill(0);
        }

        explicit GUID(const ValueType& bytes) : m_bytes(bytes) {}

        // Parse from string format: "f47ac10b-58cc-4372-a567-0e02b2c3d479"
        explicit GUID(const std::string& str) {
            *this = FromString(str);
        }

        // --- Static Helper Factories ---

        /**
         * @brief Generates an RFC 4122 Version 4 compliant random GUID.
         */
        static GUID Generate() {
            GUID guid;

            // Thread-safe high-performance random generation
            static thread_local std::random_device rd;
            static thread_local std::mt19937_64 gen(rd());
            static std::uniform_int_distribution<uint64_t> dis;

            uint64_t low = dis(gen);
            uint64_t high = dis(gen);

            std::memcpy(guid.m_bytes.data(), &low, 8);
            std::memcpy(guid.m_bytes.data() + 8, &high, 8);

            // RFC 4122 Version 4 (Random) & Variant bits specification
            guid.m_bytes[6] = (guid.m_bytes[6] & 0x0F) | 0x40; // Version 4
            guid.m_bytes[8] = (guid.m_bytes[8] & 0x3F) | 0x80; // Variant 1

            return guid;
        }

        static GUID Empty() {
            return GUID();
        }

        static GUID FromString(const std::string& str) {
            GUID guid;
            std::string hex;
            hex.reserve(32);

            for (char c : str) {
                if (std::isxdigit(static_cast<unsigned char>(c))) {
                    hex.push_back(c);
                }
            }

            if (hex.length() != 32) {
                return GUID::Empty(); // Fallback on invalid format
            }

            for (size_t i = 0; i < 16; ++i) {
                std::string byteString = hex.substr(i * 2, 2);
                guid.m_bytes[i] = static_cast<uint8_t>(std::stoul(byteString, nullptr, 16));
            }

            return guid;
        }

        // --- Utilities ---

        std::string ToString() const {
            if (IsEmpty()) return "00000000-0000-0000-0000-000000000000";

            std::stringstream ss;
            ss << std::hex << std::setfill('0');

            for (size_t i = 0; i < 16; ++i) {
                if (i == 4 || i == 6 || i == 8 || i == 10) {
                    ss << "-";
                }
                ss << std::setw(2) << static_cast<int>(m_bytes[i]);
            }

            return ss.str();
        }

        const ValueType& GetBytes() const { return m_bytes; }
        bool IsEmpty() const { return *this == Empty(); }

        // --- Operators ---

        bool operator==(const GUID& other) const { return m_bytes == other.m_bytes; }
        bool operator!=(const GUID& other) const { return m_bytes != other.m_bytes; }
        bool operator<(const GUID& other) const { return m_bytes < other.m_bytes; }

        explicit operator bool() const { return !IsEmpty(); }

    private:
        ValueType m_bytes{};
    };

    // Output stream integration (e.g. std::cout << guid)
    inline std::ostream& operator<<(std::ostream& os, const GUID& guid) {
        os << guid.ToString();
        return os;
    }

} // namespace gbe

// --- std::hash specialization for std::unordered_map support ---
namespace std {
    template <>
    struct hash<gbe::GUID> {
        size_t operator()(const gbe::GUID& guid) const noexcept {
            // Fast bit-shift combining hash of two 64-bit halves
            const uint64_t* p64 = reinterpret_cast<const uint64_t*>(guid.GetBytes().data());
            size_t h1 = std::hash<uint64_t>{}(p64[0]);
            size_t h2 = std::hash<uint64_t>{}(p64[1]);
            return h1 ^ (h2 + 0x9e3779b9 + (h1 << 6) + (h1 >> 2));
        }
    };
}