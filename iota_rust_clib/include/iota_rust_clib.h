#ifndef __IOTA_RUST_LIB_H
#define __IOTA_RUST_LIB_H

#ifdef __cplusplus
extern "C" {
#endif

// test function
uint32_t print_hello();

//! send indexiation message to iota, with pow
char* iota_send_indiced_transaction(const char* iota_url, const char* index, const char* message);

//void iota_send_indiced_transaction(iota_url: *const c_char, index: *const c_char, message: *const c_char)

//! release strings returned from rust
void free_rust_string(char* rust_string);

#ifdef __cplusplus
}
#endif

#endif //__IOTA_RUST_LIB_H