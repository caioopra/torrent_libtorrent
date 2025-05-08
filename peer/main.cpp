#include "peer.h"

int main() {
  std::string file = "test_files/sample.jpg";
  std::string torrent = "test_files/sample.torrent";

  std::string tracker_url = "http://localhost:8080/announce";

  if (!create_torrent_file(file, torrent, tracker_url)) {
    return 1;
  }

  Peer peer(torrent);
  peer.start();

  return 0;
}
