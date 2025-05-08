#pragma once

#include <string>

class Peer {
public:
  explicit Peer(const std::string &file_to_seed);

  void start();

private:
  std::string file_path;
  void run_libtorrent_session();
};

bool create_torrent_file(const std::string &file_path,
                         const std::string &torrent_path,
                         const std::string &tracker_url = "");
