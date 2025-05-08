use bytes::BytesMut;
use std::collections::HashMap;
use std::net::IpAddr;
use std::sync::{Arc, RwLock};

pub mod peer;
use crate::peer::Peer;

type SharedState = Arc<RwLock<HashMap<String, Vec<Peer>>>>;

fn bencode_dict_compact(peers: &[Peer]) -> BytesMut {
    let mut out = BytesMut::new();
    out.extend_from_slice(b"d8:intervali120e5:peers");

    let mut peers_bytes = BytesMut::new();

    for peer in peers {
        if let IpAddr::V4(ipv4) = peer.ip {
            peers_bytes.extend_from_slice(&ipv4.octets());
            peers_bytes.extend_from_slice(&peer.port.to_be_bytes());
        }
    }

    out.extend_from_slice(format!("{}", peers_bytes.len()).as_bytes());
    out.extend_from_slice(&peers_bytes);
    out.extend_from_slice(b"e");

    out
}

fn main() {
    println!("Hello, world!");
}
