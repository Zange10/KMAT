#ifndef KSP_CONNECTOR_H
#define KSP_CONNECTOR_H


typedef struct {
    int has_client;
    int server_is_open;
    int try_to_connect;
} KspConnector;


int open_connection();
int receive_data(double vector[3]);
void close_connection();


#endif //KSP_CONNECTOR_H
