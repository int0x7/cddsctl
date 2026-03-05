#include <catch.hpp>
#include <cddsctl/core/Types.hpp>

using namespace cddsctl::core;

TEST_CASE("TopicFilter exact name matching", "[topic_filter]") {
    TopicFilter filter;
    filter.use_regex = false;

    SECTION("Single topic match") {
        filter.topics = {"/sensor/imu"};

        REQUIRE(filter.matches("/sensor/imu") == true);
        REQUIRE(filter.matches("/sensor/camera") == false);
        REQUIRE(filter.matches("sensor/imu") == false);
    }

    SECTION("Multiple topics match") {
        filter.topics = {"/sensor/imu", "/sensor/camera", "/cmd_vel"};

        REQUIRE(filter.matches("/sensor/imu") == true);
        REQUIRE(filter.matches("/sensor/camera") == true);
        REQUIRE(filter.matches("/cmd_vel") == true);
        REQUIRE(filter.matches("/other/topic") == false);
    }

    SECTION("Case sensitive") {
        filter.topics = {"/Topic/Name"};

        REQUIRE(filter.matches("/Topic/Name") == true);
        REQUIRE(filter.matches("/topic/name") == false);
        REQUIRE(filter.matches("/TOPIC/NAME") == false);
    }
}

TEST_CASE("TopicFilter regex matching", "[topic_filter]") {
    TopicFilter filter;
    filter.use_regex = true;

    SECTION("Simple regex pattern") {
        filter.topics = {"/sensor/.*"};

        REQUIRE(filter.matches("/sensor/imu") == true);
        REQUIRE(filter.matches("/sensor/camera") == true);
        REQUIRE(filter.matches("/sensor/gps/data") == true);
        REQUIRE(filter.matches("/cmd_vel") == false);
    }

    SECTION("Multiple regex patterns") {
        filter.topics = {"/sensor/.*", "/robot[0-9]+/.*"};

        REQUIRE(filter.matches("/sensor/imu") == true);
        REQUIRE(filter.matches("/robot1/status") == true);
        REQUIRE(filter.matches("/robot42/cmd") == true);
        REQUIRE(filter.matches("/robotA/cmd") == false);
        REQUIRE(filter.matches("/other/topic") == false);
    }

    SECTION("Anchored regex") {
        filter.topics = {"^/exact/path$"};

        REQUIRE(filter.matches("/exact/path") == true);
        REQUIRE(filter.matches("/exact/path/extra") == false);
        REQUIRE(filter.matches("prefix/exact/path") == false);
    }

    SECTION("Complex regex pattern") {
        filter.topics = {"/topic_[a-z]+_[0-9]{2,4}"};

        REQUIRE(filter.matches("/topic_abc_12") == true);
        REQUIRE(filter.matches("/topic_xyz_1234") == true);
        REQUIRE(filter.matches("/topic_abc_1") == false);
        REQUIRE(filter.matches("/topic_ABC_12") == false);
    }
}

TEST_CASE("TopicFilter exclude pattern", "[topic_filter]") {
    TopicFilter filter;

    SECTION("Exclude with empty include list (match all)") {
        filter.topics = {};  // Match all
        filter.exclude_regex = "/internal/.*";

        REQUIRE(filter.matches("/sensor/imu") == true);
        REQUIRE(filter.matches("/cmd_vel") == true);
        REQUIRE(filter.matches("/internal/debug") == false);
        REQUIRE(filter.matches("/internal/status") == false);
    }

    SECTION("Exclude with specific include list") {
        filter.topics = {"/robot"};
        filter.use_regex = true;
        filter.exclude_regex = "/robot/internal.*";

        // Note: /robot with regex will match exactly /robot
        filter.topics = {"/robot/.*"};

        REQUIRE(filter.matches("/robot/sensor") == true);
        REQUIRE(filter.matches("/robot/cmd") == true);
        REQUIRE(filter.matches("/robot/internal") == false);
        REQUIRE(filter.matches("/robot/internal_debug") == false);
    }

    SECTION("Exclude takes precedence") {
        filter.topics = {"/sensor/imu"};
        filter.use_regex = false;
        filter.exclude_regex = "/sensor/.*";

        // Even though /sensor/imu is in the include list,
        // the exclude regex matches it, so it should be excluded
        REQUIRE(filter.matches("/sensor/imu") == false);
    }
}

TEST_CASE("TopicFilter empty filter (match all)", "[topic_filter]") {
    TopicFilter filter;

    SECTION("Default filter matches everything") {
        REQUIRE(filter.topics.empty());
        REQUIRE(filter.use_regex == false);
        REQUIRE(filter.exclude_regex.empty());

        REQUIRE(filter.matches("/any/topic") == true);
        REQUIRE(filter.matches("relative_topic") == true);
        REQUIRE(filter.matches("") == true);
    }

    SECTION("Empty topics list matches everything") {
        filter.topics = {};

        REQUIRE(filter.matches("/sensor/imu") == true);
        REQUIRE(filter.matches("/cmd_vel") == true);
        REQUIRE(filter.matches("anything") == true);
    }
}

TEST_CASE("TopicFilter edge cases", "[topic_filter]") {
    TopicFilter filter;

    SECTION("Empty topic name with exact match") {
        filter.topics = {""};
        filter.use_regex = false;

        REQUIRE(filter.matches("") == true);
        REQUIRE(filter.matches("non_empty") == false);
    }

    SECTION("Special characters in exact match") {
        filter.topics = {"/topic/with.dots", "/topic/with-dashes"};
        filter.use_regex = false;

        REQUIRE(filter.matches("/topic/with.dots") == true);
        REQUIRE(filter.matches("/topic/with-dashes") == true);
    }

    SECTION("Special regex characters escaped in topics") {
        // When use_regex is true, dots are regex special characters
        filter.topics = {"/topic\\.exact"};
        filter.use_regex = true;

        REQUIRE(filter.matches("/topic.exact") == true);
        REQUIRE(filter.matches("/topicXexact") == false);
    }
}
