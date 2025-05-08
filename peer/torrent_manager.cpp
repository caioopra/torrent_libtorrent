#include <fstream>
#include <iostream>
#include <libtorrent/alert.hpp>
#include <libtorrent/bencode.hpp>
#include <libtorrent/create_torrent.hpp>
#include <libtorrent/file.hpp>
#include <libtorrent/file_storage.hpp>
#include <libtorrent/fwd.hpp>
#include <libtorrent/session.hpp>
#include <libtorrent/sha1_hash.hpp>
#include <libtorrent/write_resume_data.hpp>

using namespace lt;

bool create_torrent_file(const std::string &file_path,
                         const std::string &torrent_path,
                         const std::string &tracker_url = "",
                         const std::string &base_path = "torrents") {
  file_storage fs;
  add_files(fs, file_path, [](std::string const &) { return true; });

  create_torrent t(fs, 16 * 1024); // 16 KiB piece size

  // Optionally add a tracker
  if (!tracker_url.empty()) {
    t.add_tracker(tracker_url);
  }

  set_piece_hashes(t, "torrents", [](int i) {
    std::cout << "Calculating piece hash: " << i << std::endl;
  });
  std::cout << "Piece hashes calculated." << std::endl;

  entry e = t.generate();

  std::ofstream out(torrent_path, std::ios_base::binary);
  if (!out.is_open()) {
    std::cerr << "Failed to open " << torrent_path << " for writing"
              << std::endl;
    return false;
  }

  bencode(std::ostream_iterator<char>(out), e);
  std::cout << "Torrent file created at: " << torrent_path << std::endl;
  return true;
}
