#include <chrono>
#include <iostream>
#include <thread>

#include "peer.h"

#include <libtorrent/add_torrent_params.hpp>
#include <libtorrent/magnet_uri.hpp>
#include <libtorrent/session.hpp>
#include <libtorrent/settings_pack.hpp>
#include <libtorrent/torrent_info.hpp>

using namespace lt;

Peer::Peer(const std::string &file_to_seed) : file_path(file_to_seed) {}

void Peer::start() { run_libtorrent_session(); }

void Peer::run_libtorrent_session() {
  settings_pack settings;
  settings.set_int(settings_pack::alert_mask, alert::status_notification);
  settings.set_str(settings_pack::listen_interfaces, "0.0.0.0:6881");

  session ses(settings);

  // seeding file
  add_torrent_params p;

  std::cout << "123123 " << std::endl;
  p.save_path = "./downloads";
  std::cout << "444 " << std::endl;
  p.ti =
      std::make_shared<torrent_info>(file_path); // creating torrent from file
      std::cout << "3333 " << std::endl;
  error_code ec;
  torrent_handle h = ses.add_torrent(p, ec);
  if (ec) {
    std::cerr << "Error adding torrent: " << ec.message() << std::endl;
    return;
  }

  std::cout << "Seeding " << file_path << std::endl;

  // event loop
  for (;;) {
    std::vector<alert *> alerts;
    ses.pop_alerts(&alerts);

    for (alert* a : alerts) {
      std::cout << a->message() << std::endl;
    }

    std::this_thread::sleep_for(std::chrono::seconds(1));
  }
}
