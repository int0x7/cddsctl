#include <catch.hpp>
#include <cddsctl/core/Config.hpp>
#include <fstream>
#include <cstdio>

using namespace cddsctl::core;

// Test implementation of ConfigBase for testing
class TestConfig : public ConfigBase {
public:
    std::string custom_value;
    int custom_int = 0;

protected:
    void parse_specific(const YAML::Node& root) override {
        if (root["custom"]) {
            custom_value = get_optional<std::string>(root["custom"], "value", "");
            custom_int = get_optional<int>(root["custom"], "number", 0);
        }
    }
};

TEST_CASE("ConfigBase load from YAML string", "[config]") {
    TestConfig config;

    SECTION("Basic DDS configuration") {
        std::string yaml = R"(
dds:
  domain_id: 42
  discovery_timeout: 5.0
)";
        config.load_from_string(yaml);

        REQUIRE(config.dds_config().domain_id == 42);
        REQUIRE(config.dds_config().discovery_timeout == Approx(5.0));
    }

    SECTION("Topic filter configuration") {
        std::string yaml = R"(
dds:
  domain_id: 0
  topics:
    - /sensor/imu
    - /sensor/camera
  use_regex: false
)";
        config.load_from_string(yaml);

        const auto& filter = config.dds_config().topic_filter;
        REQUIRE(filter.topics.size() == 2);
        REQUIRE(filter.topics[0] == "/sensor/imu");
        REQUIRE(filter.topics[1] == "/sensor/camera");
        REQUIRE(filter.use_regex == false);
    }

    SECTION("Topic filter with regex enabled") {
        std::string yaml = R"(
dds:
  topics:
    - /sensor/.*
    - /robot[0-9]+/.*
  use_regex: true
)";
        config.load_from_string(yaml);

        const auto& filter = config.dds_config().topic_filter;
        REQUIRE(filter.topics.size() == 2);
        REQUIRE(filter.use_regex == true);
    }

    SECTION("Custom configuration values") {
        std::string yaml = R"(
custom:
  value: "hello world"
  number: 123
)";
        config.load_from_string(yaml);

        REQUIRE(config.custom_value == "hello world");
        REQUIRE(config.custom_int == 123);
    }
}

TEST_CASE("ConfigBase default values", "[config]") {
    TestConfig config;

    SECTION("Missing DDS section uses defaults") {
        std::string yaml = R"(
custom:
  value: "test"
)";
        config.load_from_string(yaml);

        REQUIRE(config.dds_config().domain_id == 0);
        REQUIRE(config.dds_config().discovery_timeout == Approx(10.0));
        REQUIRE(config.dds_config().topic_filter.topics.empty());
    }

    SECTION("Missing fields use defaults") {
        std::string yaml = R"(
dds:
  domain_id: 5
)";
        config.load_from_string(yaml);

        REQUIRE(config.dds_config().domain_id == 5);
        REQUIRE(config.dds_config().discovery_timeout == Approx(10.0));
    }

    SECTION("Empty YAML uses all defaults") {
        config.load_from_string("");

        REQUIRE(config.dds_config().domain_id == 0);
        REQUIRE(config.dds_config().discovery_timeout == Approx(10.0));
    }
}

TEST_CASE("ConfigBase load from file", "[config]") {
    TestConfig config;

    SECTION("Valid YAML file") {
        // Create temporary file
        std::string tmp_path = "/tmp/test_config_valid.yaml";
        {
            std::ofstream ofs(tmp_path);
            ofs << R"(
dds:
  domain_id: 99
  topics:
    - /test/topic
custom:
  value: "from file"
)";
        }

        config.load_from_file(tmp_path);

        REQUIRE(config.dds_config().domain_id == 99);
        REQUIRE(config.dds_config().topic_filter.topics.size() == 1);
        REQUIRE(config.custom_value == "from file");

        std::remove(tmp_path.c_str());
    }

    SECTION("Non-existent file throws exception") {
        REQUIRE_THROWS_AS(
            config.load_from_file("/non/existent/path.yaml"),
            ConfigException
        );
    }
}

TEST_CASE("ConfigBase invalid YAML handling", "[config]") {
    TestConfig config;

    SECTION("Invalid YAML syntax throws exception") {
        std::string invalid_yaml = R"(
dds:
  domain_id: [invalid
)";
        REQUIRE_THROWS_AS(
            config.load_from_string(invalid_yaml),
            ConfigException
        );
    }

    SECTION("Invalid type for field uses default") {
        // When type conversion fails, get_optional returns default
        std::string yaml = R"(
dds:
  domain_id: "not a number"
  discovery_timeout: 5.0
)";
        // This should not throw, but use default for domain_id
        config.load_from_string(yaml);
        REQUIRE(config.dds_config().domain_id == 0);  // default
        REQUIRE(config.dds_config().discovery_timeout == Approx(5.0));
    }
}

TEST_CASE("ConfigBase setters", "[config]") {
    TestConfig config;

    SECTION("set_domain_id") {
        config.set_domain_id(123);
        REQUIRE(config.dds_config().domain_id == 123);
    }

    SECTION("set_topic_filter") {
        TopicFilter filter;
        filter.topics = {"/a", "/b"};
        filter.use_regex = true;

        config.set_topic_filter(filter);

        REQUIRE(config.dds_config().topic_filter.topics.size() == 2);
        REQUIRE(config.dds_config().topic_filter.use_regex == true);
    }

    SECTION("Setters override loaded values") {
        std::string yaml = R"(
dds:
  domain_id: 10
)";
        config.load_from_string(yaml);
        REQUIRE(config.dds_config().domain_id == 10);

        config.set_domain_id(20);
        REQUIRE(config.dds_config().domain_id == 20);
    }
}
