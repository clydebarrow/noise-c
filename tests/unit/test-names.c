/*
 * Copyright (C) 2016 Southern Storm Software, Pty Ltd.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included
 * in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 */

#include "test-helpers.h"
#include <stdarg.h>

/* Check that a known mapping is present in the algorithm_names table */
static void check_id(const char *name, int id)
{
    int category = id & NOISE_ID(0xFF, 0);
    char adjusted[32];

    /* Check the expected mappings */
    verify(!strcmp(noise_id_to_name(category, id), name));
    verify(!strcmp(noise_id_to_name(0, id), name));
    compare(noise_name_to_id(category, name, strlen(name)), id);
    compare(noise_name_to_id(0, name, strlen(name)), id);

    /* Check that the name length must be exact for a match */
    if (category != NOISE_PATTERN_CATEGORY) {
        /* Doesn't work for patterns which can be prefixes of each other */
        compare(noise_name_to_id(category, name, strlen(name) - 1), 0);
        compare(noise_name_to_id(0, name, strlen(name) - 1), 0);
    }
    strcpy(adjusted, name);
    strcat(adjusted, "Z");
    compare(noise_name_to_id(category, adjusted, strlen(name) + 1), 0);
    compare(noise_name_to_id(0, adjusted, strlen(name) + 1), 0);

    /* Check that we cannot find the name/id under the wrong category */
    category ^= 0x0100;
    verify(noise_id_to_name(category, id) == NULL);
    compare(noise_name_to_id(category, name, strlen(name)), 0);
}

static void test_id_mappings(void)
{
    /* Check for known names/identifiers */
    check_id("ChaChaPoly", NOISE_CIPHER_CHACHAPOLY);
    check_id("AESGCM", NOISE_CIPHER_AESGCM);

    check_id("BLAKE2s", NOISE_HASH_BLAKE2s);
    check_id("BLAKE2b", NOISE_HASH_BLAKE2b);
    check_id("SHA256", NOISE_HASH_SHA256);
    check_id("SHA512", NOISE_HASH_SHA512);

    check_id("25519", NOISE_DH_CURVE25519);
    check_id("448", NOISE_DH_CURVE448);
    check_id("NewHope", NOISE_DH_NEWHOPE);

    check_id("N", NOISE_PATTERN_N);
    check_id("X", NOISE_PATTERN_X);
    check_id("K", NOISE_PATTERN_K);
    check_id("NN", NOISE_PATTERN_NN);
    check_id("NK", NOISE_PATTERN_NK);
    check_id("NX", NOISE_PATTERN_NX);
    check_id("XN", NOISE_PATTERN_XN);
    check_id("XK", NOISE_PATTERN_XK);
    check_id("XX", NOISE_PATTERN_XX);
    check_id("KN", NOISE_PATTERN_KN);
    check_id("KK", NOISE_PATTERN_KK);
    check_id("KX", NOISE_PATTERN_KX);
    check_id("IN", NOISE_PATTERN_IN);
    check_id("IK", NOISE_PATTERN_IK);
    check_id("IX", NOISE_PATTERN_IX);

    check_id("fallback", NOISE_MODIFIER_FALLBACK);
    check_id("hfs", NOISE_MODIFIER_HFS);
    check_id("psk0", NOISE_MODIFIER_PSK0);
    check_id("psk1", NOISE_MODIFIER_PSK1);
    check_id("psk2", NOISE_MODIFIER_PSK2);
    check_id("psk3", NOISE_MODIFIER_PSK3);

    check_id("Noise", NOISE_PREFIX_STANDARD);

    check_id("Ed25519", NOISE_SIGN_ED25519);

    /* Check for unknown names/identifiers */
    compare(noise_name_to_id(NOISE_CIPHER_CATEGORY, "AESGCM-128", 10), 0);
    compare(noise_name_to_id(0, "AESGCM-128", 10), 0);
    verify(noise_id_to_name(NOISE_CIPHER_CATEGORY, NOISE_ID('C', 200)) == 0);
    verify(noise_id_to_name(0, NOISE_ID('C', 200)) == 0);
}

#define MAX_IDS 16

/* Check the parsing and construction of name lists */
static void check_name_list(const char *name, int error,
                            int category1, int category2, ...)
{
    int ids[MAX_IDS];
    int expected_ids[MAX_IDS];
    int expected_ids_len = 0;
    char output_name[NOISE_MAX_PROTOCOL_NAME];
    va_list va;

    /* Build the identifier list from the function arguments */
    va_start(va, category2);
    while (expected_ids_len < MAX_IDS) {
        expected_ids[expected_ids_len] = va_arg(va, int);
        if (!(expected_ids[expected_ids_len]))
            break;
        ++expected_ids_len;
    }
    va_end(va);

    /* Parse the name into an identifier list */
    if (error == NOISE_ERROR_NONE) {
        compare(noise_name_list_to_ids(ids, MAX_IDS, name, strlen(name),
                                       category1, category2), expected_ids_len);
        verify(!memcmp(ids, expected_ids, expected_ids_len * sizeof(int)));
    } else {
        compare(noise_name_list_to_ids(ids, MAX_IDS, name, strlen(name),
                                       category1, category2), -error);
        return;
    }

    /* Format the list back into a name */
    memset(output_name, 0x66, sizeof(output_name));
    compare(noise_ids_to_name_list(output_name, sizeof(output_name),
                                   expected_ids, expected_ids_len,
                                   category1, category2),
            NOISE_ERROR_NONE);
    verify(memchr(output_name, '\0', sizeof(output_name)) != 0);
    verify(!strcmp(output_name, name));

    /* Check for parameter error conditions */
    compare(noise_name_list_to_ids(0, MAX_IDS, name, strlen(name),
                                   category1, category2),
            -NOISE_ERROR_INVALID_PARAM);
    compare(noise_name_list_to_ids(ids, 0, name, strlen(name),
                                   category1, category2),
            -NOISE_ERROR_INVALID_LENGTH);
    compare(noise_name_list_to_ids(ids, MAX_IDS, 0, strlen(name),
                                   category1, category2),
            -NOISE_ERROR_INVALID_PARAM);
    compare(noise_name_list_to_ids(ids, MAX_IDS, name, 0,
                                   category1, category2),
            -NOISE_ERROR_UNKNOWN_NAME);
    compare(noise_name_list_to_ids(ids, MAX_IDS, name, strlen(name),
                                   NOISE_SIGN_CATEGORY, category2),
            -NOISE_ERROR_UNKNOWN_NAME);
    compare(noise_ids_to_name_list(0, sizeof(output_name),
                                   expected_ids, expected_ids_len,
                                   category1, category2),
            NOISE_ERROR_INVALID_PARAM);
    compare(noise_ids_to_name_list(output_name, 0,
                                   expected_ids, expected_ids_len,
                                   category1, category2),
            NOISE_ERROR_INVALID_PARAM);
    compare(noise_ids_to_name_list(output_name, sizeof(output_name),
                                   0, expected_ids_len,
                                   category1, category2),
            NOISE_ERROR_INVALID_PARAM);
    compare(noise_ids_to_name_list(output_name, sizeof(output_name),
                                   expected_ids, 0,
                                   category1, category2),
            NOISE_ERROR_INVALID_PARAM);
    compare(noise_ids_to_name_list(output_name, sizeof(output_name),
                                   expected_ids, expected_ids_len,
                                   NOISE_SIGN_CATEGORY, category2),
            NOISE_ERROR_UNKNOWN_ID);
    compare(noise_ids_to_name_list(output_name, 1,
                                   expected_ids, expected_ids_len,
                                   category1, category2),
            NOISE_ERROR_INVALID_LENGTH);
    compare(noise_ids_to_name_list(output_name, strlen(name),
                                   expected_ids, expected_ids_len,
                                   category1, category2),
            NOISE_ERROR_INVALID_LENGTH);
}

static void test_name_lists(void)
{
    check_name_list("25519", NOISE_ERROR_NONE,
                    NOISE_DH_CATEGORY, NOISE_DH_CATEGORY,
                    NOISE_DH_CURVE25519, 0);
    check_name_list("25519+448", NOISE_ERROR_NONE,
                    NOISE_DH_CATEGORY, 0,
                    NOISE_DH_CURVE25519, NOISE_DH_CURVE448, 0);
    check_name_list("25519+BLAKE2s", NOISE_ERROR_NONE,
                    NOISE_DH_CATEGORY, NOISE_HASH_CATEGORY,
                    NOISE_DH_CURVE25519, NOISE_HASH_BLAKE2s, 0);
    check_name_list("25519+BLAKE2s+SHA512", NOISE_ERROR_NONE,
                    NOISE_DH_CATEGORY, NOISE_HASH_CATEGORY,
                    NOISE_DH_CURVE25519, NOISE_HASH_BLAKE2s,
                    NOISE_HASH_SHA512, 0);

    check_name_list("KX", NOISE_ERROR_NONE,
                    NOISE_PATTERN_CATEGORY, NOISE_MODIFIER_CATEGORY,
                    NOISE_PATTERN_KX, 0);
    check_name_list("IKhfs", NOISE_ERROR_NONE,
                    NOISE_PATTERN_CATEGORY, NOISE_MODIFIER_CATEGORY,
                    NOISE_PATTERN_IK, NOISE_MODIFIER_HFS, 0);
    check_name_list("XXfallback+psk1", NOISE_ERROR_NONE,
                    NOISE_PATTERN_CATEGORY, NOISE_MODIFIER_CATEGORY,
                    NOISE_PATTERN_XX, NOISE_MODIFIER_FALLBACK,
                    NOISE_MODIFIER_PSK1, 0);
    check_name_list("XXfallback+hfs+psk0+psk1", NOISE_ERROR_NONE,
                    NOISE_PATTERN_CATEGORY, NOISE_MODIFIER_CATEGORY,
                    NOISE_PATTERN_XX, NOISE_MODIFIER_FALLBACK,
                    NOISE_MODIFIER_HFS, NOISE_MODIFIER_PSK0,
                    NOISE_MODIFIER_PSK1, 0);
    check_name_list("KX+N", NOISE_ERROR_NONE,
                    NOISE_PATTERN_CATEGORY, 0,
                    NOISE_PATTERN_KX, NOISE_PATTERN_N, 0);

    /* Parsing errors due to invalid inputs */
    check_name_list("", NOISE_ERROR_UNKNOWN_NAME,
                    NOISE_DH_CATEGORY, 0, 0);
    check_name_list("+25519", NOISE_ERROR_UNKNOWN_NAME,
                    NOISE_DH_CATEGORY, 0, 0);
    check_name_list("25519+", NOISE_ERROR_UNKNOWN_NAME,
                    NOISE_DH_CATEGORY, 0, 0);
    check_name_list("Curve25519+448", NOISE_ERROR_UNKNOWN_NAME,
                    NOISE_DH_CATEGORY, 0, 0);
    check_name_list("25519+448", NOISE_ERROR_UNKNOWN_NAME,
                    NOISE_DH_CATEGORY, NOISE_HASH_CATEGORY, 0);
    check_name_list("25519+448+", NOISE_ERROR_UNKNOWN_NAME,
                    NOISE_DH_CATEGORY, 0, 0);
    check_name_list("", NOISE_ERROR_UNKNOWN_NAME,
                    NOISE_PATTERN_CATEGORY, NOISE_MODIFIER_CATEGORY, 0);
    check_name_list("XX+", NOISE_ERROR_UNKNOWN_NAME,
                    NOISE_PATTERN_CATEGORY, NOISE_MODIFIER_CATEGORY, 0);
    check_name_list("XXxfs", NOISE_ERROR_UNKNOWN_NAME,
                    NOISE_PATTERN_CATEGORY, NOISE_MODIFIER_CATEGORY, 0);
    check_name_list("XX+hfs", NOISE_ERROR_UNKNOWN_NAME,
                    NOISE_PATTERN_CATEGORY, NOISE_MODIFIER_CATEGORY, 0);
    check_name_list("XXfallback+hfs+", NOISE_ERROR_UNKNOWN_NAME,
                    NOISE_PATTERN_CATEGORY, NOISE_MODIFIER_CATEGORY, 0);
}

/* Check the parsing and formatting of a specific protocol name */
static void check_protocol_name
    (const char *name, int prefix_id, int pattern_id,
     const int *modifier_ids, int dh_id, int cipher_id,
     int hash_id, int hybrid_id)
{
    NoiseProtocolId expected_id;
    NoiseProtocolId actual_id;
    char buffer[NOISE_MAX_PROTOCOL_NAME];
    size_t posn;

    /* Construct the expected protocol id from parsing the name */
    memset(&expected_id, 0, sizeof(expected_id));
    expected_id.prefix_id = prefix_id;
    expected_id.pattern_id = pattern_id;
    if (modifier_ids) {
        memcpy(expected_id.modifier_ids, modifier_ids,
               sizeof(expected_id.modifier_ids));
    }
    expected_id.dh_id = dh_id;
    expected_id.cipher_id = cipher_id;
    expected_id.hash_id = hash_id;
    expected_id.hybrid_id = hybrid_id;

    /* Parse the name into its constituent identifiers */
    memset(&actual_id, 0x66, sizeof(actual_id));
    compare(noise_protocol_name_to_id(&actual_id, name, strlen(name)),
            NOISE_ERROR_NONE);
    compare(actual_id.prefix_id, expected_id.prefix_id);
    if (modifier_ids) {
        verify(!memcmp(actual_id.modifier_ids, modifier_ids,
               sizeof(actual_id.modifier_ids)));
    } else {
        compare(actual_id.modifier_ids[0], NOISE_MODIFIER_NONE);
    }
    compare(actual_id.pattern_id, expected_id.pattern_id);
    compare(actual_id.dh_id, expected_id.dh_id);
    compare(actual_id.cipher_id, expected_id.cipher_id);
    compare(actual_id.hash_id, expected_id.hash_id);
    compare(actual_id.hybrid_id, expected_id.hybrid_id);
    verify(!memcmp(&actual_id, &expected_id, sizeof(actual_id)));

    /* Format the name from the identifiers */
    memset(buffer, 0xAA, sizeof(buffer));
    compare(noise_protocol_id_to_name(buffer, sizeof(buffer), &expected_id),
            NOISE_ERROR_NONE);
    verify(memchr(buffer, '\0', sizeof(buffer)) != 0);
    verify(!strcmp(buffer, name));

    /* Check for parameter error conditions */
    compare(noise_protocol_name_to_id(0, name, strlen(name)),
            NOISE_ERROR_INVALID_PARAM);
    compare(noise_protocol_name_to_id(&actual_id, 0, strlen(name)),
            NOISE_ERROR_INVALID_PARAM);
    memset(&actual_id, 0x66, sizeof(actual_id));
    compare(noise_protocol_name_to_id(&actual_id, name, strlen(name) - 1),
            NOISE_ERROR_UNKNOWN_NAME);
    compare(actual_id.prefix_id, 0);
    compare(actual_id.pattern_id, 0);
    compare(actual_id.modifier_ids[0], 0);
    compare(actual_id.dh_id, 0);
    compare(actual_id.cipher_id, 0);
    compare(actual_id.hash_id, 0);
    compare(actual_id.hybrid_id, 0);
    memset(buffer, 0xAA, sizeof(buffer));
    compare(noise_protocol_id_to_name(buffer, sizeof(buffer), 0),
            NOISE_ERROR_INVALID_PARAM);
    compare(buffer[0], '\0');
    compare(noise_protocol_id_to_name(0, sizeof(buffer), &expected_id),
            NOISE_ERROR_INVALID_PARAM);
    memset(buffer, 0x66, sizeof(buffer));
    compare(noise_protocol_id_to_name(buffer, 0, &expected_id),
            NOISE_ERROR_INVALID_LENGTH);
    compare(buffer[0], 0x66);
    compare(noise_protocol_id_to_name(buffer, strlen(name), &expected_id),
            NOISE_ERROR_INVALID_LENGTH);
    compare(buffer[0], '\0');
    compare(buffer[strlen(name)], 0x66); /* Check that no overflow occurred */
    memset(buffer, 0x66, sizeof(buffer));
    compare(noise_protocol_id_to_name(buffer, 3, &expected_id),
            NOISE_ERROR_INVALID_LENGTH);
    compare(buffer[0], '\0');

    /* Reserved identifiers cannot be formatted */
    for (posn = 0; posn < (sizeof(expected_id.reserved) / sizeof(expected_id.reserved[0])); ++posn) {
        expected_id.reserved[posn] = NOISE_PREFIX_STANDARD;
        memset(buffer, 0x66, sizeof(buffer));
        compare(noise_protocol_id_to_name(buffer, sizeof(buffer), &expected_id),
                NOISE_ERROR_UNKNOWN_ID);
        compare(buffer[0], '\0');
        expected_id.reserved[posn] = 0;
    }

    /* Identifiers in the wrong fields */
    expected_id.cipher_id = hash_id;
    memset(buffer, 0x66, sizeof(buffer));
    compare(noise_protocol_id_to_name(buffer, sizeof(buffer), &expected_id),
            NOISE_ERROR_UNKNOWN_ID);
    compare(buffer[0], '\0');
}

static void test_protocol_names(void)
{
    static int const fallback[NOISE_MAX_MODIFIER_IDS] = {
        NOISE_MODIFIER_FALLBACK
    };
    static int const multi[NOISE_MAX_MODIFIER_IDS] = {
        NOISE_MODIFIER_FALLBACK, NOISE_MODIFIER_HFS, NOISE_MODIFIER_PSK0
    };
    check_protocol_name
        ("Noise_XX_25519_AESGCM_SHA256",
         NOISE_PREFIX_STANDARD, NOISE_PATTERN_XX, NULL,
         NOISE_DH_CURVE25519, NOISE_CIPHER_AESGCM,
         NOISE_HASH_SHA256, 0);
    check_protocol_name
        ("Noise_N_25519_ChaChaPoly_BLAKE2s",
         NOISE_PREFIX_STANDARD, NOISE_PATTERN_N, NULL,
         NOISE_DH_CURVE25519, NOISE_CIPHER_CHACHAPOLY,
         NOISE_HASH_BLAKE2s, 0);
    check_protocol_name
        ("Noise_XXfallback_448_AESGCM_SHA512",
         NOISE_PREFIX_STANDARD, NOISE_PATTERN_XX, fallback,
         NOISE_DH_CURVE448, NOISE_CIPHER_AESGCM,
         NOISE_HASH_SHA512, 0);
    check_protocol_name
        ("Noise_XXfallback+hfs+psk0_448_AESGCM_SHA512",
         NOISE_PREFIX_STANDARD, NOISE_PATTERN_XX, multi,
         NOISE_DH_CURVE448, NOISE_CIPHER_AESGCM,
         NOISE_HASH_SHA512, 0);
    check_protocol_name
        ("Noise_IK_448_ChaChaPoly_BLAKE2b",
         NOISE_PREFIX_STANDARD, NOISE_PATTERN_IK, NULL,
         NOISE_DH_CURVE448, NOISE_CIPHER_CHACHAPOLY,
         NOISE_HASH_BLAKE2b, 0);
    check_protocol_name
        ("Noise_NN_NewHope_AESGCM_SHA256",
         NOISE_PREFIX_STANDARD, NOISE_PATTERN_NN, NULL,
         NOISE_DH_NEWHOPE, NOISE_CIPHER_AESGCM,
         NOISE_HASH_SHA256, 0);
    check_protocol_name
        ("Noise_XX_25519+NewHope_AESGCM_SHA256",
         NOISE_PREFIX_STANDARD, NOISE_PATTERN_XX, NULL,
         NOISE_DH_CURVE25519, NOISE_CIPHER_AESGCM,
         NOISE_HASH_SHA256, NOISE_DH_NEWHOPE);
}

void test_names(void)
{
    test_id_mappings();
    test_name_lists();
    test_protocol_names();
}
