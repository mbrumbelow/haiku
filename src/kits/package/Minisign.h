/* 
 * Copyright, 2023 Frank Denis. All rights reserved
 * Released under the terms of the MIT license.
 */
#ifndef MINISIGN_H 
#define MINISIGN_H 1

#include <sodium.h>

#define COMMENTMAXBYTES                1024
#define PASSWORDMAXBYTES               1024
#define SIGALG                         "Ed"
#define SIGALG_HASHED                  "ED"
#define KDFALG                         "Sc"
#define KDFNONE                        "\0\0"
#define CHKALG                         "B2"
#define COMMENT_PREFIX                 "untrusted comment: "
#define DEFAULT_COMMENT                "signature from minisign secret key"
#define SECRETKEY_DEFAULT_COMMENT      "minisign encrypted secret key"
#define TRUSTED_COMMENT_PREFIX         "trusted comment: "
#define SIG_DEFAULT_CONFIG_DIR         ".minisign"
#define SIG_DEFAULT_CONFIG_DIR_ENV_VAR "MINISIGN_CONFIG_DIR"
#define SIG_DEFAULT_PKFILE             "minisign.pub"
#define SIG_DEFAULT_SKFILE             "minisign.key"
#define SIG_SUFFIX                     ".minisig"

#define MINISIGN_KEYNUMBYTES            8
#define MINISIGN_TRUSTEDCOMMENTMAXBYTES 8192

typedef struct SigStruct_ {
    unsigned char sig_alg[2];
    unsigned char keynum[MINISIGN_KEYNUMBYTES];
    unsigned char sig[crypto_sign_BYTES];
} SigStruct;
typedef struct MinisignKeynumPK_ {
    unsigned char keynum[MINISIGN_KEYNUMBYTES];
    unsigned char pk[crypto_sign_PUBLICKEYBYTES];
} MinisignKeynumPK;


typedef enum MinisignError {
    MinisignOutOfMemory,
    MinisignReadError,
    MinisignWriteError,
    MinisignParseError,
    MinisignUsageError,
    MinisignLegacySignature,
    MinisignKeyError,
    MinisignVerificationFailed,
} MinisignError;

typedef struct MinisignPubkeyStruct_ {
    unsigned char    sig_alg[2];
    MinisignKeynumPK keynum_pk;
} MinisignPubkeyStruct;


MinisignPubkeyStruct *minisign_pubkey_load(const char *pk_file, const char *pubkey_s,
                                           MinisignError *const err);

void minisign_pubkey_free(MinisignPubkeyStruct *pubkey_struct);

int minisign_verify(MinisignPubkeyStruct *pubkey_struct, const char *message_file,
                    const char *sig_file, int output, int allow_legacy,
                    char                 trusted_comment[MINISIGN_TRUSTEDCOMMENTMAXBYTES],
                    MinisignError *const err);

#endif
