int parse_dependencies(char *str, char job_names[MAX_JOBS][MAX_ARGUMENT_LEN], job *j);
void lowercase(char *str);
void submit_job_to_server(host_port host, job *to_send, data_size *files, int num_files);
int get_file_into_memory(FILE *file, data_size *location);
