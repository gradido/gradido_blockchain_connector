#ifndef __IOTA_RUST_LIB_H
#define __IOTA_RUST_LIB_H

#ifdef __cplusplus
extern "C" {
#endif

// test function
void print_hello();

//! send indexiation message to iota, with pow
char* iota_send_indiced_transaction(const char* iota_url_p, const char* index_p, const char* message_p);

//! release strings returned from rust
void free_rust_string(char* rust_string);

#ifdef __cplusplus
}
#endif

#endif //__IOTA_RUST_LIB_H