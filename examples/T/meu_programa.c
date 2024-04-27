#include <stdio.h>
#include <string.h>
#include "mbedtls/aes.h"

#define BLOCK_SIZE 16  // Tamanho do bloco AES (16 bytes)

// Função para preencher o texto com zeros até ser múltiplo de 16 bytes
void zero_padding(unsigned char *plaintext, size_t length, unsigned char *padded_plaintext) {
    // Copia o texto original para o buffer preenchido
    memcpy(padded_plaintext, plaintext, length);
    // Preenche o restante do buffer com zeros
    memset(padded_plaintext + length, 0, BLOCK_SIZE - (length % BLOCK_SIZE));
}

// Função para criptografar o texto com a chave usando o modo ECB do AES com mbedTLS
void encrypt_text(unsigned char *text, unsigned char *key, size_t text_length, unsigned char *ciphertext) {
    // Calcula o tamanho necessário para preencher o texto
    size_t padded_len = ((text_length + BLOCK_SIZE - 1) / BLOCK_SIZE) * BLOCK_SIZE;

    // Buffer para armazenar o texto preenchido
    unsigned char padded_text[padded_len];

    // Preenche o texto com zeros para ser múltiplo de 16 bytes
    zero_padding(text, text_length, padded_text);

    // Inicializar a estrutura AES
    mbedtls_aes_context aes;
    mbedtls_aes_init(&aes);

    // Definir a chave para criptografia
    mbedtls_aes_setkey_enc(&aes, key, 128); // 128 bits

    // Criptografar cada bloco de 16 bytes do texto
    for (size_t i = 0; i < padded_len; i += BLOCK_SIZE) {
        mbedtls_aes_crypt_ecb(&aes, MBEDTLS_AES_ENCRYPT, &padded_text[i], &ciphertext[i]);
    }

    // Limpar a estrutura AES
    mbedtls_aes_free(&aes);
}

// Função para descriptografar o texto criptografado com a chave usando o modo ECB do AES com mbedTLS
void decrypt_text(unsigned char *ciphertext, unsigned char *key, size_t cipher_length, unsigned char *decrypted_text) {
    // Inicializar a estrutura AES
    mbedtls_aes_context aes;
    mbedtls_aes_init(&aes);

    // Definir a chave para descriptografia
    mbedtls_aes_setkey_dec(&aes, key, 128); // 128 bits

    // Descriptografar cada bloco de 16 bytes do texto criptografado
    for (size_t i = 0; i < cipher_length; i += BLOCK_SIZE) {
        mbedtls_aes_crypt_ecb(&aes, MBEDTLS_AES_DECRYPT, &ciphertext[i], &decrypted_text[i]);
    }

    // Limpar a estrutura AES
    mbedtls_aes_free(&aes);
}

int main() {
    // Chave de 16 bytes (128 bits)
    unsigned char key[16] = "abcdef012345678";

    // Texto a ser criptografado
    unsigned char text[] = "texto longo para criptografia";

    // Tamanho do texto
    size_t text_length = strlen(text);

    // Buffer para armazenar o texto criptografado
    // O tamanho deve ser igual ao texto preenchido
    size_t padded_len = ((text_length + BLOCK_SIZE - 1) / BLOCK_SIZE) * BLOCK_SIZE;
    unsigned char ciphertext[padded_len];

    // Chama a função para criptografar o texto
    encrypt_text(text, key, text_length, ciphertext);

    // Exibir o texto criptografado em formato hexadecimal
    printf("Texto criptografado: ");
    for (size_t i = 0; i < padded_len; i++) {
        printf("%02x", ciphertext[i]);
    }
    printf("\n");
	
	// Tamanho do texto criptografado
    size_t cipher_length = sizeof(ciphertext);
    
    // Buffer para armazenar o texto descriptografado
    unsigned char decrypted_text[cipher_length];
    
	// Chama a função para descriptografar o texto
    decrypt_text(ciphertext, key, cipher_length, decrypted_text);
	
	// Exibir o texto descriptografado
    printf("Texto descriptografado: %s\n", decrypted_text);
	
    return 0;
}

