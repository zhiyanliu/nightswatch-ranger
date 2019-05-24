//
// Created by Liu, Zhiyan on 2019-05-24.
//

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

    if (NULL == obj_url) {
        return 1;
    }

    if (NULL == out_file_path) {
        return 1;
    }

    // init the curl session
    curl_handle = curl_easy_init();

    // set URL to get
    curl_easy_setopt(curl_handle, CURLOPT_URL, obj_url);

    // switch on full protocol/debug output
    curl_easy_setopt(curl_handle, CURLOPT_VERBOSE, 1L);

    // disable progress meter, set to 0L to enable and disable debug output
    curl_easy_setopt(curl_handle, CURLOPT_NOPROGRESS, 1L);

    // send all data to callback to write data
    curl_easy_setopt(curl_handle, CURLOPT_WRITEFUNCTION, write_data);

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
