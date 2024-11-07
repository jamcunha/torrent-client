#ifndef TORRENT_FILE_H
#define TORRENT_FILE_H

#include "bencode.h"

/**
 * @brief Parse a torrent file
 *
 * @param filename The torrent file to parse
 * @return bencode_node_t* The parsed torrent file
 */
bencode_node_t *torrent_file_parse(const char *filename);

#endif // !TORRENT_FILE_H
