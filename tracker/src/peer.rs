use std::{net::IpAddr, time::SystemTime};

#[derive(Clone)]
pub struct Peer {
    pub ip: IpAddr,
    pub port: u16,
    peer_id: String,
    last_seen: SystemTime,
}
