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
                 "list, stats, exit\n> ";

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
    } else if (action == "stats") {
      show_statistics();
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
  // Check if the file exists before proceeding.
  if (!std::filesystem::exists(file_path)) {
    std::cerr << "File does not exist: " << file_path << std::endl;
    return;
  }

  // Create a file storage object to represent the file structure.
  file_storage fs;
  add_files(fs, file_path); // Add the file to the file storage.

  // Create a torrent object from the file storage.
  create_torrent t(fs);

  // Add the tracker URL to the torrent. This is where peers will announce
  // themselves.
  t.add_tracker(tracker_url);

  // Generate piece hashes for the file. This ensures data integrity during
  // transfers. The `set_piece_hashes` function calculates the SHA-1 hash for
  // each piece of the file. These hashes are stored in the torrent metadata and
  // are used by peers to verify the integrity of the data they download. The
  // function takes:
  // - The `create_torrent` object to store the hashes.
  // - The base path of the file.
  // - A callback function to log progress for each piece.
  set_piece_hashes(t, std::filesystem::path(file_path).parent_path(),
                   [](int i) {
                     std::cout << "Hashing piece " << i
                               << std::endl; // Log progress for each piece.
                   });

  // Define the path where the .torrent file will be saved.
  std::string torrent_path =
      "torrents/" + std::filesystem::path(file_path).filename().string() +
      ".torrent";

  // Write the generated torrent metadata to a .torrent file.
  // The `bencode` function encodes the torrent metadata into the bencode
  // format, which is a compact, efficient encoding format used by the
  // BitTorrent protocol. The metadata includes information such as:
  // - File structure (name, size, etc.).
  // - Piece hashes for data integrity.
  // - Tracker URLs for peer discovery.
  std::ofstream out(torrent_path, std::ios::binary);
  if (!out) {
    std::cerr << "Failed to create torrent file: " << torrent_path << std::endl;
    return;
  }
  bencode(std::ostream_iterator<char>(out),
          t.generate()); // Encode the torrent metadata in bencode format.
  out.flush();

  std::cout << "Created torrent: " << torrent_path << std::endl;

  // Prepare the torrent for seeding by setting its parameters.
  add_torrent_params atp;
  atp.ti = std::make_shared<torrent_info>(
      torrent_path); // Load the torrent metadata.
  atp.save_path = std::filesystem::path(file_path)
                      .parent_path()
                      .string(); // Set the save path for the file.

  // Add the torrent to the libtorrent session for seeding.
  torrent_handle h = session_->add_torrent(std::move(atp));
  std::string name = h.status().name; // Get the name of the torrent.
  active_torrents_[name] =
      h; // Store the torrent handle in the active torrents map.

  std::cout << "Seeding: " << name << std::endl;
}

void Peer::download_torrent(const std::string &torrent_path) {
  // Check if the .torrent file exists before proceeding.
  if (!std::filesystem::exists(torrent_path)) {
    std::cerr << "Torrent file does not exist: " << torrent_path << std::endl;
    return;
  }

  // Prepare the parameters for adding the torrent to the session.
  add_torrent_params atp;

  // Load the torrent metadata from the .torrent file.
  atp.ti = std::make_shared<torrent_info>(torrent_path);

  // Set the directory where the downloaded files will be saved.
  atp.save_path = "downloads";

  // Add the torrent to the libtorrent session for downloading.
  torrent_handle h = session_->add_torrent(std::move(atp));

  // Retrieve the name of the torrent from its status.
  std::string name = h.status().name;

  // Store the torrent handle in the active torrents map for tracking.
  active_torrents_[name] = h;

  // Log the start of the download process.
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
