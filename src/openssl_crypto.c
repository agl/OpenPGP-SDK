#include "crypto.h"
#include <openssl/md5.h>
#include <openssl/sha.h>
#include <openssl/dsa.h>
#include <openssl/rsa.h>
#include <assert.h>
#include <stdlib.h>

static void md5_init(ops_hash_t *hash)
    {
    assert(!hash->data);
    hash->data=malloc(sizeof(MD5_CTX));
    MD5_Init(hash->data);
    }

static void md5_add(ops_hash_t *hash,const unsigned char *data,unsigned length)
    {
    MD5_Update(hash->data,data,length);
    }

static unsigned md5_finish(ops_hash_t *hash,unsigned char *out)
    {
    MD5_Final(out,hash->data);
    free(hash->data);
    hash->data=NULL;
    return 16;
    }

static ops_hash_t md5={md5_init,md5_add,md5_finish};

void ops_hash_md5(ops_hash_t *hash)
    {
    *hash=md5;
    }

static void sha1_init(ops_hash_t *hash)
    {
    assert(!hash->data);
    hash->data=malloc(sizeof(SHA_CTX));
    SHA1_Init(hash->data);
    }

static void sha1_add(ops_hash_t *hash,const unsigned char *data,
		     unsigned length)
    {
    SHA1_Update(hash->data,data,length);
    }

static unsigned sha1_finish(ops_hash_t *hash,unsigned char *out)
    {
    SHA1_Final(out,hash->data);
    free(hash->data);
    hash->data=NULL;
    return 20;
    }

static ops_hash_t sha1={sha1_init,sha1_add,sha1_finish};

void ops_hash_sha1(ops_hash_t *hash)
    {
    *hash=sha1;
    }

ops_boolean_t ops_dsa_verify(const unsigned char *hash,size_t hash_length,
			     const ops_dsa_signature_t *sig,
			     const ops_dsa_public_key_t *dsa)
    {
    DSA_SIG *osig;
    DSA *odsa;
    int ret;

    osig=DSA_SIG_new();
    osig->r=sig->r;
    osig->s=sig->s;

    odsa=DSA_new();
    odsa->p=dsa->p;
    odsa->q=dsa->q;
    odsa->g=dsa->g;
    odsa->pub_key=dsa->y;

    ret=DSA_do_verify(hash,hash_length,osig,odsa);
    assert(ret >= 0);

    odsa->p=odsa->q=odsa->g=odsa->pub_key=NULL;
    DSA_free(odsa);
 
    osig->r=osig->s=NULL;
    DSA_SIG_free(osig);

    return ret != 0;
    }

int ops_rsa_public_decrypt(unsigned char *out,const unsigned char *in,
			   size_t length,const ops_rsa_public_key_t *rsa)
    {
    RSA *orsa;
    int n;

    orsa=RSA_new();
    orsa->n=rsa->n;
    orsa->e=rsa->e;

    n=RSA_public_decrypt(length,in,out,orsa,RSA_NO_PADDING);

    orsa->n=orsa->e=NULL;
    RSA_free(orsa);

    return n;
    }