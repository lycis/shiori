#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "common.h"
#include "config.h"
#include "note.h"

/* logging.o normally gets this process-wide flag from main.c. The unit test
 * does not link main.c because it provides its own main(). */
bool g_debug_enabled = false;

static int failures = 0;

#define ASSERT_TRUE_REASON(condition, reason)                                                                          \
    do {                                                                                                               \
        if(!(condition)) {                                                                                             \
            fprintf(stderr, "FAIL %s:%d: %s (%s)\n", __FILE__, __LINE__, reason, #condition);                          \
            failures++;                                                                                                \
        }                                                                                                              \
    } while(false)

static void assert_encoded(const char *topic, const char *expected) {
    char encoded[DEFAULT_BUFFER_SIZE * 3] = {};
    int result = encode_topic(topic, encoded, sizeof(encoded));

    if(result != R_OK) {
        fprintf(stderr, "FAIL encode %s: expected R_OK, got %d\n", topic, result);
        failures++;
        return;
    }

    if(strcmp(encoded, expected) != 0) {
        fprintf(stderr, "FAIL encode %s: expected '%s', got '%s'\n", topic, expected, encoded);
        failures++;
    }
}

static void assert_decoded(const char *encoded, const char *expected) {
    char topic[DEFAULT_BUFFER_SIZE] = {};
    int result = decode_topic(encoded, strlen(encoded), topic, sizeof(topic));

    if(result != R_OK) {
        fprintf(stderr, "FAIL decode %s: expected R_OK, got %d\n", encoded, result);
        failures++;
        return;
    }

    if(strcmp(topic, expected) != 0) {
        fprintf(stderr, "FAIL decode %s: expected '%s', got '%s'\n", encoded, expected, topic);
        failures++;
    }
}

static void test_encode_preserves_uri_unreserved_bytes(void) {
    assert_encoded("Project-Alpha_2.0~notes", "Project-Alpha_2.0~notes");
}

static void test_encode_escapes_spaces_reserved_bytes_and_utf8(void) {
    assert_encoded("Project Alpha", "Project%20Alpha");
    assert_encoded("100% #1/planning", "100%25%20%231%2Fplanning");
    assert_encoded("Caf\xC3\xA9 Planung", "Caf%C3%A9%20Planung");
    assert_encoded("\xE6\x9D\xB1\xE4\xBA\xAC Planning", "%E6%9D%B1%E4%BA%AC%20Planning");
}

static void test_decode_restores_encoded_topics(void) {
    assert_decoded("Project%20Alpha", "Project Alpha");
    assert_decoded("100%25%20%231%2Fplanning", "100% #1/planning");
    assert_decoded("Caf%C3%A9%20Planung", "Caf\xC3\xA9 Planung");
    assert_decoded("%E6%9D%B1%E4%BA%AC%20Planning", "\xE6\x9D\xB1\xE4\xBA\xAC Planning");
}

static void test_decode_accepts_legacy_topics_and_lowercase_hex(void) {
    assert_decoded("project", "project");
    assert_decoded("Project%20caf%c3%a9", "Project caf\xC3\xA9");
}

static void test_decode_respects_explicit_input_length(void) {
    const char encoded[] = "A%20B trailing bytes";
    char topic[sizeof("A B")];

    int result = decode_topic(encoded, strlen("A%20B"), topic, sizeof(topic));
    ASSERT_TRUE_REASON(result == R_OK, "decoder rejected a valid explicit-length input");
    if(result == R_OK) {
        ASSERT_TRUE_REASON(strcmp(topic, "A B") == 0, "decoder read beyond the explicit input length");
    }
}

static void test_round_trip(void) {
    const char *topics[] = {
        "Project Alpha",
        "Caf\xC3\xA9 Planung",
        "\xE6\x9D\xB1\xE4\xBA\xAC Planning",
        "Progress 100% #1/planning",
    };

    for(size_t i = 0; i < sizeof(topics) / sizeof(topics[0]); ++i) {
        char encoded[DEFAULT_BUFFER_SIZE * 3];
        char decoded[DEFAULT_BUFFER_SIZE];

        int encode_result = encode_topic(topics[i], encoded, sizeof(encoded));
        if(encode_result != R_OK) {
            fprintf(stderr, "FAIL round trip '%s': encoder returned %d\n", topics[i], encode_result);
            failures++;
            continue;
        }

        int decode_result = decode_topic(encoded, strlen(encoded), decoded, sizeof(decoded));
        if(decode_result != R_OK) {
            fprintf(stderr, "FAIL round trip '%s': decoder returned %d for '%s'\n", topics[i], decode_result, encoded);
            failures++;
            continue;
        }

        if(strcmp(decoded, topics[i]) != 0) {
            fprintf(stderr, "FAIL round trip '%s': decoded as '%s' via '%s'\n", topics[i], decoded, encoded);
            failures++;
        }
    }
}

static void test_exact_buffer_sizes(void) {
    char encoded[sizeof("A%20B")];
    char decoded[sizeof("A B")];

    int encode_result = encode_topic("A B", encoded, sizeof(encoded));
    ASSERT_TRUE_REASON(encode_result == R_OK, "encoder rejected an exactly sized output buffer");
    if(encode_result != R_OK) {
        return;
    }

    ASSERT_TRUE_REASON(strcmp(encoded, "A%20B") == 0, "encoder produced the wrong exact-buffer result");

    int decode_result = decode_topic(encoded, strlen(encoded), decoded, sizeof(decoded));
    ASSERT_TRUE_REASON(decode_result == R_OK, "decoder rejected an exactly sized output buffer");
    if(decode_result == R_OK) {
        ASSERT_TRUE_REASON(strcmp(decoded, "A B") == 0, "decoder produced the wrong exact-buffer result");
    }
}

static void test_small_buffers_are_rejected(void) {
    char encoded[sizeof("A%20B") - 1];
    char decoded[sizeof("A B") - 1];

    ASSERT_TRUE_REASON(
        encode_topic("A B", encoded, sizeof(encoded)) == R_ERROR,
        "encoder accepted a buffer without room for the terminator"
    );
    ASSERT_TRUE_REASON(
        decode_topic("A%20B", strlen("A%20B"), decoded, sizeof(decoded)) == R_ERROR,
        "decoder accepted a buffer without room for the terminator"
    );
}

static void test_malformed_escapes_are_rejected(void) {
    char topic[DEFAULT_BUFFER_SIZE];

    ASSERT_TRUE_REASON(
        decode_topic("Project%", strlen("Project%"), topic, sizeof(topic)) == R_ERROR,
        "decoder accepted a trailing percent sign"
    );
    ASSERT_TRUE_REASON(
        decode_topic("Project%2", strlen("Project%2"), topic, sizeof(topic)) == R_ERROR,
        "decoder accepted an incomplete percent escape"
    );
    ASSERT_TRUE_REASON(
        decode_topic("Project%GG", strlen("Project%GG"), topic, sizeof(topic)) == R_ERROR,
        "decoder accepted non-hexadecimal escape digits"
    );
}

static void test_invalid_arguments_are_rejected(void) {
    char buffer[16];

    ASSERT_TRUE_REASON(encode_topic(nullptr, buffer, sizeof(buffer)) == R_ERROR, "encoder accepted a null topic");
    ASSERT_TRUE_REASON(encode_topic("topic", nullptr, sizeof(buffer)) == R_ERROR, "encoder accepted a null output");
    ASSERT_TRUE_REASON(encode_topic("topic", buffer, 0) == R_ERROR, "encoder accepted a zero-sized output");
    ASSERT_TRUE_REASON(decode_topic(nullptr, 0, buffer, sizeof(buffer)) == R_ERROR, "decoder accepted null input");
    ASSERT_TRUE_REASON(
        decode_topic("topic", strlen("topic"), nullptr, sizeof(buffer)) == R_ERROR,
        "decoder accepted a null output"
    );
    ASSERT_TRUE_REASON(
        decode_topic("topic", strlen("topic"), buffer, 0) == R_ERROR,
        "decoder accepted a zero-sized output"
    );
}

int main(void) {
    test_encode_preserves_uri_unreserved_bytes();
    test_encode_escapes_spaces_reserved_bytes_and_utf8();
    test_decode_restores_encoded_topics();
    test_decode_accepts_legacy_topics_and_lowercase_hex();
    test_decode_respects_explicit_input_length();
    test_round_trip();
    test_exact_buffer_sizes();
    test_small_buffers_are_rejected();
    test_malformed_escapes_are_rejected();
    test_invalid_arguments_are_rejected();

    if(failures > 0) {
        fprintf(stderr, "%d topic codec test(s) failed\n", failures);
        return EXIT_FAILURE;
    }

    printf("topic codec tests passed\n");
    return EXIT_SUCCESS;
}
