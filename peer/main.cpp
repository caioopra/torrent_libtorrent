#include "peer.h"
#include "torrent_manager.h"
#include <filesystem>
#include <iostream>

int main(int argc, char *argv[]) {
  if (argc < 3) {
    std::cout << "Usage:\n"
              << "  Share a file:    ./peer share <file_path>\n"
              << "  Download a file: ./peer download <torrent_path>\n";

    return 1;
  }

  std::string mode = argv[1];

  if (mode == "share") {
    std::string file_path = argv[2];
    std::cout << "Sharing file: " << file_path << std::endl;
    if (!std::filesystem::exists(file_path)) {
      std::cerr << "File does not exist: " << file_path << std::endl;
      return 1;
    }

    std::string torrent_path =
        "torrents/" + std::filesystem::path(file_path).filename().string() +
        ".torrent";
    std::cout << "Creating torrent file: " << torrent_path << std::endl;

    std::string tracker_url = "http://localhost:8080/announce";

    if (create_torrent_file(file_path, torrent_path, tracker_url)) {
      Peer peer(torrent_path, PeerMode::SEED);
      peer.start();
    }
  } else if (mode == "download") {
    std::string torrent_path = argv[2];
    download_torrent(torrent_path);
  } else {
    std::cerr << "Invalid mode. Use 'share' or 'download'." << std::endl;
    return 1;
  }

  return 0;
}
