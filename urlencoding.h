#ifndef URLENCODING_H_INCLUDED
#define URLENCODING_H_INCLUDED

#ifdef __cplusplus
extern "C" {
#endif


unsigned urlencoded_length(const char *str);

unsigned urldecoded_length(const char *str);

void urlencode(char *dst, const char *src);

void urldecode(char *dst, const char *src);


#ifdef __cplusplus
}
#endif

#endif /* ndef URLENCODING_H_INCLUDED */
