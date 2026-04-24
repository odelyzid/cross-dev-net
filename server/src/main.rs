//! mixnetd — TCP hub for 68mixCross protocol v0 (+ mixnetd extensions: PART, WHO, ROOMS).
//! Run on a modern host. Win9x (and other) clients connect with plain TCP.
//!
//! Usage: `mixnetd [port]`  (default 19677; bind 0.0.0.0)  
//! Env: `MIXNETD_IDLE_SEC=<n>` — if set, drop session after n seconds of no data on the line reader.

use std::collections::HashMap;
use std::io::{self, BufRead, BufReader, ErrorKind, Write};
use std::net::{TcpListener, TcpStream, Shutdown};
use std::time::Duration;
use std::sync::mpsc::{self, Receiver, Sender};
use std::sync::{Arc, Mutex};
use std::thread;

const DEFAULT_PORT: u16 = 19677;
const MAX_LINE: usize = 512;

struct Hub {
    next_id: u64,
    nick_to_session: HashMap<String, u64>,
    sessions: HashMap<u64, SessionData>,
}

struct SessionData {
    nick: Option<String>,
    room: Option<String>,
    out: Sender<String>,
}

type HubState = Arc<Mutex<Hub>>;

enum LineResult {
    Replies(Vec<String>),
    RepliesAndClose(Vec<String>),
    Silent, // e.g. MSG: broadcast only, nothing back to sender
}

fn main() {
    let port: u16 = std::env::args()
        .nth(1)
        .and_then(|s| s.parse().ok())
        .unwrap_or(DEFAULT_PORT);
    let addr = format!("0.0.0.0:{}", port);
    let listener = TcpListener::bind(&addr).unwrap_or_else(|e| {
        eprintln!("[mixnetd] bind {}: {}", addr, e);
        std::process::exit(1);
    });
    eprintln!("[mixnetd] listening on {} (protocol v0, cleartext)", addr);

    let state: HubState = Arc::new(Mutex::new(Hub {
        next_id: 1,
        nick_to_session: HashMap::new(),
        sessions: HashMap::new(),
    }));

    for stream in listener.incoming() {
        match stream {
            Ok(stream) => {
                let st = Arc::clone(&state);
                thread::spawn(move || {
                    if let Err(e) = client_session(st, stream) {
                        eprintln!("[mixnetd] session: {}", e);
                    }
                });
            }
            Err(e) => eprintln!("[mixnetd] accept: {}", e),
        }
    }
}

fn client_session(state: HubState, stream: TcpStream) -> std::io::Result<()> {
    let peer = stream.peer_addr().ok();
    let (out_tx, out_rx): (Sender<String>, Receiver<String>) = mpsc::channel();

    if let Some(secs) = read_idle_from_env() {
        let _ = stream.set_read_timeout(Some(Duration::from_secs(secs)));
    }

    let session_id = {
        let mut h = state.lock().unwrap();
        let id = h.next_id;
        h.next_id = h.next_id.saturating_add(1);
        h.sessions.insert(
            id,
            SessionData {
                nick: None,
                room: None,
                out: out_tx.clone(),
            },
        );
        id
    };
    let sid_hex = format!("{:016x}", session_id);
    if let Some(a) = peer {
        eprintln!("[mixnetd] + session {} from {}", sid_hex, a);
    }
    // MOTD (allowed before HELLO; see protocol INFO lines)
    let _ = out_tx.send(format!("INFO mixnetd session {} — send HELLO <nick> then JOIN <room>\n", sid_hex));

    let mut stream_write = stream.try_clone()?;
    let stream_kill = stream.try_clone()?;
    let writer = thread::spawn(move || {
        for line in out_rx {
            if stream_write.write_all(line.as_bytes()).is_err() {
                break;
            }
        }
        let _ = stream_write.flush();
    });

    let mut reader = BufReader::new(stream);
    let mut line_buf = String::new();

    loop {
        line_buf.clear();
        match reader.read_line(&mut line_buf) {
            Ok(0) => break,
            Ok(_) => {}
            Err(e) if is_read_idle(&e) => {
                let _ = out_tx.send("INFO idle_timeout\n".to_string());
                let _ = stream_kill.shutdown(Shutdown::Both);
                break;
            }
            Err(e) => return Err(e),
        };
        if line_buf.len() > MAX_LINE {
            let _ = out_tx.send("ERR line_too_long\n".to_string());
            continue;
        }
        let line = line_buf.trim_end_matches(['\n', '\r']);
        if line.is_empty() {
            continue;
        }

        let res = handle_line(&state, session_id, line);
        match res {
            LineResult::Replies(msgs) => {
                for m in msgs {
                    if out_tx.send(m).is_err() {
                        break;
                    }
                }
            }
            LineResult::RepliesAndClose(msgs) => {
                for m in msgs {
                    if out_tx.send(m).is_err() {
                        break;
                    }
                }
                break;
            }
            LineResult::Silent => {}
        }
    }

    remove_session(&state, session_id, true);
    drop(out_tx);
    let _ = writer.join();
    if let Some(a) = peer {
        eprintln!("[mixnetd] - session {} (peer {})", sid_hex, a);
    }
    Ok(())
}

fn read_idle_from_env() -> Option<u64> {
    std::env::var("MIXNETD_IDLE_SEC")
        .ok()?
        .parse::<u64>()
        .ok()
        .filter(|&n| n > 0)
}

fn is_read_idle(e: &io::Error) -> bool {
    matches!(e.kind(), ErrorKind::WouldBlock | ErrorKind::TimedOut)
}

/// If `notify_room`, other clients in the same room get `INFO <nick> left (disconnect)`.
fn remove_session(state: &HubState, session_id: u64, notify_room: bool) {
    let cell_opt = {
        let mut h = state.lock().unwrap();
        h.sessions.remove(&session_id)
    };
    if let Some(cell) = cell_opt {
        if let Some(ref n) = cell.nick {
            let mut h = state.lock().unwrap();
            h.nick_to_session.remove(n);
        }
        if notify_room {
            if let (Some(nick), Some(room)) = (&cell.nick, &cell.room) {
                let targets: Vec<Sender<String>> = {
                    let h = state.lock().unwrap();
                    h.sessions
                        .iter()
                        .filter(|(id, s)| {
                            *id != &session_id
                                && s.room.as_ref() == Some(room)
                                && s.nick.is_some()
                        })
                        .map(|(_, s)| s.out.clone())
                        .collect()
                };
                let line = format!("INFO {} left {}\n", nick, room);
                for tx in targets {
                    let _ = tx.send(line.clone());
                }
            }
        }
    }
}

fn handle_line(state: &HubState, session_id: u64, line: &str) -> LineResult {
    let (verb, rest) = if let Some(i) = line.find(' ') {
        (line[..i].to_uppercase(), &line[i + 1..])
    } else {
        (line.to_uppercase(), "")
    };
    let v = verb.as_str();

    match v {
        "PING" => {
            if !rest.is_empty() {
                return LineResult::Replies(vec!["ERR unknown_command\n".to_string()]);
            }
            LineResult::Replies(vec!["PONG\n".to_string()])
        }
        "QUIT" => {
            if !rest.is_empty() {
                return LineResult::Replies(vec!["ERR unknown_command\n".to_string()]);
            }
            LineResult::RepliesAndClose(vec!["OK bye\n".to_string()])
        }
        "HELLO" => handle_hello(state, session_id, rest),
        "JOIN" => handle_join(state, session_id, rest),
        "PART" => handle_part(state, session_id, rest),
        "WHO" => handle_who(state, session_id, rest),
        "ROOMS" => handle_rooms(state, session_id, rest),
        "MSG" => handle_msg(state, session_id, rest),
        _ => LineResult::Replies(vec!["ERR unknown_command\n".to_string()]),
    }
}

fn handle_hello(state: &HubState, session_id: u64, rest: &str) -> LineResult {
    if rest.is_empty() || rest.contains(' ') {
        return LineResult::Replies(vec!["ERR nick_invalid\n".to_string()]);
    }
    if !is_valid_nick_or_room(rest) {
        return LineResult::Replies(vec!["ERR nick_invalid\n".to_string()]);
    }
    {
        let h = state.lock().unwrap();
        if h
            .sessions
            .get(&session_id)
            .map(|c| c.nick.is_some())
            .unwrap_or(false)
        {
            return LineResult::Replies(vec!["ERR unknown_command\n".to_string()]);
        }
        if h.nick_to_session.contains_key(rest) {
            return LineResult::Replies(vec!["ERR nick_in_use\n".to_string()]);
        }
    }
    {
        let mut h = state.lock().unwrap();
        h.nick_to_session.insert(rest.to_string(), session_id);
        if let Some(ref mut c) = h.sessions.get_mut(&session_id) {
            c.nick = Some(rest.to_string());
        }
    }
    LineResult::Replies(vec![
        format!("OK hello {:016x}\n", session_id),
        format!("INFO welcome session {:016x}\n", session_id),
    ])
}

fn handle_join(state: &HubState, session_id: u64, rest: &str) -> LineResult {
    if rest.is_empty() || rest.contains(' ') {
        return LineResult::Replies(vec!["ERR no_such_room\n".to_string()]);
    }
    if !is_valid_nick_or_room(rest) {
        return LineResult::Replies(vec!["ERR no_such_room\n".to_string()]);
    }
    let (my_nick, old_room) = {
        let mut h = state.lock().unwrap();
        let cell = match h.sessions.get_mut(&session_id) {
            Some(c) => c,
            None => return LineResult::Replies(vec!["ERR unknown_command\n".to_string()]),
        };
        if cell.nick.is_none() {
            return LineResult::Replies(vec!["ERR not_hello\n".to_string()]);
        }
        if cell.room.as_deref() == Some(rest) {
            return LineResult::Replies(vec![format!("OK join {}\n", rest)]);
        }
        let nick = cell.nick.as_ref().unwrap().clone();
        let old = cell.room.take();
        (nick, old)
    };
    if let Some(ref old) = old_room {
        if old != rest {
            let targets: Vec<Sender<String>> = {
                let h = state.lock().unwrap();
                h.sessions
                    .iter()
                    .filter(|(id, s)| {
                        *id != &session_id
                            && s.room.as_ref() == Some(old)
                            && s.nick.is_some()
                    })
                    .map(|(_, s)| s.out.clone())
                    .collect()
            };
            for tx in targets {
                let _ = tx.send(format!("INFO {} left {}\n", my_nick, old));
            }
        }
    }
    {
        let mut h = state.lock().unwrap();
        h.sessions.get_mut(&session_id).unwrap().room = Some(rest.to_string());
    }
    let room = rest.to_string();
    let targets: Vec<Sender<String>> = {
        let h = state.lock().unwrap();
        h.sessions
            .iter()
            .filter(|(id, s)| {
                *id != &session_id
                    && s.room.as_ref() == Some(&room)
                    && s.nick.is_some()
            })
            .map(|(_, s)| s.out.clone())
            .collect()
    };
    for tx in targets {
        let _ = tx.send(format!("INFO {} joined {}\n", my_nick, room));
    }
    LineResult::Replies(vec![format!("OK join {}\n", rest)])
}

fn handle_part(state: &HubState, session_id: u64, rest: &str) -> LineResult {
    if !rest.is_empty() {
        return LineResult::Replies(vec!["ERR unknown_command\n".to_string()]);
    }
    {
        let h = state.lock().unwrap();
        if h
            .sessions
            .get(&session_id)
            .map(|c| c.nick.is_none() || c.room.is_none())
            .unwrap_or(true)
        {
            return LineResult::Replies(vec!["ERR not_in_room\n".to_string()]);
        }
    }
    let (nick, room) = {
        let mut h = state.lock().unwrap();
        let cell = h.sessions.get_mut(&session_id).unwrap();
        let nick = cell.nick.as_ref().unwrap().clone();
        let room = cell.room.take().unwrap();
        (nick, room)
    };
    let targets: Vec<Sender<String>> = {
        let h = state.lock().unwrap();
        h.sessions
            .iter()
            .filter(|(id, s)| {
                *id != &session_id
                    && s.room.as_ref() == Some(&room)
                    && s.nick.is_some()
            })
            .map(|(_, s)| s.out.clone())
            .collect()
    };
    for tx in targets {
        let _ = tx.send(format!("INFO {} left {}\n", nick, room));
    }
    LineResult::Replies(vec!["OK part\n".to_string()])
}

fn handle_who(state: &HubState, session_id: u64, rest: &str) -> LineResult {
    if !rest.is_empty() {
        return LineResult::Replies(vec!["ERR unknown_command\n".to_string()]);
    }
    let h = state.lock().unwrap();
    if h
        .sessions
        .get(&session_id)
        .map(|c| c.nick.is_none() || c.room.is_none())
        .unwrap_or(true)
    {
        return LineResult::Replies(vec!["ERR not_in_room\n".to_string()]);
    }
    let my_room = h
        .sessions
        .get(&session_id)
        .and_then(|c| c.room.as_ref().cloned())
        .unwrap();
    let mut nicks: Vec<&str> = h
        .sessions
        .iter()
        .filter(|(_, s)| s.room.as_ref() == Some(&my_room) && s.nick.is_some())
        .map(|(_, s)| s.nick.as_deref().unwrap())
        .collect();
    nicks.sort_unstable();
    let list = nicks.join(",");
    // OK who <room> <comma_nicks> — stay within 512 if possible; server-side truncation would break clients
    if list.len() + my_room.len() + 8 > 500 {
        return LineResult::Replies(vec!["ERR line_too_long\n".to_string()]);
    }
    LineResult::Replies(vec![format!("OK who {} {}\n", my_room, list)])
}

fn handle_rooms(state: &HubState, session_id: u64, rest: &str) -> LineResult {
    if !rest.is_empty() {
        return LineResult::Replies(vec!["ERR unknown_command\n".to_string()]);
    }
    {
        let h = state.lock().unwrap();
        if h
            .sessions
            .get(&session_id)
            .map(|c| c.nick.is_none())
            .unwrap_or(true)
        {
            return LineResult::Replies(vec!["ERR not_hello\n".to_string()]);
        }
    }
    let mut rooms: Vec<String> = {
        let h = state.lock().unwrap();
        h.sessions
            .values()
            .filter_map(|s| s.room.as_ref().cloned())
            .collect()
    };
    rooms.sort_unstable();
    rooms.dedup();
    if rooms.is_empty() {
        return LineResult::Replies(vec!["OK rooms\n".to_string()]);
    }
    let list = rooms.join(",");
    if list.len() > 480 {
        return LineResult::Replies(vec!["ERR line_too_long\n".to_string()]);
    }
    LineResult::Replies(vec![format!("OK rooms {}\n", list)])
}

fn handle_msg(state: &HubState, session_id: u64, text: &str) -> LineResult {
    {
        let h = state.lock().unwrap();
        if h
            .sessions
            .get(&session_id)
            .map(|c| c.nick.is_none())
            .unwrap_or(true)
        {
            return LineResult::Replies(vec!["ERR not_hello\n".to_string()]);
        }
        if h
            .sessions
            .get(&session_id)
            .map(|c| c.room.is_none())
            .unwrap_or(true)
        {
            return LineResult::Replies(vec!["ERR not_in_room\n".to_string()]);
        }
    }
    if !is_ascii_msg(text) {
        return LineResult::Replies(vec!["ERR unknown_command\n".to_string()]);
    }

    let (nick, room) = {
        let h = state.lock().unwrap();
        let c = h.sessions.get(&session_id).unwrap();
        (c.nick.as_ref().unwrap().clone(), c.room.as_ref().unwrap().clone())
    };
    let targets: Vec<Sender<String>> = {
        let h = state.lock().unwrap();
        h.sessions
            .iter()
            .filter(|(id, s)| {
                *id != &session_id
                    && s.room.as_ref() == Some(&room)
                    && s.nick.is_some()
            })
            .map(|(_, s)| s.out.clone())
            .collect()
    };
    let privmsg = format!("PRIVMSG {} {}\n", nick, text);
    for tx in targets {
        let _ = tx.send(privmsg.clone());
    }
    LineResult::Silent
}

fn is_valid_nick_or_room(s: &str) -> bool {
    let len = s.len();
    if len < 1 || len > 32 {
        return false;
    }
    s.chars()
        .all(|c| c.is_ascii_alphanumeric() || c == '_' || c == '-')
}

fn is_ascii_msg(s: &str) -> bool {
    s.chars()
        .all(|c| (c as u32) >= 0x20 && (c as u32) <= 0x7e)
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn valid_token() {
        assert!(is_valid_nick_or_room("user-1"));
        assert!(!is_valid_nick_or_room("bad name"));
    }

    #[test]
    fn msg_text() {
        assert!(is_ascii_msg("hello world"));
        assert!(!is_ascii_msg("a\nb"));
    }
}
