#ifdef __MINGW32__
#include <_mingw.h>
extern "C" {
    void quick_exit(int status);
    int at_quick_exit(void (*func)(void));
}
#endif

#include <iostream>
#include <openssl/evp.h>

int main() {
    std::cout << "Environment Test: OK" << std::endl;
    EVP_CIPHER_CTX *ctx = EVP_CIPHER_CTX_new();
    if (ctx) {
        std::cout << "OpenSSL Init: OK" << std::endl;
        EVP_CIPHER_CTX_free(ctx);
    }
    return 0;
}
