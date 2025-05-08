#ifndef TORRENT_MANAGER_H
#define TORRENT_MANAGER_H

#include <string>

bool create_torrent_file(const std::string &file_path,
                         const std::string &torrent_path,
                         const std::string &tracker_url = "",
                         const std::string &base_path = "torrents");

void download_torrent(const std::string &torrent_path,
                      const std::string &save_path = "downloads");

#endif // TORRENT_MANAGER_H