#pragma once

#include <libtorrent/session.hpp>
#include <libtorrent/torrent_handle.hpp>
#include <string>
#include <unordered_map>

class Peer {
public:
  Peer();
  void start_cli_loop(); // accepts commands: "seed", "download", "stop", "list"

  void seed_file(const std::string &file_path, const std::string &tracker_url);
  void download_torrent(const std::string &torrent_path);
  void stop_torrent(const std::string &name_or_path);
  void list_active_torrents();

  void show_statistics();

private:
  std::unique_ptr<libtorrent::session> session_;
  std::unordered_map<std::string, libtorrent::torrent_handle> active_torrents_;

  void handle_alerts();
};
