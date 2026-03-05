#include <catch.hpp>
#include <cddsctl/core/Types.hpp>
#include <cstring>
#include <unordered_set>

using namespace cddsctl::core;

TEST_CASE("Guid::to_string formatting", "[guid]") {
    Guid guid;

    SECTION("All zeros") {
        std::fill(guid.value.begin(), guid.value.end(), 0);
        std::string result = guid.to_string();
        REQUIRE(result == "00000000.00000000.00000000.00000000");
    }

    SECTION("All 0xFF") {
        std::fill(guid.value.begin(), guid.value.end(), 0xFF);
        std::string result = guid.to_string();
        REQUIRE(result == "ffffffff.ffffffff.ffffffff.ffffffff");
    }

    SECTION("Sequential values") {
        for (size_t i = 0; i < guid.value.size(); ++i) {
            guid.value[i] = static_cast<uint8_t>(i);
        }
        std::string result = guid.to_string();
        REQUIRE(result == "00010203.04050607.08090a0b.0c0d0e0f");
    }

    SECTION("Mixed values") {
        guid.value = {0x01, 0x23, 0x45, 0x67, 0x89, 0xab, 0xcd, 0xef,
                      0xfe, 0xdc, 0xba, 0x98, 0x76, 0x54, 0x32, 0x10};
        std::string result = guid.to_string();
        REQUIRE(result == "01234567.89abcdef.fedcba98.76543210");
    }
}

TEST_CASE("Guid::from_dds_guid conversion", "[guid]") {
    SECTION("Valid DDS GUID") {
        uint8_t dds_guid[16] = {0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08,
                                0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f, 0x10};
        Guid guid = Guid::from_dds_guid(dds_guid);

        for (size_t i = 0; i < 16; ++i) {
            REQUIRE(guid.value[i] == dds_guid[i]);
        }
    }

    SECTION("Nullptr DDS GUID") {
        Guid guid = Guid::from_dds_guid(nullptr);
        for (size_t i = 0; i < 16; ++i) {
            REQUIRE(guid.value[i] == 0);
        }
    }
}

TEST_CASE("Guid equality operators", "[guid]") {
    Guid guid1, guid2;

    SECTION("Equal GUIDs") {
        std::fill(guid1.value.begin(), guid1.value.end(), 0x42);
        std::fill(guid2.value.begin(), guid2.value.end(), 0x42);

        REQUIRE(guid1 == guid2);
        REQUIRE_FALSE(guid1 != guid2);
    }

    SECTION("Different GUIDs") {
        std::fill(guid1.value.begin(), guid1.value.end(), 0x42);
        std::fill(guid2.value.begin(), guid2.value.end(), 0x43);

        REQUIRE(guid1 != guid2);
        REQUIRE_FALSE(guid1 == guid2);
    }

    SECTION("Single byte difference") {
        std::fill(guid1.value.begin(), guid1.value.end(), 0x00);
        std::fill(guid2.value.begin(), guid2.value.end(), 0x00);
        guid2.value[15] = 0x01;

        REQUIRE(guid1 != guid2);
    }
}

TEST_CASE("Guid comparison operator", "[guid]") {
    Guid guid1, guid2;

    SECTION("Less than") {
        std::fill(guid1.value.begin(), guid1.value.end(), 0x00);
        std::fill(guid2.value.begin(), guid2.value.end(), 0x01);

        REQUIRE(guid1 < guid2);
        REQUIRE_FALSE(guid2 < guid1);
    }

    SECTION("Equal GUIDs not less than") {
        std::fill(guid1.value.begin(), guid1.value.end(), 0x42);
        std::fill(guid2.value.begin(), guid2.value.end(), 0x42);

        REQUIRE_FALSE(guid1 < guid2);
        REQUIRE_FALSE(guid2 < guid1);
    }

    SECTION("Lexicographic ordering") {
        guid1.value = {0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                       0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
        guid2.value = {0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
                       0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};

        REQUIRE(guid2 < guid1);  // 0x00... < 0x01...
    }
}

TEST_CASE("GuidHash function", "[guid]") {
    GuidHash hasher;
    Guid guid1, guid2;

    SECTION("Same GUID produces same hash") {
        std::fill(guid1.value.begin(), guid1.value.end(), 0x42);
        std::fill(guid2.value.begin(), guid2.value.end(), 0x42);

        REQUIRE(hasher(guid1) == hasher(guid2));
    }

    SECTION("Different GUIDs typically produce different hashes") {
        // Use values that produce distinct hashes
        guid1.value = {0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08,
                       0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f, 0x10};
        guid2.value = {0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17, 0x18,
                       0x19, 0x1a, 0x1b, 0x1c, 0x1d, 0x1e, 0x1f, 0x20};

        // While collisions are possible in hash functions,
        // these specific values should produce different hashes
        REQUIRE(hasher(guid1) != hasher(guid2));
    }

    SECTION("Can be used in unordered_set") {
        std::unordered_set<Guid, GuidHash> guid_set;

        std::fill(guid1.value.begin(), guid1.value.end(), 0x01);
        std::fill(guid2.value.begin(), guid2.value.end(), 0x02);

        guid_set.insert(guid1);
        guid_set.insert(guid2);
        guid_set.insert(guid1);  // Duplicate

        REQUIRE(guid_set.size() == 2);
        REQUIRE(guid_set.count(guid1) == 1);
        REQUIRE(guid_set.count(guid2) == 1);
    }
}
