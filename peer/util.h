#ifndef UTILITY_H
#define UTILITY_H

#include <vector>


void *get_in_addr(struct sockaddr *sa);
void add_to_pfds(int newfd, std::vector<struct pollfd> &pfds);
void del_from_pfds(size_t index, std::vector<struct pollfd> &pfds);

#endif //UTILITY_H
