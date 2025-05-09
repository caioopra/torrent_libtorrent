#include <chrono>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <iterator>
#include <libtorrent/bencode.hpp>
#include <libtorrent/create_torrent.hpp>
#include <libtorrent/file_storage.hpp>
#include <sstream>
#include <thread>

#include "peer.h"

#include <libtorrent/add_torrent_params.hpp>
#include <libtorrent/magnet_uri.hpp>
#include <libtorrent/session.hpp>
#include <libtorrent/settings_pack.hpp>
#include <libtorrent/torrent_info.hpp>

using namespace lt;

Peer::Peer() {
  settings_pack sp;
  sp.set_str(settings_pack::user_agent, "dynamic-peer/1.0");
  sp.set_int(settings_pack::alert_mask,
             alert::status_notification | alert::error_notification);

  session_ = std::make_unique<session>(sp);
}

void Peer::start_cli_loop() {
  std::string cmd;

  while (true) {
    std::cout << "\nCommands: seed <file>, download <.torrent>, stop <name>, "
                 "list, exit\n> ";

    std::getline(std::cin, cmd);
    std::istringstream iss(cmd);
    std::string action, arg;

    iss >> action >> std::ws;

    std::getline(iss, arg);

    if (action == "seed") {
      seed_file(arg, "http://localhost:8080/announce");
    } else if (action == "download") {
      download_torrent(arg);
    } else if (action == "stop") {
      stop_torrent(arg);
    } else if (action == "list") {
      list_active_torrents();
    } else if (action == "exit") {
      break;
    } else {
      std::cout << "Unknown command: " << action << std::endl;
    }

    handle_alerts();
  }
}

void Peer::seed_file(const std::string &file_path,
                     const std::string &tracker_url) {
  if (!std::filesystem::exists(file_path)) {
    std::cerr << "File does not exist: " << file_path << std::endl;
    return;
  }

  file_storage fs;
  add_files(fs, file_path);

  create_torrent t(fs);
  t.add_tracker(tracker_url);
  set_piece_hashes(t, std::filesystem::path(file_path).parent_path(), [](int i) {
    std::cout << "Hashing piece " << i << std::endl;
  });

  std::string torrent_path =
      "torrents/" + std::filesystem::path(file_path).filename().string() +
      ".torrent";
  std::ofstream out(torrent_path, std::ios::binary);
  if (!out) {
    std::cerr << "Failed to create torrent file: " << torrent_path << std::endl;
    return;
  }
  bencode(std::ostream_iterator<char>(out), t.generate());
  out.flush();

  std::cout << "Created torrent: " << torrent_path << std::endl;

  add_torrent_params atp;
  atp.ti = std::make_shared<torrent_info>(torrent_path);
  atp.save_path = std::filesystem::path(file_path).parent_path().string();

  torrent_handle h = session_->add_torrent(std::move(atp));
  std::string name = h.status().name;
  active_torrents_[name] = h;

  std::cout << "Seeding: " << name << std::endl;
}

void Peer::download_torrent(const std::string &torrent_path) {
  if (!std::filesystem::exists(torrent_path)) {
    std::cerr << "Torrent file does not exist: " << torrent_path << std::endl;
    return;
  }

  add_torrent_params atp;
  atp.ti = std::make_shared<torrent_info>(torrent_path);
  atp.save_path = "downloads";

  torrent_handle h = session_->add_torrent(std::move(atp));
  std::string name = h.status().name;
  active_torrents_[name] = h;

  std::cout << "Downloading: " << name << std::endl;
}

void Peer::stop_torrent(const std::string &name) {
  auto it = active_torrents_.find(name);
  if (it != active_torrents_.end()) {
    session_->remove_torrent(it->second);
    active_torrents_.erase(it);

    std::cout << "Stopped torrent: " << name << std::endl;

    return;
  }

  std::cerr << "Torrent not found: " << name << std::endl;
}

void Peer::show_statistics() {
  std::cout << "Active torrents:" << std::endl;
  for (const auto &[name, handle] : active_torrents_) {
    auto st = handle.status();
    std::cout << " - " << name << " ["
              << (st.is_seeding ? "Seeding" : "Downloading")
              << "] Progress: " << (int)(st.progress * 100) << "%"
              << " Downloaded: " << st.all_time_download
              << " Uploaded: " << st.all_time_upload
              << " Peers: " << st.num_peers
              << " Download Rate: " << st.download_rate / 1000
              << " KB/s Upload Rate: " << st.upload_rate / 1000 << " KB/s"
              << std::endl;
  }
}

void Peer::list_active_torrents() {
  if (active_torrents_.empty()) {
    std::cout << "No active torrents." << std::endl;
    return;
  }

  for (const auto &[name, handle] : active_torrents_) {
    auto st = handle.status();
    std::cout << " - " << name << " ["
              << (st.is_seeding ? "Seeding" : "Downloading")
              << "] Progress: " << (int)(st.progress * 100) << "%" << std::endl;
  }
}

void Peer::handle_alerts() {
  std::vector<alert *> alerts;

  session_->pop_alerts(&alerts);

  for (alert *a : alerts) {
    std::cout << "Alert: " << a->message() << std::endl;
  }
}
