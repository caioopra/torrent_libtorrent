use std::{net::IpAddr, time::SystemTime};

use bytes::BytesMut;

#[derive(Clone)]
pub struct Peer {
    pub ip: IpAddr,
    pub port: u16,
    pub peer_id: String,
    pub last_seen: SystemTime,
}

/// Encodes a list of peers into a compact bencoded dictionary format as required by the BitTorrent protocol.
///
/// The function generates a dictionary with the following structure:
/// - `interval`: A hardcoded value of 120 seconds, representing the interval at which peers should contact the tracker.
/// - `peers`: A compact representation of peer information.
///
/// The `peers` field is encoded as a binary string where each peer is represented by:
/// - 4 bytes for the IPv4 address (in network byte order).
/// - 2 bytes for the port number (in network byte order).
///
/// This compact format is used by the BitTorrent protocol to minimize the size of tracker responses.
///
/// # Arguments
/// - `peers`: A slice of `Peer` structs containing the peer information to encode.
///
/// # Returns
/// - A `BytesMut` containing the bencoded dictionary.
///
/// # Notes
/// - Only IPv4 addresses are included in the compact representation.
/// - The function does not include IPv6 addresses or additional peer metadata.
///
/// # Example
/// ```
/// let peers = vec![
///     Peer {
///         ip: "192.168.1.1".parse().unwrap(),
///         port: 6881,
///         peer_id: "peer1".to_string(),
///         last_seen: SystemTime::now(),
///     },
/// ];
/// let encoded = bencode_dict_compact(&peers);
/// ```
pub fn bencode_dict_compact(peers: &[Peer]) -> BytesMut {
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
