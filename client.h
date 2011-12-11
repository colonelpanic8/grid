int parse_dependencies(char *str, char job_names[MAX_JOBS][MAX_ARGUMENT_LEN], job *j);
void lowercase(char *str);
int submit_job_to_server(char *host, int port, job *to_send, data_size *files, int num_files);
int get_file_into_memory(char *name, data_size *location);
void simple_add(char *host, int port);
