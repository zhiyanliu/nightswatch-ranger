//
// Created by Liu, Zhiyan on 2019-05-24.
//

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <curl/curl.h>


void s3_http_init() {
    curl_global_init(CURL_GLOBAL_ALL);
}

static size_t write_data(void *ptr, size_t size, size_t nmemb, void *stream) {
    return fwrite(ptr, size, nmemb, (FILE *)stream);
}

int s3_http_download(char* obj_url, char* out_file_path) {
    CURL *curl_handle = NULL;
    FILE *file_out = NULL;
    char *http_proxy = NULL, *https_proxy = NULL;

    if (NULL == obj_url)
        return 1;

    if (NULL == out_file_path)
        return 1;

    http_proxy = getenv("HTTP_PROXY");
    if (NULL == http_proxy)
        http_proxy = getenv("http_proxy");
    https_proxy = getenv("HTTPS_PROXY");
    if (NULL == http_proxy)
        https_proxy = getenv("https_proxy");

    // init the curl session
    curl_handle = curl_easy_init();
    if (NULL == curl_handle)
        return 1;

    // set URL to get
    curl_easy_setopt(curl_handle, CURLOPT_URL, obj_url);

    // switch on full protocol/debug output
    curl_easy_setopt(curl_handle, CURLOPT_VERBOSE, 1L);

    // disable progress meter, set to 0L to enable and disable debug output
    curl_easy_setopt(curl_handle, CURLOPT_NOPROGRESS, 1L);

    // send all data to callback to write data
    curl_easy_setopt(curl_handle, CURLOPT_WRITEFUNCTION, write_data);

    if (NULL != https_proxy) // s3 uses https, try first
        curl_easy_setopt(curl_handle, CURLOPT_PROXY, https_proxy);
    else if (NULL != http_proxy)
        curl_easy_setopt(curl_handle, CURLOPT_PROXY, http_proxy);

     file_out = fopen(out_file_path, "wb");
    if(file_out) {
        // write the s3 object to the callback
        curl_easy_setopt(curl_handle, CURLOPT_WRITEDATA, file_out);

        // get request
        curl_easy_perform(curl_handle);

        fclose(file_out);
    }

    // cleanup curl
    curl_easy_cleanup(curl_handle);

    return 0;
}

void s3_http_free() {
    curl_global_cleanup();
}
