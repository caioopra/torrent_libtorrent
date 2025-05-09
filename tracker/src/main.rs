use dotenv::dotenv;
use std::collections::HashMap;
use std::net::IpAddr;
use std::sync::{Arc, RwLock};
use std::time::{Duration, SystemTime};

pub mod peer;
use actix_web::{get, web, App, HttpResponse, HttpServer, Responder};
use peer::bencode_dict_compact;

use crate::peer::Peer;

type SharedState = Arc<RwLock<HashMap<String, Vec<Peer>>>>;

#[get("/announce")]
async fn announce(
    query: web::Query<HashMap<String, String>>,
    req: actix_web::HttpRequest,
    state: web::Data<SharedState>,
) -> impl Responder {
    log::info!("Received request: {:?}", query);

    let info_hash = match query.get("info_hash") {
        Some(h) => h.clone(),
        None => {
            log::warn!("Missing info_hash in request");
            return HttpResponse::BadRequest().body("Missing info_hash");
        }
    };

    let peer_id = query.get("peer_id").cloned().unwrap_or_default();

    let port = match query.get("port").and_then(|p| p.parse::<u16>().ok()) {
        Some(p) => p,
        None => {
            log::warn!("Missing or invalid port in request");
            return HttpResponse::BadRequest().body("Missing or invalid port");
        }
    };

    let ip = req
        .peer_addr()
        .map(|addr| addr.ip())
        .unwrap_or(IpAddr::from([127, 0, 0, 1]));

    let event = query.get("event").cloned().unwrap_or_default();

    let mut map = state.write().unwrap();
    let peers = map.entry(info_hash.clone()).or_default();

    if event == "stopped" {
        peers.retain(|p| p.peer_id != peer_id);
        log::info!("Peer stopped: {}", peer_id);
    } else {
        // remove outdated peers (> 3 min)
        peers.retain(|p| p.last_seen.elapsed().unwrap_or_default() < Duration::from_secs(180));

        // adding / updating peer
        match peers.iter_mut().find(|p| p.peer_id == peer_id) {
            Some(existing) => {
                log::info!("Updated peer: {}", peer_id);
                existing.last_seen = SystemTime::now();
            }
            None => {
                peers.push(Peer {
                    ip,
                    port,
                    peer_id: peer_id.clone(),
                    last_seen: SystemTime::now(),
                });
                log::info!("Added new peer: {}", peer_id);
            }
        };
    }

    let response = bencode_dict_compact(peers);
    log::info!("Response: {:?}", response);

    HttpResponse::Ok().content_type("text/plain").body(response)
}

#[actix_web::main]
async fn main() -> std::io::Result<()> {
    dotenv().ok();
    env_logger::init();

    let state: SharedState = Arc::new(RwLock::new(HashMap::new()));
    log::info!("Tracker server running on http://0.0.0.0:8080");

    HttpServer::new(move || {
        App::new()
            .app_data(web::Data::new(state.clone()))
            .service(announce)
    })
    .bind(("0.0.0.0", 8080))?
    .run()
    .await
}
