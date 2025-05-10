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

/// Handles `/announce` requests from peers in a BitTorrent network.
///
/// This method is a core part of the tracker server, responsible for managing
/// peer information for a specific torrent file identified by its `info_hash`.
/// It processes requests from peers to register, update, or remove themselves
/// from the tracker and responds with a list of active peers.
///
/// # Arguments
/// - `query`: A map of query parameters sent by the peer. Expected keys include:
///   - `info_hash`: The unique identifier of the torrent file.
///   - `peer_id`: The unique identifier of the peer.
///   - `port`: The port number the peer is listening on.
///   - `event` (optional): Indicates the peer's action (`started`, `stopped`, etc.).
/// - `req`: The HTTP request object, used to extract the peer's IP address.
/// - `state`: Shared state containing a map of `info_hash` to a list of active peers.
///
/// # Returns
/// - An HTTP response containing a compact list of active peers for the requested torrent.
///   The response is encoded in the bencoded dictionary format as required by the BitTorrent protocol.
///
/// # Behavior
/// - If the `event` is "stopped", the peer is removed from the list.
/// - If no `event` is provided, the peer is added or updated.
/// - Peers inactive for more than 3 minutes are automatically removed.
///
/// # Example
/// A peer sends a GET request to `/announce` with the following query:
/// ```
/// /announce?info_hash=abc123&peer_id=peer1&port=6881
/// ```
/// The tracker responds with a list of active peers for the torrent.
#[get("/announce")]
async fn announce(
    query: web::Query<HashMap<String, String>>,
    req: actix_web::HttpRequest,
    state: web::Data<SharedState>,
) -> impl Responder {
    log::info!("Received request: {:?}", query);

    // Extract the `info_hash` parameter, which identifies the torrent file.
    let info_hash = match query.get("info_hash") {
        Some(h) => h.clone(),
        None => {
            log::warn!("Missing info_hash in request");
            return HttpResponse::BadRequest().body("Missing info_hash");
        }
    };

    // Extract the `peer_id` parameter
    let peer_id = query.get("peer_id").cloned().unwrap_or_default();

    // Extract and validate the `port` parameter
    let port = match query.get("port").and_then(|p| p.parse::<u16>().ok()) {
        Some(p) => p,
        None => {
            log::warn!("Missing or invalid port in request");
            return HttpResponse::BadRequest().body("Missing or invalid port");
        }
    };

    // Determine the peer's IP address from the request or use a default value.
    let ip = req
        .peer_addr()
        .map(|addr| addr.ip())
        .unwrap_or(IpAddr::from([127, 0, 0, 1]));

    // Extract the `event` parameter, which indicates the peer's action (e.g., "started", "stopped").
    let event = query.get("event").cloned().unwrap_or_default();

    // Access the shared state to update the list of peers for the given `info_hash`.
    let mut map = state.write().unwrap();
    let peers = map.entry(info_hash.clone()).or_default();

    if event == "stopped" {
        // Remove the peer from the list if the event is "stopped".
        peers.retain(|p| p.peer_id != peer_id);
        log::info!("Peer stopped: {}", peer_id);
    } else {
        // Remove peers that have been inactive for more than 3 minutes.
        peers.retain(|p| p.last_seen.elapsed().unwrap_or_default() < Duration::from_secs(180));

        // Add or update the peer in the list.
        match peers.iter_mut().find(|p| p.peer_id == peer_id) {
            Some(existing) => {
                log::info!("Updated peer: {}", peer_id);
                existing.last_seen = SystemTime::now(); // Update the last seen timestamp.
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

    // Encode the list of peers into a compact response format.
    let response = bencode_dict_compact(peers);
    log::info!("Response: {:?}", response);

    // Return the response to the peer.
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
