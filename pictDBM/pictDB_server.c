/**
 * @file pictDB_server.c
 * @brief HTTP server to use the Picture Database Management Tool
 *
 *
 * @author Basile Thullen - Jeremy Hottinger
 * @date 17 May 2016
 */

#include "libmongoose/mongoose.h"
#include "pictDB.h"

// maximum number of parameters in the query string
#define MAX_QUERY_PARAM 5

// boolean to signal if the program is terminated
static int s_sig_received = 0;
// server options
static struct mg_serve_http_opts s_http_server_opts;
// port on which the server will be binded
static const char *s_http_port = "8000";

/* handler for actions */
static void handle_list_call(struct mg_connection* nc, struct http_message* hm);
static void handle_read_call(struct mg_connection* nc, struct http_message* hm);
static void handle_insert_call(struct mg_connection* nc, struct http_message* hm);
static void handle_delete_call(struct mg_connection* nc, struct http_message* hm);

/* request and signals handler */
static void ev_handler(struct mg_connection *nc, int ev, void *p);
static void signal_handler(int sig_num);

/* helper functions */
void mg_error(struct mg_connection* nc, int error);
void split (char* result[], char* tmp, const char* src, const char* delim, size_t len);

/**
 * @brief split a query string into an array of parameters
 *
 * @param result is the array which will contains the parameters of the string
 * @param tmp an array of character also containing the parameters of the string but
 *            but separated by '\0'
 * @param src array containing the original query string to split
 * @param delim array of delimiters on which to split the query string
 * @param len length of the query string because it's not null terminated
 */
void split (char* result[], char* tmp, const char* src, const char* delim, size_t len)
{
    if(tmp != NULL && src != NULL && delim != NULL && len > 0) {
        strncpy(tmp, src, len);
        char* tok = strtok(tmp, delim);
        int counter = 0;

        while(tok != NULL && counter < MAX_QUERY_PARAM-1) {
            result[counter++] = tok;
            tok = strtok(NULL, delim);
        }
        result[counter] = NULL;
    }
}

/**
 * @brief send a http header
 *
 * @param nc the connection at which to send the header
 * @param code the http response code
 * @param ct http Content-Type field
 * @param cl http Content-Length field
 */
static void send_header(struct mg_connection* nc, const char* code, const char* ct, size_t cl)
{
    mg_printf(nc, "HTTP/1.1 %s\r\nContent-Type: %s\r\nContent-Length: %lu\r\n\r\n", code, ct, cl); // send header
}

/**
 * @brief send a json formatted version of the list command
 *
 * @param nc connection at which to send
 * @param hm message received when the action was triggered
 */
static void handle_list_call(struct mg_connection* nc, struct http_message* hm)
{
    char* do_list_res = do_list(nc->user_data, JSON);
    send_header(nc, "200 OK", "application/json", strlen(do_list_res));
    mg_printf(nc, "%s", do_list_res);
    mg_send_http_chunk(nc, "", 0);
    free(do_list_res);
}

/**
 * @brief send the content of an image to a client
 *
 * @param connection at which to send
 * @param hm message containing the query string with parameters
 */
static void handle_read_call(struct mg_connection* nc, struct http_message* hm)
{
    char* result[MAX_QUERY_PARAM];
    char* tmp = calloc((MAX_PIC_ID + 1) * MAX_QUERY_PARAM, sizeof(char));
    struct mg_str query = hm->query_string;

    // get an array with key value pairs parameters
    split(result, tmp, query.p, "&=", query.len);

    int counter = 0;
    int res_code = -1;
    char* pict_id = NULL;
    char* curr = NULL;

    // pass through all query string parameters and get the ones that we
    // are interested in
    while((curr = result[counter++]) != NULL) {
        if(strcmp(curr, "res") == 0) {
            if(result[counter] != NULL) {
                res_code = resolution_atoi(result[counter]);
            }
        } else if (strcmp(curr, "pict_id") == 0) {
            if(result[counter] != NULL) {
                pict_id = result[counter];
            }
        }
    }

    char* img_array = NULL;
    uint32_t size = 0;
    // get the db file from the user data linked to the connection
    struct pictdb_file* db_file = (struct pictdb_file*)nc->user_data;

    // if we didn't get all the parameters return
    if(pict_id == NULL || res_code < 0) {
        mg_error(nc, ERR_IO);
        return;
    }

    // read the image from the database
    int res = do_read(pict_id, res_code, &img_array, &size, db_file);
    if(res != 0) {
        mg_error(nc, res);
        return;
    }

    // send a response if reading was good
    send_header(nc, "200 OK", "image/jpeg", size);
    mg_send(nc, img_array, size);

    free(img_array);
    free(tmp);
}

/**
 * @brief insert an image received from the client
 *
 * @param nc connection which has sent the image
 * @param hm message containing the image content
 */
static void handle_insert_call(struct mg_connection* nc, struct http_message* hm)
{
    char var_name[100], filename[MAX_PIC_ID];
    const char *image = NULL;
    size_t image_size = 0;

    // get the image content
    if(mg_parse_multipart(hm->body.p, hm->body.len, var_name, sizeof(var_name), filename, MAX_PIC_ID, &image, &image_size) == 0) {
        return;
    }

    struct pictdb_file* db_file = (struct pictdb_file*)nc->user_data;

    // try to insert, if fail forward the error to mg_error
    int res = do_insert(image, image_size, filename, db_file);
    if(res != 0) {
        mg_error(nc, res);
        return;
    }

    // if insert works, send response to client and redirect him
    mg_printf(nc, "HTTP/1.1 302 Found\r\nLocation: http://localhost:%s/index.html\r\n\r\n", s_http_port);
    mg_send_http_chunk(nc, "", 0);
}

/**
 * @brief delete an image chosen by a client
 *
 * @param nc connection which has initiate the delete request
 * @param hm message containing the query param
 */
static void handle_delete_call(struct mg_connection* nc, struct http_message* hm)
{
    char* result[MAX_QUERY_PARAM];
    char* tmp = calloc((MAX_PIC_ID + 1) * MAX_QUERY_PARAM, sizeof(char));
    struct mg_str query = hm->query_string;

    // fill the result tab
    split(result, tmp, query.p, "&=", query.len);

    char* pict_id = NULL;

    int counter = 0;
    while(result[counter] != NULL && (strcmp(result[counter], "pict_id") != 0)) {
        counter++;
    }

    // we couldn't find a parameter with key pict_id
    if(result[counter] == NULL) {
        mg_error(nc, ERR_IO);
        return;
    }

    pict_id = result[counter+1];

    if(pict_id == NULL) {
        mg_error(nc, ERR_INVALID_PICID);
        return;
    }

    struct pictdb_file* db_file = (struct pictdb_file*)nc->user_data;
    int res = do_delete(pict_id, db_file);
    if(res != 0) {
        mg_error(nc, res);
        return;
    }

    mg_printf(nc, "HTTP/1.1 302 Found\r\nLocation: http://localhost:%s/index.html\r\n\r\n", s_http_port);
    mg_send_http_chunk(nc, "", 0);
    free(tmp);
}

/**
 * @brief event handler that dispatch to sub-handlers
 *
 * @param nc connection which send the message
 * @param ev type of event that occured
 * @param p everything that can be sent by a client
 */
static void ev_handler(struct mg_connection *nc, int ev, void *p)
{

    struct http_message *hm = (struct http_message*) p;

    switch(ev) {
    case MG_EV_HTTP_REQUEST:
        if(mg_vcmp(&hm->uri, "/pictDB/list") == 0) {
            handle_list_call(nc, hm);
        } else if(mg_vcmp(&hm->uri, "/pictDB/read") == 0) {
            handle_read_call(nc, hm);
        } else if(mg_vcmp(&hm->uri, "/pictDB/insert") == 0) {
            handle_insert_call(nc, hm);
        } else if(mg_vcmp(&hm->uri, "/pictDB/delete") == 0) {
            handle_delete_call(nc, hm);
        } else {
            mg_serve_http(nc, hm, s_http_server_opts);
        }
    }
}

/**
 * @brief handle signals generated by OS
 *
 * @param sig_num signal number that arrived
 */
static void signal_handler(int sig_num)
{
    signal(sig_num, signal_handler);
    s_sig_received = sig_num;
}

/**
 * @brief send http error 500
 *
 * @param nc conection at which to send the error
 * @param error number corresponding to the error
 */

// static void send_header(struct mg_connection* nc, const char* code, const char* ct, size_t cl) {
void mg_error(struct mg_connection* nc, int error)
{
    size_t msg_len = 64 + strlen(ERROR_MESSAGES[error]);
    mg_printf(nc, "HTTP/1.1 500 %s\r\nContent-Type: text/html\r\nContent-Length: %zu\r\n\r\n<!DOCTYPE html><h1>Error 500</h1><p>Internal server errror: %s</p>\r\n\r\n", ERROR_MESSAGES[error], msg_len, ERROR_MESSAGES[error]);
    mg_send_http_chunk(nc, "", 0);
}


/**
 * @brief Entry point
 */
int main (int argc, char* argv[])
{

    // check for the right number of arguments
    if(argc != 2) {
        return ERR_INVALID_ARGUMENT;
    }

    if(strlen(argv[1]) > MAX_DB_NAME) {
        return ERR_INVALID_FILENAME;
    }

    if (VIPS_INIT(argv[0])) {
        vips_error_exit("unable to start VIPS");
    }

    struct pictdb_file db_file;
    // open the db_file
    if(do_open(argv[1], "rb+", &db_file) != 0) {
        return ERR_IO;
    }

    print_header(&(db_file.header));

    // assign a signal handler to SIGTERM and SIGINT to handle the server termination
    signal(SIGTERM, signal_handler);
    signal(SIGINT, signal_handler);

    struct mg_mgr mgr;
    struct mg_connection *nc;

    mg_mgr_init(&mgr, NULL);
    if((nc = mg_bind(&mgr, s_http_port, ev_handler)) == NULL) {
        do_close(&db_file);
        return ERR_IO;
    }
    nc->user_data = &db_file;

    // Set up HTTP server parameters
    mg_set_protocol_http_websocket(nc);
    s_http_server_opts.document_root = ".";
    s_http_server_opts.enable_directory_listing = "yes";

    printf("Server started on port %s\n", s_http_port);

    // listen while we didn't receive a termination signal
    while(!s_sig_received) {
        mg_mgr_poll(&mgr, 1000);
    }

    printf("\nExiting on signal %d\n", s_sig_received);

    do_close(&db_file);
    mg_mgr_free(&mgr);
    vips_shutdown();

    return 0;
}
